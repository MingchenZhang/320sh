#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char ** argv, char **envp){
	int pid;
//	sscanf(argv[1], "%d", &pid);
	if((pid = fork()) == 0){
		execlp("./exit", "./exit", "3", NULL);
	}
	while(1){
		int status = 0;
		int result = waitpid(pid, &status, WNOHANG | WUNTRACED);
		printf("\n");
		printf("return:%d\n", result);
		printf("WIFEXITED:%d\n", WIFEXITED(status));
		printf("WIFEXITED:%d\n", WEXITSTATUS(status));
		printf("WIFSIGNALED:%d\n", WIFSIGNALED(status));
		printf("WIFSTOPPED:%d\n", WIFSTOPPED(status));
		sleep(10);
	}
}
