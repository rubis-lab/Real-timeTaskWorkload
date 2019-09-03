#ifndef __OPS_H__
#define __OPS_H__

#include <string>
#include <iostream>

#include "thr_routine.hpp"
#include "json.hpp"
#include "sched_deadline.hpp"

using namespace std;
using json = nlohmann::json;

char* itoa(int val, int base){
	
	static char buf[32] = {0};
	
	int i = 30;
	
	for(; val && i ; --i, val /= base)
	
		buf[i] = "0123456789abcdef"[val % base];
	
	return &buf[i+1];
	
}

char* lltoa(signed long long val, int base){
	
	static char buf[32] = {0};
	
	int i = 30;
	
	for(; val && i ; --i, val /= base)
	
		buf[i] = "0123456789abcdef"[val % base];
	
	return &buf[i+1];
	
}

int get_strategy(json cfg) {
	string str;
	cfg.at("strategy").get_to(str);
	int strategy;

	if (str.compare("cho") == 0) {
		strategy = 0;
	} else if (str.compare("single") == 0) {
		strategy = 1;
	} else if (str.compare("max") == 0) {
		strategy = 2;
	} else if (str.compare("random") == 0) {
		strategy = 3;
	} else {
		cout << "[ERROR] The input strategy is not correct!!" << endl;
		exit(EXIT_FAILURE);
	}
	cout << "strategy : " << strategy << '\t'; 

	return strategy;
}

int get_num_task(json cfg) {
	int num_task = atoi(cfg["num_task"].dump().substr(1,-1).c_str());
    cout << "num_task : " << num_task << endl << endl;

    return num_task;
}

void configure_task(struct thr_arg* targ, json cfg, int task_id) {
	targ->popt = atoi(cfg[itoa(task_id,10)]
		["popt"].dump().substr(1,-1).c_str());
	if(!targ->popt){
		cout << "[ERROR] popt not exists in config.json" << endl;
		exit(EXIT_FAILURE);
	}

	targ->mode = atoi(cfg[itoa(task_id,10)]
		["mode"].dump().substr(1,-1).c_str());

	targ->deadline = atoi(cfg[itoa(task_id,10)]
		["deadline"].dump().substr(1,-1).c_str());
	if(!targ->deadline){
		cout << "[ERROR] deadline not exists in config.json" << endl;
		exit(EXIT_FAILURE);
	}
	
	targ->period = atoi(cfg[itoa(task_id,10)]
		["period"].dump().substr(1,-1).c_str());
	if(!targ->period){
		cout << "[ERROR] period not exists in config.json" << endl;
		exit(EXIT_FAILURE);
	}
	return;
}

void configure_thread(struct thr_arg* targ, json cfg, int task_id, int thr_id) {

	targ->exec_time = atoi(cfg[itoa(task_id,10)]
		["exec_time"][itoa(thr_id,10)].dump().substr(1,-1).c_str());
	if(!targ->exec_time){
		cout << "[ERROR] exec_time not exists in config.json" << endl;
		exit(EXIT_FAILURE);
	}

	return;
}


#endif
