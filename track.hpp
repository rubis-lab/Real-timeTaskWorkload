#ifndef __TRACK_H__
#define __TRACK_H__
#define USE_POSIX_SHARED_MEMORY
#define RUBIS_SCALE 1
#define ITERATION_RUBIS 1
#define ITERATION_COUNT_FROM 0
#include <opencv2/core/utility.hpp>
#include <opencv2/core/core.hpp>
//#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <sys/time.h>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <string>

#include "utils.h"

using namespace std;
using namespace cv;

static void crop(IplImage *src, IplImage *dst, CvRect rect)
{
	cvSetImageROI(src, rect);     // clip "rect" from "src" image
	cvCopy(src, dst);             // copy clipped portion to "dst"
	cvResetImageROI(src);         // reset cliping of "src"
}

struct Lane {
	Lane(){}
	Lane(CvPoint a, CvPoint b, float angle, float kl, float bl)
	: p0(a), p1(b), angle(angle), votes(0),visited(false),found(false),k(kl),b(bl) { }
	CvPoint p0, p1;
	float angle;
	int votes;
	bool visited, found;
	float k, b;
};

struct Status {
	Status(): reset(true),lost(0){}
	ExpMovingAverage k, b;
	bool reset;
	int lost;
};

#define GREEN  CV_RGB(0, 255, 0)
#define RED    CV_RGB(255, 0, 0)
#define BLUE   CV_RGB(0, 0, 255)
#define PURPLE CV_RGB(255, 0, 255)

Status laneR, laneL;

enum {
  SCAN_STEP           = 5,      // in pixels
  LINE_REJECT_DEGREES = 10,     // in degrees
  BW_TRESHOLD         = 250,    // edge response strength to recognize for 'WHITE'
  BORDERX             = 10,     // px, skip this much from left & right borders
  MAX_RESPONSE_DIST   = 5,      // px

  CANNY_MIN_TRESHOLD = 1,       // edge detector minimum hysteresis threshold
  CANNY_MAX_TRESHOLD = 100,     // edge detector maximum hysteresis threshold

  HOUGH_TRESHOLD        = 50,   // line approval vote threshold
  HOUGH_MIN_LINE_LENGTH = 50,   // remove lines shorter than this treshold
  HOUGH_MAX_LINE_GAP    = 100   // join lines to one with smaller than this gaps
};

#define K_VARY_FACTOR 0.2f
#define B_VARY_FACTOR 20
#define MAX_LOST_FRAMES 30

static void FindResponses(IplImage *img, int startX, int endX, int y, std::vector<int> &list)
{
  /* scans for single response: /^\_ */

  const int row = y * img->width * img->nChannels;
  unsigned char *ptr = (unsigned char *)img->imageData;

  int step = (endX < startX) ? -1 : 1;
  int range = (endX > startX) ? endX-startX+1 : startX-endX+1;

  for (int x = startX; range>0; x+=step, range--)
    {
      if (ptr[row + x] <= BW_TRESHOLD) continue; // skip black: loop until white pixels show up

      /* first response found */
      int idx = x + step;

      /* skip same response(white) pixels */
      while (range > 0 && ptr[row+idx] > BW_TRESHOLD) {
        idx += step;
        range--;
    }

      /* reached black again */
      if (ptr[row+idx] <= BW_TRESHOLD) {
        list.push_back(x);
      }

      x = idx;                  // begin from new pos
    }
}

static unsigned char pixel(IplImage *img, int x, int y)
{
  return (unsigned char)img->imageData[(y*img->width+x)*img->nChannels];
}

