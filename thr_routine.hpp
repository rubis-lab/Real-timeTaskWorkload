#ifndef __THR_ROUTINE_H__
#define __THR_ROUTINE_H__

#define ITER_DEFAULT 20

#include <pthread.h>

#include <iostream>

#include "sched_deadline.hpp"

#include "act.hpp"
#include "track.hpp"
//#include "dktest2.h"

extern "C" int init_darknet_detect_thr_routine(int thr_id, int num_thr);
extern "C" int darknet_detect_thr_routine(int thr_id, int num_thr);

pthread_barrier_t init_barrier;

using namespace std;

struct thr_arg {
	int task_id;
	int thr_id;
	int popt;
	int mode;
	int exec_time;
	int period;
	int deadline;
	int parent;
	int strategy;
};

enum thr_workload_mode {
	_CAM, _TRCK, _DTCT, _ACT
};

struct sched_attr configure_attr(struct thr_arg targ) {
	// set rt-sched attribute
	struct sched_attr attr;
	attr.size = sizeof(attr);
	attr.sched_flags = 0;
	attr.sched_nice = 0;
	attr.sched_priority = 0;

	attr.sched_policy = SCHED_DEADLINE; // 6
	attr.sched_runtime = targ.exec_time;
	attr.sched_deadline = targ.deadline;
	attr.sched_period = targ.period; 

	return attr;
}

void print_thr_config(struct thr_arg thr_config) {
	cout << "        " << " ";
	cout << "task_id: " << thr_config.task_id << " ";
	cout << "thr_id: " << thr_config.thr_id << " ";
	cout << "mode: " << thr_config.mode << " ";
	cout.width(12);
	cout << "exec_time: " << thr_config.exec_time << " ";
	cout.width(12);
	cout << "deadline: " << thr_config.deadline << " " << endl;
	return;
}

void *run_deadline(void *data) {
	// unpack argument
	struct thr_arg thr_config = *((struct thr_arg *) data);
    free(data);
    print_thr_config(thr_config);

    // init workloads
	Track track;
	switch(thr_config.mode){
		case _DTCT: //object_detection 
			init_darknet_detect_thr_routine(thr_config.thr_id, thr_config.popt);
			break;
		default:
			break;
	}


	// configure thread attributes
	struct sched_attr attr = configure_attr(thr_config);
	int ret = sched_setattr(0, &attr, attr.sched_flags);
	if(ret < 0) {
		cout << "[ERROR] sched_setattr failed." << endl;
		perror("sched_setattr");
		exit(-1);
	}
	
	long int real_runtime;
	for(int iter = 0; iter < ITER_DEFAULT; iter++) {
		// thread workload
		switch(thr_config.mode){
			case _CAM: //sensor_read_camera_module
				real_runtime = actuator(thr_config.exec_time, thr_config.popt);
				//cout << "_CAM is finished in " << real_runtime << " ms." << endl;
				break;
			case _TRCK: //lane_tracking
				// track.update();
				real_runtime = autoware_vision_thr_routine(thr_config.popt);
				//cout << "_TRCK is finished in " << real_runtime << " ms." << endl;
				break;
			case _DTCT: //object_detection 
				real_runtime = darknet_detect_thr_routine(thr_config.thr_id, thr_config.popt);
				//cout << "_DTCT is finished in " << real_runtime << " ms." << endl;
				break;
			case _ACT: //actuator_and_stirring
				real_runtime = actuator(thr_config.exec_time, thr_config.popt);
				//cout << "_ACT is finished in " << real_runtime << " ms." << endl;
				break;
			default:
				printf("[ERROR] The mode has incorrect value.\n");
				exit(-1);
		}

		sched_yield();
	}

	return NULL;
}

#endif
