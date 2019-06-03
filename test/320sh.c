#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#include <sys/types.h>
#include <sys/wait.h>

#define clear() printf("\033[H\033[J")
#define gotoxy(x,y) printf("\033[%d;%dH", (x), (y))

// Assume no input line will be longer than 1024 bytes
#define MAX_INPUT 1024

int testFork();

int 
main (int argc, char ** argv, char **envp) {

	int finished = 0;
	char *prompt = "320sh> ";
	char cmd[MAX_INPUT];
	
	static struct termios oldtio, newtio;
	tcgetattr(0, &oldtio);
	newtio = oldtio;
	newtio.c_lflag |= ISIG;
	tcsetattr(0, TCSANOW, &newtio);

	while (!finished) {
		char a;
		char *cursor = &a;
		char last_char;
		int rv;
		int count;
		

		// Print the prompt
		rv = write(1, prompt, strlen(prompt));
		if (!rv) { 
			finished = 1;
			printf("breaked\n");
			break;
		}
	    
		// read and parse the input
		for(rv = 1, count = 0, cursor = cmd, last_char = 1;
			rv && (++count < (MAX_INPUT-1)) && (last_char != '\n');
			cursor++) { 
			rv = read(0, cursor, 1);
			last_char = *cursor;
			if(last_char == 3) {
				write(1, "^c", 2);
			}else if(last_char == 26){
				write(1, "^z", 2);
			}else{
				write(1, &last_char, 1);
			}
		}
		*cursor = '\0';

		if (!rv) { 
			finished = 1;
			printf("breaked\n");
			break;
		}
		
		if(strcmp(cmd, "exit\n") == 0){
			tcsetattr(0, TCSANOW, &oldtio);
			exit(EXIT_SUCCESS);
		}else if(strcmp(cmd, "test_fork\n") == 0){
			testFork();
		}

		// Execute the command, handling built-in commands separately 
		// Just echo the command line for now
		// write(1, cmd, strnlen(cmd, MAX_INPUT));
	}
	return 0;
}

int testFork(){
	pid_t pid = 0;
	int status = 0;
	
	if((pid = fork()) == 0){
		//child
		execl("./infinity_loop", "infinity_loop", NULL);
	}
	
	printf("parent started waiting\n");
	waitpid(pid, &status, 0);
	printf("child interrupted");
	
	return 0;
}