static int findSymmetryAxisX(IplImage *half_frame, CvPoint bmin, CvPoint bmax)
{
  float value = 0;
  int axisX = -1;               // not found

  int xmin = bmin.x;
  int ymin = bmin.y;
  int xmax = bmax.x;
  int ymax = bmax.y;
  int half_width = half_frame->width/2;

  for (int x=xmin, j=0; x<xmax; x++, j++)
    {
      float HS = 0;
      for (int y=ymin; y<ymax; y++)
        {
          int row = y*half_frame->width*half_frame->nChannels;
          for (int step=1; step<half_width; step++)
            {
              int neg = x-step;
              int pos = x + step;
              unsigned char Gneg = (neg < xmin) ? 0 :
                (unsigned char)half_frame->imageData[row+neg*half_frame->nChannels];
              unsigned char Gpos = (pos >= xmax) ? 0 :
                (unsigned char)half_frame->imageData[row+pos*half_frame->nChannels];
              HS += abs(Gneg - Gpos);
            }
        }

      if (axisX == -1 || value > HS) { // find minimum
        axisX = x;
        value = HS;
      }
    }

  return axisX;
}

static bool hasVertResponse(IplImage *edges, int x, int y, int ymin, int ymax)
{
  bool has = (pixel(edges, x, y) > BW_TRESHOLD);
  if (y-1 >= ymin) has &= (pixel(edges, x, y-1) < BW_TRESHOLD);
  if (y+1 < ymax) has &= (pixel(edges, x, y+1) < BW_TRESHOLD);
  return has;
}

static int horizLine(IplImage *edges, int x, int y, CvPoint bmin, CvPoint bmax, int maxHorzGap)
{
  /* scan to right */
  int right = 0;
  int gap = maxHorzGap;
  for (int xx=x; xx<bmax.x; xx++)
    {
      if (hasVertResponse(edges, xx, y, bmin.y, bmax.y))
        {
          right++;
          gap = maxHorzGap;     // reset
        }
      else
        {
          gap--;
          if (gap <= 0)
            {
              break;
            }
        }
    }

  int left = 0;
  gap = maxHorzGap;
  for (int xx=x-1; xx>=bmin.x; xx--)
    {
      if (hasVertResponse(edges, xx, y, bmin.y, bmax.y))
        {
          left++;
          gap = maxHorzGap;     // reset
        }
      else
        {
          gap--;
          if (gap <= 0)
            {
              break;
            }
        }
    }

  return left + right;
}

