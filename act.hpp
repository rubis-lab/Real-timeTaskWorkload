#ifndef __ACT_H__
#define __ACT_H__

#include <time.h>

int actuator(int exec_time, int popt) {
	int y = 0;
	int workload = (int)80 * exec_time / popt;
	
	clock_t start_t, end_t;
	start_t = clock();
/*
	int i, j, m, n;
	m = (int)100/popt;
	n = 10000;
	int A[m][n];
	int B[n][m];
	for (i = 0; i< m; i++) {
		for (j = 0; j < n; j++) {
			A[i][j] = i+j;
		}
	}

			
	for (i = 0; i < n; i++) {
		for (j = 0; j < m; j++) {
			B[i][j] = A[j][i];
			while(y < workload){
				y++;
			}
		}
	}
*/
	while(y < workload){
		y++;
	}


	end_t = clock();
	long int real_runtime =  (long int)((end_t - start_t) * 1000 * 1000 / CLOCKS_PER_SEC);
	//printf("_ACT finished in %d ms.\n", real_runtime);
	return real_runtime;
}
#endif
