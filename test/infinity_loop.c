#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char ** argv, char **envp){
	printf("current pid:%d pgid:%d\n", getpid(), getpgrp());
	time_t t;
	srand((unsigned) time(&t));
	int random = rand() % 1000;
	while(1){
		sleep(10);
		printf("\"%d\"", random);
		fflush(stdout);
	}
	return 0;
}
