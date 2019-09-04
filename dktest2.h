#ifndef __DARKNET_OBJECT_H__
#define __DARKNET_OBJECT_H__
#include "darknet.h"
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>

#define ITERATION_DARKNET 1
#define SCALE_DARKNET 1

int init_darknet_detect_thr_routine(int thr_id, int num_thr);
int darknet_detect_thr_routine(int thr_id, int num_thr);

#endif //__DARKNET_OBJECT_H__