void processSide(std::vector<Lane> lanes, IplImage *edges, bool right)
{
  Status *side = right ? &laneR : &laneL;

  /* response search */
  int w = edges->width;
  int h = edges->height;
  const int BEGINY = 0;
  const int ENDY = h-1;
  const int ENDX = right ? (w-BORDERX) : BORDERX;
  int midx = w/2;
  int midy = edges->height/2;

  /* show responses */
  int *votes = new int[lanes.size()];
  for (std::size_t i = 0; i < lanes.size(); ++i) votes[i++] = 0;

  for (int y=ENDY; y>=BEGINY; y-=SCAN_STEP)
    {
      std::vector<int> rsp;
      FindResponses(edges, midx, ENDX, y, rsp);

      if (rsp.size() > 0)
        {
          int response_x = rsp[0]; // use first response (closest to screen center)

          float dmin = 9999999;
          float xmin = 9999999;
          int match = -1;

          for (std::size_t j = 0; j < lanes.size(); ++j)
            {
              /* compute response point destance to current line */
              float d = dist2line (
                                   cvPoint2D32f(lanes[j].p0.x, lanes[j].p0.y),
                                   cvPoint2D32f(lanes[j].p1.x, lanes[j].p1.y),
                                   cvPoint2D32f(response_x, y));

              /* point on line at current y line */
              int xline = (y - lanes[j].b) / lanes[j].k;
              int dist_mid = abs(midx - xline); // distance to midpoint

              /* pick the best closest match to line & to screen center */
              if (match == -1 || (d <= dmin && dist_mid < xmin))
                {
                  dmin = d;
                  match = j;
                  xmin = dist_mid;
                  break;
                }
            }

          /* vote for each line */
          if (match != -1)
            {
              votes[match] += 1;
            }

        }
    }

  int bestMatch = -1;
  int mini = 9999999;
  for (std::size_t i = 0; i<lanes.size(); ++i)
    {
      int xline = (midy - lanes[i].b) / lanes[i].k;
      int dist = abs(midx - xline); // distance to midpoint

      if (bestMatch == -1 || (votes[i] > votes[bestMatch] && dist < mini))
        {
          bestMatch = i;
          mini = dist;
        }
    }

  if (bestMatch != -1)
    {
      Lane *best = &lanes[bestMatch];
      float k_diff = fabs(best->k - side->k.get());
      float b_diff = fabs(best->b - side->b.get());

      bool update_ok = (k_diff <= K_VARY_FACTOR && b_diff <= B_VARY_FACTOR) || side->reset;

#ifdef SHOW_DETAIL
      printf("side: %s, k vary: %.4f, b vary: %.4f, lost: %s\n",
             (right?"RIGHT":"LEFT"), k_diff, b_diff, (update_ok?"no":"yes"));
#endif

      if (update_ok)
        {
          /* update is in valid bounds */
          side->k.add(best->k);
          side->b.add(best->b);
          side->reset = false;
          side->lost = 0;
        }
      else
        {
          /* can't update, lanes flicker periodically, start counter for partial reset! */
          side->lost++;
          if (side->lost >= MAX_LOST_FRAMES && !side->reset)
            {
              side->reset = true;
            }
        }
    }
  else
    {
#ifdef SHOW_DETAIL
      printf("no lanes detected - lane tracking lost! counter increased\n");
#endif
      side->lost++;
      if (side->lost >= MAX_LOST_FRAMES && !side->reset)
        {
          /* do full reset when lost for more than N frames */
          side->reset = true;
          side->k.clear();
          side->b.clear();
        }
    }

  delete[] votes;
}

