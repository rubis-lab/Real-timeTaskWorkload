#include <pthread.h>
#include <unistd.h>

#include <iostream>
#include <fstream>

// open-source libraries
#include "sched_deadline.hpp"
#include "json.hpp"

// custom libraries
#include "thr_routine.hpp"
#include "ops.hpp"

using namespace std;
using json = nlohmann::json;

pthread_t thread[16];

int main(int argc, char **argv)
{
	cout << "main thread id: " << gettid() << endl;

	// parse config file
	json config;
	ifstream input_file("config.json");
	input_file >> config;

	int strategy = get_strategy(config);
	int num_task = get_num_task(config);

	int thread_cnt = 0;
	for(int i = 1; i <= num_task; i++) {
			pthread_create(&thread[thread_cnt], 0, run_deadline, arg);
			thread_cnt++;
	}

	for(int i = 0; i < thread_cnt; i++) {
		pthread_join(thread[i], NULL);
	}
	cout << "main dies: " << gettid() << endl;

	return 0;
}	

