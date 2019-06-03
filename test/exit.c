#include <stdio.h>
#include <stdlib.h>

int main(int argc, char ** argv, char **envp){
	int exitStatus;
	sscanf(argv[1], "%d", &exitStatus);
	sleep(3);
	exit(exitStatus);
}