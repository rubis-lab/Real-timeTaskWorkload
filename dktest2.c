// ./darknet detect cfg/yolov3-tiny.cfg yolov3-tiny.weights data/dog.jpg

#include "dktest2.h"

network * net[4];
list * options[4];
char * name_list[4];
char ** names[4];
int * indexes[4];
image im[4];
image r[4];

void init_predict_classifier(char *datacfg, char *cfgfile, char *weightfile, char *filename, int top, int thr_id, int num_thr)
{
    net[thr_id - 1] = load_network(cfgfile, weightfile, 0);
    set_batch_network(net[thr_id - 1], 1);
    srand(2222222);

    options[thr_id - 1] = read_data_cfg(datacfg);

    name_list[thr_id - 1] = option_find_str(options[thr_id - 1], "names", 0);
    if(!name_list[thr_id - 1]) name_list[thr_id - 1] = option_find_str(options[thr_id - 1], "labels", "/home/rtssae/darknet/data/labels.list");
    if(top == 0) top = option_find_int(options[thr_id - 1], "top", 1);

    int i = 0;
    names[thr_id - 1] = get_labels(name_list[thr_id - 1]);
    
    indexes[thr_id - 1] = calloc(top, sizeof(int));
    char buff[256];
    char *input = buff;
    

    strncpy(input, filename, 256);

    im[thr_id - 1] = load_image_color(input, 0, 0);
    r[thr_id - 1] = letterbox_image(im[thr_id - 1], net[thr_id - 1]->w, net[thr_id - 1]->h);

    // backup
    // network *net = load_network(cfgfile, weightfile, 0);
    // set_batch_network(net, 1);
    // srand(2222222);

    // list *options = read_data_cfg(datacfg);

    // char *name_list = option_find_str(options, "names", 0);
    // if(!name_list) name_list = option_find_str(options, "labels", "/home/rubis/darknet/data/labels.list");
    // if(top == 0) top = option_find_int(options, "top", 1);

    // int i = 0;
    // char **names = get_labels(name_list);
    // clock_t time;
    // int *indexes = calloc(top, sizeof(int));
    // char buff[256];
    // char *input = buff;
    
    // if(filename){
    //     strncpy(input, filename, 256);
    // }else{
    //     printf("Enter Image Path: ");
    //     fflush(stdout);
    //     input = fgets(input, 256, stdin);
    //     if(!input) return;
    //     strtok(input, "\n");
    // }
    // image im = load_image_color(input, 0, 0);
    // image r = letterbox_image(im, net->w, net->h);
    // //image r = resize_min(im, 320);
    // //printf("%d %d\n", r.w, r.h);
    // //resize_network(net, r.w, r.h);
    // //printf("%d %d\n", r.w, r.h);
    // float *predictions;
    // int inner_iteration[4] = {12, 6, 4, 3};
    // for(int i = 0; i < 4; i++) {
    //     inner_iteration[i] = inner_iteration[i] * SCALE_DARKNET;
    // }
}

long int darknet_get_time_ms() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    return ms;
}


int predict_classifier(char *datacfg, char *cfgfile, char *weightfile, char *filename, int top, int thr_id, int num_thr)
{
    float *predictions;
    int inner_iteration[4] = {1, 1, 1, 1};
    for(int i = 0; i < 4; i++) {
        inner_iteration[i] = inner_iteration[i] * SCALE_DARKNET;
    }

    long int t0 = darknet_get_time_ms();
    for(int i = 0; i < ITERATION_DARKNET; i++) {
        for(int j = 0; j < inner_iteration[num_thr - 1]; j++) {
            float *X = r[thr_id - 1].data;
            predictions = network_predict(net[thr_id - 1], X);
            if(net[thr_id - 1]->hierarchy) hierarchy_predictions(predictions, net[thr_id - 1]->outputs, net[thr_id - 1]->hierarchy, 1, 1);
            top_k(predictions, net[thr_id - 1]->outputs, top, indexes[thr_id - 1]);
        }
    }
    long int t1 = darknet_get_time_ms();

    //fprintf(stderr, "_DTCT finished in %ld ms.\n", t1 - t0);

    return t1 - t0;
    // for(i = 0; i < top; ++i){
    //     int index = indexes[i];
    //     //if(net->hierarchy) printf("%d, %s: %f, parent: %s \n",index, names[index], predictions[index], (net->hierarchy->parent[index] >= 0) ? names[net->hierarchy->parent[index]] : "Root");
    //     //else printf("%s: %f\n",names[index], predictions[index]);
    //     printf("%5.2f%%: %s\n", predictions[index]*100, names[index]);
    // }
    // if(r.data != im.data) free_image(r);
    // free_image(im);
}


//int main(int argc, char **argv)
int init_darknet_detect_thr_routine(int thr_id, int num_thr)
{
    init_predict_classifier("/home/rtssae/darknet/cfg/imagenet1k.data", 
        "/home/rtssae/darknet/cfg/yolov3-tiny.cfg", 
        "/home/rtssae/darknet/yolov3-tiny.weights", 
        "my_dog.jpg", 5, thr_id, num_thr);
}

int darknet_detect_thr_routine(int thr_id, int num_thr)
{
    // predict_classifier("darknetcfg/imagenet1k.data", argv[2], argv[3], argv[4], 5);
    return predict_classifier("/home/rtssae/darknet/cfg/imagenet1k.data", 
        "/home/rtssae/darknet/cfg/yolov3-tiny.cfg", 
        "/home/rtssae/darknet/yolov3-tiny.weights", 
        "my_dog.jpg", 5, thr_id, num_thr);
    // ./darknet detect cfg/yolov3-tiny.cfg yolov3-tiny.weights data/dog.jpg
}
 

