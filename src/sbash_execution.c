/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */


#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include <termios.h>
#include <signal.h>

#include <sys/stat.h> 
#include <fcntl.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "sbash_debug.h"
#include "sbash_execution.h"
#include "sbash_module_job_control.h"
#include "util_array_list.h"
#include "sbash_module_variable.h"
#include "sbash_init.h"

char* JobStateStr[8] = {"TOBE_FOREGROUND","TOBE_BACKGROUND","FOREGROUND","BACKGROUND","STOPPED","JOB_FINFISH","SIGNALLED","EXIT"};
char* JobStateStrGet(JobState js) {return JobStateStr[js];}

int executeJob(Job* job){
	// check file existence
	Process* procCursor = job->first_process;
	while(procCursor != NULL){
		if(!isExeExist(procCursor->path)){
			error("file %s does not exist or not executable. \n", procCursor->path);
			return -1;
		}
		procCursor = procCursor->next;
	}
	//todo: existence check 
	// first block all signal
	sigset_t oldMask, newMask;
	sigfillset(&newMask);
	sigprocmask(SIG_BLOCK, &newMask, &oldMask);
	// save terminal setting
	struct termios oldTerm;
	tcgetattr(0, &oldTerm);
	// setup terminal environment for job if it is foreground
	if(job->state == TOBE_FOREGROUND)
		tcsetattr(STDIN_FILENO, TCSANOW, &(job->tmodes));
	
	// start fork
	pid_t firstProcess = -1;
	procCursor = job->first_process;
	while(procCursor != NULL){
		// check if this is a builtin command
		int builtin = builtinAction(procCursor, 0);
		if(builtin >=0 ){
			// goto next
			procCursor->state = P_COMPLETE;
			procCursor = procCursor->next;
			sprintf(QuestionMark.value, "%d", builtin);
			continue;
		}
		// stdin/stdout redirection support
		if(procCursor->redirect.in_proc != NULL ){
			if(procCursor->redirect.in_fd <=0){
				int pipePair[2];
				if(pipe(pipePair) == -1){
					error("cannot create pipe for %s", procCursor->path);
					continue;
				}
				debug("redirection pipe created(1): %d to %d\n", pipePair[1], pipePair[0]);
				procCursor->redirect.in_fd = pipePair[0];
				procCursor->redirect.in_proc->redirect.out_fd = pipePair[1];
			}else
				debug("redirection pipe has been established\n");
		}
		if(procCursor->redirect.out_proc != NULL ){
			if(procCursor->redirect.out_fd <=0){
				int pipePair[2];
				if(pipe(pipePair) == -1){
					error("cannot create pipe for %s", procCursor->path);
					continue;
				}
				debug("redirection pipe created(2): %d to %d\n", pipePair[1], pipePair[0]);
				procCursor->redirect.out_fd = pipePair[1];
				procCursor->redirect.out_proc->redirect.in_fd = pipePair[0];
			}else
				debug("redirection pipe has been established\n");
		}
		
		pid_t pid;
		if((pid = fork()) == 0){//child
			if(firstProcess == -1){
				// this is the process leader
				firstProcess = getpid();
			}
			// put itself in job process group
			setpgid(getpid(), firstProcess);
			// put itself into foreground if needed
			if(job->state == TOBE_FOREGROUND){
				tcsetpgrp(STDIN_FILENO, firstProcess);
			}
			// restore signal block
			sigprocmask(SIG_SETMASK, &oldMask, NULL);
			// reset all signal handler
			int i;
			for(i=1; i<32; i++) signal(i, SIG_DFL);// reset all handler
			
			// file redirection support
			if(procCursor->redirect.in_file_arrayList != NULL ){// has input file
				int** in_array = &(procCursor->redirect.in_file_arrayList);
				int i;
				for(i=0; i< arrayList_getSize(in_array); i++){
					FileRedirectPair* pair = ((FileRedirectPair*)arrayList_get(i, in_array));
					int fd = open(pair->file, O_RDONLY);
					if(fd == -1){
						error("open file:%s failed\n", pair->file);
						exit(EXIT_FAILURE);
					}
					if(dup2( fd, pair->process_fd) == -1){
						error("dup2 failed\n");
						exit(EXIT_FAILURE);
					}
					debug2e("process_fd:%d read file %s\n", pair->process_fd, pair->file);
				}
			}
			if(procCursor->redirect.out_file_arrayList != NULL ){// has output file 
				int** out_array = &(procCursor->redirect.out_file_arrayList);
				int i;
				for(i=0; i< arrayList_getSize(out_array); i++){
					FileRedirectPair* pair = ((FileRedirectPair*)arrayList_get(i, out_array));
					int fd = open(pair->file, O_WRONLY|O_CREAT|O_TRUNC , S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
					if(fd == -1){
						error("open file:%s failed\n", pair->file);
						exit(EXIT_FAILURE);
					}
					if(dup2( fd, pair->process_fd) == -1){
						error("dup2 failed\n");
						exit(EXIT_FAILURE);
					}
					debug2e("process_fd:%d write file %s\n", pair->process_fd, pair->file);
				}
			}
			// pipe redirection support
			if(procCursor->redirect.in_fd >0 ){// stdin from a process
				dup2(procCursor->redirect.in_fd, STDIN_FILENO);
				debug2e("process:%s(fd:%d) read from pipe %d\n", procCursor->path, procCursor->redirect.in_fd, procCursor->redirect.in_fd);
			}
			if(procCursor->redirect.out_fd >0 ){// stdout to a process
				dup2(procCursor->redirect.out_fd, STDOUT_FILENO);
				debug2e("process:%s(fd:%d) write to pipe %d\n", procCursor->path, procCursor->redirect.out_fd, procCursor->redirect.out_fd);
			}
			
			// print some debug info
#ifdef DEBUG
			fprintf(stderr, "execute %s with following arguments\n", procCursor->path);
			for(i=0; (procCursor->argv)[i]!=NULL; i++){
				fprintf(stderr, "%s,", (procCursor->argv)[i]);
			}
			fprintf(stderr, "\n");
#endif
			
			debug2e("builtin = %d\n", builtin);
			if(builtin == -2){// is complex builtin
				debug2e("complex builtin executing\n");
				if((builtin = builtinAction(procCursor, 1)) < 0) cleanUp(EXIT_FAILURE);
				else cleanUp(builtin);
			}
			
			// todo: complex builtin, use builtin var
			// execute the program
			if(!isPath(procCursor->path)){
				if(execvp(procCursor->path, procCursor->argv) <0){
					error("fail to execute %s. errno: %d\n", procCursor->path, errno);
					cleanUp(EXIT_FAILURE);
				}
			}else if( execve(procCursor->path, procCursor->argv, procCursor->envp) <0){
				error("fail to execute %s. errno: %d\n", procCursor->path, errno);
				cleanUp(EXIT_FAILURE);
			}
		}
		debug("new process:%d\n", pid);
		// close all related file descriptor
		close(procCursor->redirect.out_fd);
		close(procCursor->redirect.in_fd);
		
		if(firstProcess == -1){
			// this child is process leader
			firstProcess = pid;
		}
		// put this child in job process group
		setpgid(pid, firstProcess);
		// put this child into foreground if needed
		if(job->state == TOBE_FOREGROUND){
			tcsetpgrp(STDIN_FILENO, firstProcess);
		}
		// change process state
		procCursor->state = P_RUNNING;
		procCursor->pid = pid;
		// goto next
		procCursor = procCursor->next;
	}
	// set job pgid
	job->pgid = job->first_process->pid;
	// change job state
	if(job->state == TOBE_FOREGROUND) job->state = FOREGROUND;
	else if(job->state == TOBE_BACKGROUND) job->state = BACKGROUND;
	else error("error: 85663\n");
	// add job to jobList
	job = addJob(job);
	// print background job
	if(job->state == BACKGROUND) printAJob(job-JOB_LIST);
	// restore signal block
	sigprocmask(SIG_SETMASK, &oldMask, NULL);
	
	// wait if foreground
	if(job->state == FOREGROUND) waitForegroundJob(job);
	// restore terminal state
	tcflush(STDIN_FILENO, TCIFLUSH);
	tcsetattr(STDIN_FILENO, TCSANOW, &oldTerm);
	
	return 0;
}

int foregroundJob(int index){
	Job* job = getJob(index);
	if(job->state == BACKGROUND_STOP || job->state == BACKGROUND){
		// save terminal setting
		struct termios oldTerm;
		tcgetattr(0, &oldTerm);
		// restore saved terminal state
		tcsetattr(STDIN_FILENO, TCSANOW, &(job->tmodes));
		// put the group back to foreground
		tcsetpgrp(STDIN_FILENO, job->pgid);
		//resume job (in case job stopped)
		kill(-job->pgid, SIGCONT);
		// change job state
		job->state = FOREGROUND;
		// start waiting
		int waiterror;
		if( (waiterror = waitForegroundJob(job) ) < 0){
			error("error code:%d", waiterror);
			return -1;
		}
		// restore shell terminal state
		tcflush(STDIN_FILENO, TCIFLUSH);
		tcsetattr(STDIN_FILENO, TCSANOW, &oldTerm);
		return 0;
	}else{
		error("job state abnormal, state:%s "
				"(should be BACKGROUND_STOP OR BACKGROUND)\n", JobStateStrGet(job->state));
		return -1;
	}
}




int waitForegroundJob(Job* job){
	int result = 0;	
	if(job->state != FOREGROUND){
		error("not foreground job\n");
		return -1;
	}
	// start waiting for foreground job
	int waitResult = -2;
	waitResult = waitForJob(job);
	if(waitResult == 1) job->state = BACKGROUND_STOP;
	else if(waitResult == 0) job->state = JOB_FINFISH;
	// save stopped job terminal state if it is stopped
	if(waitResult == 1)
		tcgetattr(STDIN_FILENO, &(job->tmodes));
	// set shell back to foreground
	{
		struct sigaction newAction, oldAction;
		newAction.sa_handler = SIG_IGN;
		sigemptyset(&newAction.sa_mask);
		newAction.sa_flags = 0;
		sigaction(SIGTTOU, &newAction, &oldAction);
		if( tcsetpgrp(STDIN_FILENO, getpgid(0)) != 0 ){
			error("tcsetpgrp failed. \n");
			result = -1;
		}
		sigaction(SIGTTOU, &oldAction, NULL);
	}	
	// remove job if it is terminated
	if(waitResult == 0) removeJob(job);
	
	if((waitResult == 0 || waitResult == 1) && result >=0) return waitResult;
	else return result;
}

int waitForJob(Job* job){
	int status = -1;
	int stopped = 0;
	if(job->state == FOREGROUND){
		Process* waitCursor = job->first_process;
		while(waitCursor != NULL){
			if(waitCursor->pid != 0 && waitCursor->state != P_COMPLETE){ // only wait when it can
				waitpid(waitCursor->pid, &status, WUNTRACED);
				if(WIFSTOPPED(status)){
					waitCursor->state = P_STOP;
					stopped = 1;
				}else{
					waitCursor->state = P_COMPLETE;
					sprintf(QuestionMark.value, "%d", WEXITSTATUS(status));
					//debug("status code:%s\n", (char*)QuestionMark.value);
					debug2e("ENDED:%s(ret=%s)\n", job->command, (char*)QuestionMark.value);
				}
			}else{
				waitCursor->state = P_COMPLETE;
			}
			waitCursor = waitCursor->next;
		}
	}else{
		error("error no: 324345");
		stopped = -1;
	}
	
	return stopped;
}

int backgroundJob(int index){
	Job* job = getJob(index);
	if(job->state == BACKGROUND_STOP){
		if(job->pgid>0) killpg(job->pgid, SIGCONT);
		Process* procCursor = job->first_process;
		while(procCursor!=NULL && procCursor->pid > 0){
			if(procCursor->state != P_COMPLETE &&
					procCursor->state != P_INTERRUPTTED &&
					procCursor->state != P_STOP) 
				procCursor->state = P_RUNNING;
			procCursor = procCursor->next;
		}
		job->state = BACKGROUND;
		return 0;
	}else{
		error("job state abnormal, state:%s (should be BACKGROUND_STOP)\n", JobStateStrGet(job->state));
		return -1;
	}
}

int isPath(char* str){
	int i;
	for(i=0; str[i]!='\0'; i++){
		if(str[i] == '/') return 1;
	}
	return 0;
}

int isExeExist(char* path){
	if(isPath(path)){
		return isExecutable(path);
	}else if(isBuiltin(path)){
		return 1;
	}else if(isInSystemPathAndExecuable(path)){
		return 1;
	}else 
		return 0;
}

int isInSystemPathAndExecuable(char* fileName){
	char* envPath = getenv("PATH");
	size_t pathSize = strlen(envPath)+1;
	char* newPath = malloc(pathSize);
	strcpy(newPath, envPath);
	
	char* strtokPtr;
	char* oriOnePath;
	char* path = newPath;
	while((oriOnePath = strtok_r(path, ":", &strtokPtr)) != NULL){
		path = NULL;
		//debug("checking %s\n", oriOnePath);
		char* newOnePath = malloc(strlen(oriOnePath)+strlen(fileName)+2);
		strcpy(newOnePath, oriOnePath);
		strcat(newOnePath, "/");
		strcat(newOnePath, fileName);
		//debug("checking %s\n", newOnePath);
		int result = isExecutable(newOnePath);
		free(newOnePath);
		if(result) {free(newPath);return 1;}
	}
	free(newPath);
	return 0;
}

int isExecutable(char* path){
	struct stat procStat;
	if(stat(path, &procStat) != 0){
		//error("file %s does not exist\n", procCursor->path);
		return 0;
	}
	if (procStat.st_uid == geteuid()) return procStat.st_mode & S_IXUSR;
	if (procStat.st_gid == getegid()) return procStat.st_mode & S_IXGRP;
	return procStat.st_mode & S_IXOTH;
}

int isBuiltin(char* cmd) {
    if (strcmp(cmd, "cd") == 0) {
        // it is a cd command 
        return 1;
    } else if (strcmp(cmd, "exit") == 0) {
        //  it is a exit command
        return 1;
    } else if (strcmp(cmd, "echo") == 0) {
        // it is a echo command
        return 1;
    } else if (strcmp(cmd, "set") == 0) {
        // it is a set command
        return 1;
    } else if (strcmp(cmd, "jobs") == 0) {
        // it is a jobs command
        return 1;
    } 
    else if (strcmp(cmd, "bg") == 0) {
        // it is a bg  command
        return 1;
    } 
    else if (strcmp(cmd, "fg") == 0) {
        // it is a fg command
        return 1;
    } 
    else if (strcmp(cmd, "help") == 0) {
        // it is a jobs command
        return 1;
    } 
	else if(strcmp(cmd, "kill")==0){
	    return 1;
	   } 
	else if (strcmp(cmd, "pwd")==0){
		return 1;
	}
    else
        return 0;
}

int builtinAction(Process* proc, int runningComplex){
	char*argument = *(proc->argv);
	int number_of_arguments = 0;
	char** args = proc->argv;
	while (*args != NULL) {
		args = args + 1;
		number_of_arguments = number_of_arguments + 1;
	}
	char absolute_directory[100];
	getcwd(absolute_directory, 1000);
	// first check if it is build in  
	if (!isBuiltin(argument)) {
		return -1;
	} else if (!runningComplex) { // if flag is false will return -2 because of the redirection 
		if (arrayList_getSize(&(proc->redirect.in_file_arrayList)) != 0 || arrayList_getSize(&(proc->redirect.out_file_arrayList))!= 0 
				|| proc->redirect.in_proc != NULL || proc->redirect.out_proc != NULL)
			return -2;
	}
	// it is buildin and not redirection 
	if (strcmp(argument, "cd") == 0) {
		int return_value = 1;
		if (number_of_arguments == 1) {
			char home_directory[1000];
			char * home_directory_pointer = getenv("HOME");
			strcpy(home_directory, home_directory_pointer);
			return_value = changeDirectory(home_directory);
		} else if (number_of_arguments == 2 && (strcmp((*(proc->argv + 1)), "-") == 0)) {
			char previous_directory[1000];
			char* old_directory = getenv("OLDPWD");
			strcpy(previous_directory, old_directory);
			printf("%s\n", previous_directory);
			return_value = changeDirectory(previous_directory);
		} else if (number_of_arguments == 2 && *(proc->argv + 1) != NULL) {
			struct stat *directory_buf = malloc(sizeof(struct stat));
			if (stat(*(proc->argv + 1), directory_buf) != 0) return 2;
			if ((directory_buf->st_mode & S_IFMT) != S_IFDIR) {
				return 2;
			}
			free(directory_buf);
			char new_directory[1000];
			char concatenate_directory[1000];
			strcpy(new_directory, *(proc->argv + 1));
			char first_character = *(new_directory + (int) strlen(new_directory) - 1);
			if (first_character != '/') {
				strcpy(concatenate_directory, absolute_directory);
				strcat(concatenate_directory, "/");
				strcat(concatenate_directory, new_directory);
				strcpy(new_directory, concatenate_directory);
			}
			return_value = changeDirectory(new_directory);
		}
		return return_value;
	} else if (strcmp(argument, "exit") == 0) {
		cleanUp(EXIT_SUCCESS);
		return 0;
	} else if (strcmp(argument, "echo") == 0) {
		if (number_of_arguments == 1) {
			fprintf(stdout, "\n");
		} else {
			int i;
			for (i = 1; i < number_of_arguments; i++) {
				fprintf(stdout, "%s ", *(proc->argv + i));
			}
			fprintf(stdout, "\n");
		}
		return 0;
	} else if (strcmp(argument, "set") == 0) {
		if(number_of_arguments==1){
			return 2;
		}
		char * token;
		char * strtokPtr;
		char * concatenated_set_string = malloc(1000);
		strcpy(concatenated_set_string, *(proc->argv + 1));
		int i;
		for (i = 2; i < number_of_arguments; i++) {
			strcat(concatenated_set_string, *(proc->argv + i));
		}
		char * token1 = malloc(100);
		char * token2 = malloc(100);
		token = strtok_r(concatenated_set_string, "=", &strtokPtr);
		concatenated_set_string = NULL;
		strcpy(token1, token);
		token = strtok_r(concatenated_set_string, "=", &strtokPtr);
		strcpy(token2, token);
		//check if token1 and token2 start with $ and is alphabatic
		variable_set(token1, token2);
		free(concatenated_set_string);
		free(token1);
		free(token2);
		return 0;
	} else if (strcmp(argument, "jobs") == 0) {
		printAllJob();
		return 0;
	} else if (strcmp(argument, "pwd") == 0) {
		printf("%s\n", absolute_directory);
		return 0;
	} else if (strcmp(argument, "help") == 0) {
		printf("cd [-L|[-P [-e]] [-@]] [dir] : change directory\n");
		printf("echo [-neE] [arg ...] : print strings and expand environment variables\n");
		printf("jobs: printout the jobs their name, status and pid\n");
		printf("fg [job_spec] : make job to be foreground \n");
		printf("bg [job_spec] : make job to be background \n");
		printf("set [-abefhkmnptuvxBCHP] [-o option->  : set environment variable value and create new ones\n");
		printf("pwd [-LP]: print out the current directory\n");
		printf("exit [n]: exit the shell\n");
		printf("help: [-dms] [pattern ...]\n");
		return 0;
	} else if (strcmp(argument, "fg") == 0) {
		// if there is only one argument fg 
		if (number_of_arguments == 1) {
			if(foregroundJob(1)==0)
				return 0;
			else
				return 2; 
		}
		//if the first character is %
		int job_number=-1;
		if( **(proc->argv+1)=='%'){
		    sscanf(*(proc->argv + 1), "%% %d", &job_number);
		}
		else{// else if the first character is not %
			job_number=strtol(*(proc->argv + 1), NULL, 10);
		 }
		if (job_number > JOB_LIST_SIZE || job_number < 1) {
			error("job with job index %d not found\n", job_number);
			return 2;
		}
		if (foregroundJob(job_number) == 0)
			return 0;
		else
			return 2;
	} else if (strcmp(argument, "bg") == 0) {
		if (number_of_arguments == 1) {
			if(backgroundJob(1)==0)
				return 0;
			else
				return 2;
		}
		int job_number =-1;
		if( **(proc->argv+1)=='%'){
		    sscanf(*(proc->argv + 1), "%% %d", &job_number);
		}
		else{// else if the first character is not %
			job_number=strtol(*(proc->argv + 1), NULL, 10);
		 }
		if (job_number > JOB_LIST_SIZE || job_number < 1) {
			error("job with job index %d not found\n", job_number);
			return 2;
		}
		if (backgroundJob(job_number) == 0)
			return 0;
		else
			return 2;
	} else if (strcmp(argument, "kill") == 0) {
		if (number_of_arguments <= 1) {
			return 2;
		}
		char* kill_number = malloc(100);
		strcpy(kill_number, *(proc->argv + 1));
		if (*kill_number == '%') {
			int job_index = -1;
			sscanf(kill_number, "%% %d", &job_index);
			if (job_index > JOB_LIST_SIZE || job_index < 1) {
				error("job with job index  %d not found\n", job_index);
				return 2;
			} else {
				Job* job_to_kill=getJob(job_index);
				kill(-(job_to_kill->pgid),SIGCONT);
				kill(-(job_to_kill->pgid),SIGTERM);
				return 0;
			}

		} else {
			int job_pid = -1;
			sscanf(kill_number, "%d", &job_pid);
			kill(job_pid, SIGCONT);
			if(kill(job_pid, SIGTERM)==0)
			return 0;
			else 
				return 2;
		}
		free(kill_number);
	}
	return -3;
}






int changeDirectory(char * path) {
    if (chdir(path) == 0) {
        if (setenv("OLDPWD", getenv("PWD"), 1) == -1) {
			error("fail to change the directory, errno:%d. \n", errno);
            return 2; // failed to set the environment variable
        }
        if (setenv("PWD", path, 1) == -1) {
			error("fail to change the directory, errno:%d. \n", errno);
			return 2; // failed to set the environment variable
        }
        return 0; // changed the directory successful and change the environment variable successfullly
    } else {
        // failed to change the directory 
        return 2;
    }
}

