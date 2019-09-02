#include <unistd.h>

#include <iostream>
#include <fstream>

// open-source libraries
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

int main(int argc, char **argv)
{
	cout << "main thread id: " << gettid() << endl;

	// parse config file
	json config;
	ifstream input_file("config.json");
	input_file >> config;

	return 0;
}	

