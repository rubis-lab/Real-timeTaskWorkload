#include <unistd.h>

#include <iostream>
#include <fstream>

using namespace std;

int main(int argc, char **argv)
{
	cout << "main thread id: " << gettid() << endl;

	return 0;
}	

