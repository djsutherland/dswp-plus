#include "stdio.h"
#include <pthread.h>

int fun(int a) {
	//printf("%d\n", a);
	getchar();
	
	int i = 0, res = 200;
	//res = 0;
	//printf("begin loop\n");
	for (i = 0; i < a; i++) {
		//scanf("%d", &res);
		//getchar();
		//printf("%d %d\n", a, *((int *)a));
		printf("%d\n", i);
		//res = res + i;
		//break;
	}
	//printf("%d\n", 
	return res;
}

int main() {	
	int res;
	//printf("program start\n");
	res = fun(100);
	//printf("program end\n");
	return 0;
}
