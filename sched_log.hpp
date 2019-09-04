#ifndef __SCHED_LOG_H__
#define __SCHED_LOG_H__

#include <iostream>
#include <string>
#include <sstream>
#include <time.h>

using namespace std;

class SchedLog {
private:
	string fname;
	string thr_id;
	string tid;
	string runtime;
	string deadline;
	int deadline_raw;
	string period;
	int mode;
	ofstream out_file;

public:
	SchedLog(string _fname, int _strategy) {
		switch(_strategy) {
		case 0:
			fname = "cho-thread";
			break;
		case 1:
			fname = "single-thread";
			break;
		case 2:
			fname = "max-thread";
			break;
		case 3:
			fname = "random-thread";
			break;
		default:
			break;
		}
		
		fname += "-";
		fname += _fname;
		fname += ".txt";
	}

	void set_attr(int _id, int _tid, int _runtime, int _deadline, int _period, int _mode) {
		stringstream thr_id_str;
		thr_id_str << _id;
		thr_id = thr_id_str.str();

		stringstream tid_str;
		tid_str << _tid;
		tid = tid_str.str();

		stringstream runtime_str;
		runtime_str << _runtime;
		runtime = runtime_str.str();

		deadline_raw = _deadline;
		stringstream deadline_str;
		deadline_str << _deadline;
		deadline = deadline_str.str();

		stringstream period_str;
		period_str << _period;
		period = period_str.str();

		mode = _mode;
	}

	void create_header() {
		out_file.open(fname);
		if (out_file.is_open())
		{
			out_file << "mode " << mode << endl;
			out_file << "tid\titer\truntime\tdead\tperiod\tstart\tend\trun\tslack\trealrun" << endl;
		}
		return;
	}

	void append(int _iter, time_t _start_t, time_t _end_t, long int real_run_t) {
		stringstream start_t_str;
		start_t_str << _start_t;
		string start_t = start_t_str.str();

		stringstream end_t_str;
		end_t_str << _end_t;
		string end_t = end_t_str.str();
		
		signed long long real_runtime_raw = (signed long long)((_end_t - _start_t) * 1000 * 1000 / CLOCKS_PER_SEC);

		stringstream real_runtime_str;
		real_runtime_str << real_runtime_raw;
		string real_runtime = real_runtime_str.str();

		stringstream real_runtime_t_str;
		real_runtime_t_str << real_run_t;
		string real_runtime_t = real_runtime_t_str.str();

		string deadline_temp;

		signed long long slack_raw = (signed long long)((stoi(deadline) - (_end_t - _start_t)) * 1000 * 1000 / CLOCKS_PER_SEC);
		if(mode == 1){
			stringstream deadline_str;
			//deadline_str << stoi(deadline) - 700000;
			deadline_temp = deadline_str.str();
			//slack_raw = slack_raw - 700000;
		} else if(mode == 2){
			stringstream deadline_str;
			//deadline_str << stoi(deadline) - 300000;
			deadline_temp = deadline_str.str();
			//slack_raw = slack_raw - 300000;
		} else {
			deadline_temp = deadline;
		}


		stringstream slack_str;
		slack_str << slack_raw;
		string slack = slack_str.str();

		if (out_file.is_open())
		{
			out_file << gettid() << '\t' 
			<< _iter << '\t' 
			<< runtime << '\t'  
			<< deadline << '\t' 
			<< period << '\t' 
			<< start_t << '\t'  
			<< end_t << '\t' 
			<< real_runtime << '\t' 
			<< slack << '\t'
			<< real_runtime_t << endl;
		}

		return;
	}

	void close() {
		out_file.close();
		return;
	}
};

#endif