static void processLanes(CvSeq *lines, IplImage* edges, IplImage *temp_frame, IplImage *org_frame)
{

  /* classify lines to left/right side */
  std::vector<Lane> left, right;
  for (int i=0; i<lines->total; i++)
    {
      CvPoint *line = (CvPoint *)cvGetSeqElem(lines, i);
      int dx = line[1].x - line[0].x;
      int dy = line[1].y - line[0].y;
      float angle = atan2f(dy, dx) * 180/CV_PI;

      if (fabs(angle) <= LINE_REJECT_DEGREES) // reject near horizontal lines
        {
          continue;
        }

      /* assume that vanishing point is close to the image horizontal center
         calculate line parameters: y = kx + b; */
      dx = (dx == 0) ? 1 : dx;  // prevent DIV/0!
      float k = dy/(float)dx;
      float b = line[0].y - k*line[0].x;

      /* assign lane's side based by its midpoint position */
      int midx = (line[0].x + line[1].x) / 2;
      if (midx < temp_frame->width/2)
        {
          left.push_back(Lane(line[0], line[1], angle, k, b));
        }
      else if (midx > temp_frame->width/2)
        {
          right.push_back(Lane(line[0], line[1], angle, k, b));
        }
    }
  /* show Hough lines */
  int org_offset = temp_frame->height;
  for (std::size_t i = 0; i < right.size(); ++i)
    {
      CvPoint org_p0 = right[i].p0;
      org_p0.y += org_offset;
      CvPoint org_p1 = right[i].p1;
      org_p1.y += org_offset;
#ifdef USE_POSIX_SHARED_MEMORY
#ifdef SHOW_DETAIL
      cvLine(temp_frame, right[i].p0, right[i].p1, BLUE, 2);
#endif
      cvLine(org_frame, org_p0, org_p1, BLUE, 2);
#endif
    }
  for (std::size_t i = 0; i < left.size(); ++i)
    {
      CvPoint org_p0 = left[i].p0;
      org_p0.y += org_offset;
      CvPoint org_p1 = left[i].p1;
      org_p1.y += org_offset;
#ifdef USE_POSIX_SHARED_MEMORY
#ifdef SHOW_DETAIL
      cvLine(temp_frame, left[i].p0, left[i].p1, RED, 2);
#endif
      cvLine(org_frame, org_p0, org_p1, RED, 2);
#endif
    }
  processSide(left, edges, false);
  processSide(right, edges, true);
  /* show computed lanes */
  int x = temp_frame->width * 0.55f;
  int x2 = temp_frame->width;
#if defined(USE_POSIX_SHARED_MEMORY)
#ifdef SHOW_DETAIL
  cvLine(temp_frame, cvPoint(x, laneR.k.get()*x + laneR.b.get()),
         cvPoint(x2, laneR.k.get()*x2 + laneR.b.get()), PURPLE, 2);
#endif

  cvLine(org_frame, cvPoint(x, laneR.k.get()*x + laneR.b.get() + org_offset),
         cvPoint(x2, laneR.k.get()*x2 + laneR.b.get() + org_offset), PURPLE, 2);
#else
  autoware_msgs::ImageLaneObjects lane_msg;
  lane_msg.lane_r_x1 = x;
  lane_msg.lane_r_y1 = laneR.k.get()*x + laneR.b.get() + org_offset;
  lane_msg.lane_r_x2 = x2;
  lane_msg.lane_r_y2 = laneR.k.get()*x2 + laneR.b.get() + org_offset;
#endif
  x = temp_frame->width * 0;
  x2 = temp_frame->width * 0.45f;
#if defined(USE_POSIX_SHARED_MEMORY)
#ifdef SHOW_DETAIL
  cvLine(temp_frame, cvPoint(x, laneL.k.get()*x + laneL.b.get()),
         cvPoint(x2, laneL.k.get()*x2 + laneL.b.get()), PURPLE, 2);
#endif

  cvLine(org_frame, cvPoint(x, laneL.k.get()*x + laneL.b.get() + org_offset),
         cvPoint(x2, laneL.k.get()*x2 + laneL.b.get() + org_offset), PURPLE, 2);
#else
  lane_msg.lane_l_x1 = x;
  lane_msg.lane_l_y1 = laneL.k.get()*x + laneL.b.get() + org_offset;
  lane_msg.lane_l_x2 = x2;
  lane_msg.lane_l_y2 = laneL.k.get()*x2 + laneL.b.get() + org_offset;

  image_lane_objects.publish(lane_msg);
#endif
  // cvLine(org_frame, cvPoint(lane_msg.lane_l_x1, lane_msg.lane_l_y1), cvPoint(lane_msg.lane_l_x2, lane_msg.lane_l_y2), RED, 5);
  // cvLine(org_frame, cvPoint(lane_msg.lane_r_x1, lane_msg.lane_r_y1), cvPoint(lane_msg.lane_r_x2, lane_msg.lane_r_y2), RED, 5);
}

