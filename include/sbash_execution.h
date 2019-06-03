/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   sbash_execution.h
 * Author: mingchen
 *
 * Created on March 12, 2016, 9:11 PM
 */

#ifndef SBASH_EXECUTION_H
#define SBASH_EXECUTION_H

#include <termios.h>

int TIME_FLAG;

typedef enum ProcessState {P_INIT, P_RUNNING, P_STOP, P_COMPLETE, P_INTERRUPTTED, P_EXIT} ProcessState;
typedef enum JobState {TOBE_FOREGROUND, TOBE_BACKGROUND, FOREGROUND, BACKGROUND, BACKGROUND_STOP, JOB_FINFISH, INTERRUPT, EXIT} JobState;
char* JobStateStrGet(JobState js);

typedef struct Process Process;
typedef struct Job Job;

typedef struct FileRedirectPair{
	int process_fd;
	char* file;
} FileRedirectPair;

typedef struct RedirectionInfo{
	int* in_file_arrayList;		/* store all in redirection to a file, size 0 if no file redirection */
	Process* in_proc;			/* standard in process, NULL if no process redirection */
	int in_fd;					/* used if redirect from a process, initialize to -1 */
	int* out_file_arrayList;	/* store all out redirection to a file, size 0 if no file redirection */
	Process* out_proc;			/* standard out process, NULL if no process redirection */
	int out_fd;					/* used if redirect to a process, initialize to -1 */
} RedirectionInfo;

typedef struct Process {
	struct Process *next;		/* next process */
	char * path;				/* process path */
	char **argv;				/* all arguments including process filename */
	char **envp;				/* process environment variable */
	pid_t pid;					/* process ID,W initialize to 0 */
	ProcessState state;			/* process current state */
	int status;					/* reported exit value */
	RedirectionInfo redirect;	    /* redirection info */
} Process;

typedef struct Job {
	char *command;				/* command line, used for messages */
	Process *first_process;		       /* list of processes in this job */
	pid_t pgid;		               /* process group ID */
	JobState state;				/* job current state */
	int terminalSaved;			/* true if terminal state was saved before (0 initially) */
	struct termios tmodes;		/* saved terminal modes */
} Job;

Job* jobs;

/**
 * execute command
 * @param job needs to be executed
 * @return negative if it failed (and it is the error code)
 * 0 if it succeed.
 */
int executeJob(Job* job);

int foregroundJob(int index);

int waitForegroundJob(Job* job);

int waitForJob(Job* job);

void initializeJobTerminal();

int isPath(char* str);

int isExeExist(char* path);

int isInSystemPathAndExecuable(char* fileName);

int isExecutable(char* path);

int isBuiltin(char* cmd);

/**
 * detect and react to builtin command
 * @param proc process info
 * @return -1 if this proc is not builtin. -2 if it is a complex builtin. 
 * otherwise, it is a status code. 
 */
int builtinAction(Process* proc, int runningComplex);
int changeDirectory(char * path);


#endif /* SBASH_EXECUTION_H */

