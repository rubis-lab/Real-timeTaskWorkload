#ifndef __THR_ROUTINE_H__
#define __THR_ROUTINE_H__

#include <pthread.h>

#include <iostream>

#include "sched_deadline.hpp"

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

	return NULL;
}

#endif