static void process_image_common(IplImage *frame)
{

  CvFont font;
  cvInitFont(&font, CV_FONT_VECTOR0, 0.25f, 0.25f);
  CvSize video_size;
#if defined(USE_POSIX_SHARED_MEMORY)
  video_size.height = frame->height;
  video_size.width  = frame->width;
  //video_size.height = *shrd_ptr_height;
  //video_size.width  = *shrd_ptr_width;

#else
  // XXX These parameters should be set ROS parameters
  video_size.height = frame->height;
  video_size.width  = frame->width;
#endif
  CvSize    frame_size = cvSize(video_size.width, video_size.height/2);
  IplImage *temp_frame = cvCreateImage(frame_size, IPL_DEPTH_8U, 3);
  IplImage *gray       = cvCreateImage(frame_size, IPL_DEPTH_8U, 1);
  IplImage *edges      = cvCreateImage(frame_size, IPL_DEPTH_8U, 1);
  IplImage *half_frame = cvCreateImage(cvSize(video_size.width/2, video_size.height/2), IPL_DEPTH_8U, 3);

  CvMemStorage *houghStorage = cvCreateMemStorage(0);
  cvPyrDown(frame, half_frame, CV_GAUSSIAN_5x5); // Reduce the image by 2
  /* we're intersted only in road below horizont - so crop top image portion off */
  crop(frame, temp_frame, cvRect(0, frame_size.height, frame_size.width, frame_size.height));
  // removed
  //cvCvtColor(temp_frame, gray, CV_BGR2GRAY); // contert to grayscale
  /* Perform a Gaussian blur & detect edges */
  // smoothing image more strong than original program
  cvSmooth(gray, gray, CV_GAUSSIAN, 15, 15);
  // removed
  //cvCanny(gray, edges, CANNY_MIN_TRESHOLD, CANNY_MAX_TRESHOLD);
  /* do Hough transform to find lanes */
  double rho = 1;
  double theta = CV_PI/180;
  CvSeq *lines = cvHoughLines2(edges, houghStorage, CV_HOUGH_PROBABILISTIC,
                               rho, theta, HOUGH_TRESHOLD, HOUGH_MIN_LINE_LENGTH, HOUGH_MAX_LINE_GAP);
  processLanes(lines, edges, temp_frame, frame);


#ifdef SHOW_DETAIL
  /* show middle line */
  cvLine(temp_frame, cvPoint(frame_size.width/2, 0),
         cvPoint(frame_size.width/2, frame_size.height), CV_RGB(255, 255, 0), 1);

  // cvShowImage("Gray", gray);
  // cvShowImage("Edges", edges);
  // cvShowImage("Color", temp_frame);
  // cvShowImage("temp_frame", temp_frame);
  // cvShowImage("frame", frame);
#endif

#if defined(USE_POSIX_SHARED_MEMORY)

  //cvNamedWindow("Test", 1);
  //cvShowImage("Test", frame);
  //setImage_toSHM(frame);
#endif

#ifdef SHOW_DETAIL
  // cvMoveWindow("Gray", 0, 0);
  // cvMoveWindow("Edges", 0, frame_size.height+25);
  // cvMoveWindow("Color", 0, 2*(frame_size.height+25));
#endif

  cvReleaseMemStorage(&houghStorage);
  cvReleaseImage(&gray);
  cvReleaseImage(&edges);
  cvReleaseImage(&temp_frame);
  cvReleaseImage(&half_frame);
}

#if !defined(USE_POSIX_SHARED_MEMORY)
static void lane_cannyhough_callback(const sensor_msgs::Image& image_source)
{
  cv_bridge::CvImagePtr cv_image = cv_bridge::toCvCopy(image_source, sensor_msgs::image_encodings::BGR8);
  IplImage frame = cv_image->image;
  process_image_common(&frame);
  cvWaitKey(2);
}
#endif

