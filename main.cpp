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
		// configure threads
		struct thr_arg targ;
		struct thr_arg* targ_ptr = &targ;
		targ_ptr->strategy = strategy;
		targ_ptr->task_id = i;
		targ_ptr->parent = gettid();
		configure_task(targ_ptr, config, i);
		//cout << "Processing: task #: " << i << ", having " << targ_ptr->popt << " thrs." << endl;

		// create threads
		for(int j = 1; j <= targ_ptr->popt; j++){
			targ_ptr->thr_id = j;
			configure_thread(targ_ptr, config, i, j);
			struct thr_arg *arg = (struct thr_arg *) malloc(sizeof(*arg));

			cout << "Creating task_id: " << i << " thr_id: " << j << "/" << targ.popt << endl;

			// argument passed to thread
			*arg = targ;
			if (!arg) {
	            fprintf(stderr, 
	            	"[ERROR] Couldn't allocate memory for thread arg.\n");
	            exit(EXIT_FAILURE);
	        }
        
			pthread_create(&thread[thread_cnt], 0, run_deadline, arg);
			thread_cnt++;
		}
	}

	for(int i = 0; i < thread_cnt; i++) {
		pthread_join(thread[i], NULL);
	}
	cout << "main dies: " << gettid() << endl;

	return 0;
}	

