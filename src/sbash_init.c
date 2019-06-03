/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include "sbash_init.h"
#include "sbash_debug.h"
#include "sbash_editing.h"

#include "sbash_module_job_control.h"
#include "sbash_module_variable.h"
#include "sbash_module_history.h"

static struct termios oldTerm, defaultTerm;


void do_nothing_handler(int sig){
//	write(STDOUT_FILENO, "\n", 1);
//	signal(sig, do_nothing_handler);
}

int init (int argc, char ** argv, char **envp) {
     /* for(int i=0;i<argc; i++){
      printf("my %dth argument is %s\n",i,*(argv+i));
  }*/



	DEBUG_FLAG = 0;
	TIME_FLAG = 0;
	int opt;
	while((opt = getopt(argc, argv, "td")) != -1) {
        switch(opt) {
		case 't':
			TIME_FLAG = 1;
			break;
		case 'd':
			DEBUG_FLAG = 1;
			break;
		case '?':
			// Let this case fall down to default;
			// handled during bad option.
		default:
			// A bad option was provided. 
			error("-t:\tdisplay time after finishing execution\n"
					"-d\tprint debug info\n");
			exit(EXIT_FAILURE);
			break;
        }
    }
	
	// checking if the shell is foreground
#ifndef DEBUG
	while(tcgetpgrp(STDIN_FILENO) != getpgrp()){
		error("Shell is not a foreground program, stopped. \n");
		kill(-getpgrp(), SIGTTIN); 
	}
#endif	
	// setup terminal environment
	tcgetattr(STDIN_FILENO, &oldTerm);
	defaultTerm = oldTerm;
	defaultTerm.c_lflag |= ISIG;
	defaultTerm.c_lflag &= ~(ECHO|ECHOE|ECHOK|ECHONL|ICANON);
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &defaultTerm) <0 ){
		error("fail to setup terminal environment, errno:%d. \n", errno);
		cleanUp(EXIT_FAILURE);
	}
	
	// setup signal blocking
	sigset_t defaultSigMask;
	sigfillset(&defaultSigMask);
	//sigaddset(&defaultSigMask, SIGINT);
	if(sigprocmask(SIG_UNBLOCK, &defaultSigMask, NULL) <0 ){// override the default sig mask
		error("fail to setup signal blocking, errno:%d. \n", errno);
		cleanUp(EXIT_FAILURE);
	}
	int i;
	for(i=1; i<32; i++) signal(i, SIG_DFL);// reset all handler
//	signal(SIGINT, do_nothing_handler);
	struct sigaction newAction;
	newAction.sa_handler = do_nothing_handler;
	sigemptyset(&newAction.sa_mask);
	newAction.sa_flags = 0;
	sigaction(SIGINT, &newAction, NULL);
	newAction.sa_handler = SIG_IGN;
	sigaction(SIGTSTP, &newAction, NULL);
//	signal(SIGINT, SIG_IGN);
	
	// set itself as the foreground process leader
	if(tcsetpgrp(STDIN_FILENO, getpid()) <0 || setpgid(0, getpid()) <0 ){
		error("fail to set itself as the foreground process leader, errno:%d. \n", errno);
#ifndef DEBUG
		cleanUp(EXIT_FAILURE);
#endif
	}
	
	// initialize modules
	init_JobControl();
	variable_init();
	
	// shell basic initialization complete
	// now initialize modules
	history_initAndLoad(HISTORY_LOG_PATH);
	
	
	// now enter editing mode
	debug("current pid:%d pgid:%d\n", getpid(), getpgrp());
	prompt(envp);
//	prompt_test(envp);
	// temp test code
	
	
	// exit 320sh shell
	// now clean up
	cleanUp(EXIT_SUCCESS);
	return 0;
}

void cleanUp(int exitStatus){
	// revert the terminal environment
	tcsetattr(0, TCSANOW, &oldTerm);
	
	history_save(HISTORY_LOG_PATH);
	history_clean();
	cleanUp_jobControl();
	
	exit(exitStatus);
}