int autoware_vision_thr_routine(int num_thr)
{
  // force opencv single thread
  // omp_set_dynamic(0);
  // omp_set_num_threads(1);
  // int max_num_of_thread = 4;
  // int num_of_thread = 1;
  // if(argc >= 1) {
  //   num_of_thread = atoi(argv[1]);
  //   if(num_of_thread > max_num_of_thread) {
  //     num_of_thread = max_num_of_thread;
  //   }

  //printf("num thr: %d\n", num_thr);
  // }
  // int num_of_thread = num_thr;
  IplImage* road_image_final = cvLoadImage("road_x3.jpg");
  //Mat road_image = imread("road.jpg");

  //IplImage* road_image_final;
  //IplImage copy;
/*
  copy=road_image;
  road_image_final=&copy;
*/

  // rubis - baseline
  //road_image_final = cvLoadImage("road_x6.jpg");
  // rubis - iteration num table
  
  int inner_iteration[4] = {6, 4, 2, 1};
  for(int i = 0; i < 4; i++) {
    inner_iteration[i] = inner_iteration[i] * RUBIS_SCALE;
  }
  
  struct timeval start, end;

  // previously inside while loop -rubis
  CvMemStorage *houghStorage = cvCreateMemStorage(0);


  long sum_time = 0;
	// while (1) {
  for(int i = 0; i < ITERATION_RUBIS; i++) {
    gettimeofday(&start, NULL);
    ///getImage_fromSHM -> not in opencv
    //IplImage *frame = getImage_fromSHM();
    //IplImage* frame = road_image;
    // rubis - inner iteration to scale WCET
    for(int j = 0; j < inner_iteration[num_thr - 1]; j++) {
      process_image_common(road_image_final);
      //cvReleaseImage(&frame);
    }  
    // rubis - time measure
    gettimeofday(&end, NULL);
    long loop_time = ((end.tv_sec * 1000000 + end.tv_usec)
            - (start.tv_sec * 1000000 + start.tv_usec));
    //printf("looptime: %ld\n", loop_time);
    if(i >= ITERATION_COUNT_FROM) {
      sum_time += loop_time;
    } 
  }

  long int avg_time = sum_time / (ITERATION_RUBIS - ITERATION_COUNT_FROM);

  //printf("_TRCK finished in %ld ms.\n", sum_time);
  //printf("_TRCK finished in average %ld ms.\n", avg_time);

	//cvReleaseImage(&road_image);
	//cvDestroyWindow("Test");
	//detach_ShareMem -> not in opencv
	//detach_ShareMem();
  return sum_time;
}

#define TRACK_FEATURES_TOT  1
#define TRACK_W	960
#define TRACK_H 540


class Track {
private:
	int thr_id;
	int tracked_features;
	Mat frame[2];
	int frame_idx;

	VideoCapture vid_cap;
	vector<Ptr<Tracker>> trackers;
	vector<Rect2d> rois;

	long int get_time_ms() {
		struct timeval tp;
		gettimeofday(&tp, NULL);
		long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
		return ms;
	}

public:
	Track() {
		
	}

	void init(int _popt, int _thr_id) {
		thr_id = _thr_id;
		tracked_features = (int)(TRACK_FEATURES_TOT / _popt);
		string fname;

		fname = "lane_" + to_string(_thr_id) + "a.png";
		cout << "thr: " << _thr_id <<" loading: " << fname << endl;
		frame[0] = imread(fname, CV_LOAD_IMAGE_COLOR);

		fname = "lane_" + to_string(_thr_id) + "b.png";
		cout << "thr: " << _thr_id <<" loading: " << fname << endl;
		frame[1] = imread(fname, CV_LOAD_IMAGE_COLOR);
		frame_idx = 0;



		// init trackers
		trackers = vector<Ptr<Tracker>>();
		for(int i = 0; i < tracked_features; i++) {
			// cout<<"creating tracker " << i << endl;

			// random region of interest
			int roi_x = 0;
			int roi_y = 0;
			int roi_w = 300;
			int roi_h = 300;
			Rect2d roi(roi_x, roi_y, roi_w, roi_h);
			rois.push_back(roi);

			Ptr<Tracker> tracker = TrackerKCF::create();
			trackers.push_back(tracker);
			// trackers[i]->init(frame, rois[i]);
			trackers[i]->init(frame[frame_idx % 2], rois[i]);
		}
	}

	void update() {
		long int t0 = get_time_ms();
		// vid_cap >> frame;
		for (int i = 0; i < tracked_features; i++) {
			trackers[i]->update(frame[frame_idx % 2], rois[i]);
		}
		frame_idx++;

		long int t1 = get_time_ms();

		cout << "id: " << thr_id << ", time spent: " << t1 - t0 << " ms" << endl;

		
		return;
	}
};

#endif
