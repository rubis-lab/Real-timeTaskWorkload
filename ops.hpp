#ifndef __OPS_H__
#define __OPS_H__

#include <string>
#include <iostream>

#include "json.hpp"

using namespace std;
using json = nlohmann::json;


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



#endif
