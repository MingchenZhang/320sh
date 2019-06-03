/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "sbash_debug.h"
#include "sbash_module_job_control.h"
#include "util_array_list.h"

void init_JobControl(){
	JOB_LIST = NULL;
	JOB_LIST_SIZE = 0;
	JOB_LIST_CAPACITY = 0;
}

Job* addJob(Job* job){
	if(JOB_LIST_CAPACITY <= JOB_LIST_SIZE){// increase jobList capacity
		JOB_LIST_CAPACITY = JOB_LIST_CAPACITY*2+10;
		JOB_LIST = realloc(JOB_LIST, JOB_LIST_CAPACITY*sizeof(Job));
	}
	JOB_LIST[JOB_LIST_SIZE++] = *job;
	return JOB_LIST+JOB_LIST_SIZE-1;
}

int removeJob(Job* job){
	int removed = 0;
	int i;
	for(i=0; i<JOB_LIST_SIZE; i++){
		if(JOB_LIST+i == job){
			removeJobIndex(i);
			removed = 1;
		}
	}
	return removed;
}

int removeJobIndex(int index){
	if(index <0 || index >=JOB_LIST_SIZE){
		error("index out of bound\n");
		return -1;
	}
	
	// todo: mem cleanup, also fd, pipe fd
	// find job first
	Job* job = JOB_LIST+index;
	Process* procCursor = job->first_process;
	while(procCursor!=NULL){
		int* argsArray = (int*)procCursor->argv;
		int i;
		for(i=0; i<arrayList_getSize(&argsArray); i++){
			char* arg = *(char**)arrayList_get(i, &argsArray);
			if(arg!=NULL) free(arg);
		}
		arrayList_free(&argsArray);
		Process* tempProc = procCursor;
		procCursor = procCursor->next;
		free(tempProc);
	}
	
	memmove(JOB_LIST+index, JOB_LIST+index+1, (JOB_LIST_SIZE-index-1)*sizeof(Job));
	JOB_LIST_SIZE--;
	return index;
}

void printAllJob(){// todo: check all jobs status before calling; this
	if(JOB_LIST == NULL || JOB_LIST_SIZE <= 0){
		info("no job found\n");
	}
	updateAllJobStatus(1);
	int i;
	for(i=0; i<JOB_LIST_SIZE; i++){
		printAJob(i);
	}
}

void printAJob(int i){
	if(JOB_LIST[i].state!=EXIT && JOB_LIST[i].state!=INTERRUPT){
		printf("[%d]\t(%d)\t%s\t%s\n", 
			i + 1,
			JOB_LIST[i].pgid, 
			JobStateStrGet(JOB_LIST[i].state),
			JOB_LIST[i].command);
	}else{
		printf("[%d]\t(%d)\t%s:%d\t%s\n", 
			i + 1,
			JOB_LIST[i].pgid, 
			JobStateStrGet(JOB_LIST[i].state),
			JOB_LIST[i].first_process->status, // todo: should be the last exit code
			JOB_LIST[i].command);
	}
	
	if (JOB_LIST[i].state == JOB_FINFISH ||
			JOB_LIST[i].state == INTERRUPT ||
			JOB_LIST[i].state == EXIT ) {
		removeJobIndex(i);
	}
}

Job* getJob(int index){
	if(index <=0 || index >JOB_LIST_SIZE){
		error("index out of bound\n");
		return NULL;
	}
	return JOB_LIST+(index-1);
}

void updateAllJobStatus(int noPrint){
	int i;
	for(i=0; i<JOB_LIST_SIZE; i++){
		Job* jobCursor = JOB_LIST+i;
		int changed = 0;
		Process* procCursor = jobCursor->first_process;
		while(procCursor!=NULL){
			int status;
			int pidResult;
			if((pidResult = waitpid(procCursor->pid, &status, WNOHANG | WUNTRACED))==0){
				procCursor = procCursor->next;
				continue;
			}

			if(WIFEXITED(status) && pidResult > 0){
				if(procCursor->state!=P_INTERRUPTTED && procCursor->state!=P_EXIT){
					procCursor->state=P_EXIT;
					procCursor->status = WEXITSTATUS(status);
					changed = 1;
				}
			}else if(WIFSIGNALED(status)){
				if(procCursor->state!=P_INTERRUPTTED && procCursor->state!=P_EXIT){
					procCursor->state = P_INTERRUPTTED;
					procCursor->status = WTERMSIG(status);
					changed = 1;
				}
			}else if(WIFSTOPPED(status)){
				procCursor->state=P_STOP;
				changed = 1;
			}
			procCursor = procCursor->next;
		}
		if(changed){
			int allExit=1;
			int allInter=1;
			int allStop=1;
			int allEnded=1;
			Process* procCursor = jobCursor->first_process;
			while(procCursor!=NULL){
				if(procCursor->state!=P_EXIT) allExit = 0;
				if(procCursor->state!=P_INTERRUPTTED) allInter = 0;
				if(procCursor->state!=P_STOP) allStop = 0;
				if(procCursor->state!=P_EXIT && procCursor->state!=P_INTERRUPTTED)
					allEnded=0;
				
				procCursor = procCursor->next;
			}
			
			if(allExit) jobCursor->state = EXIT;
			if(allInter) jobCursor->state = INTERRUPT;
			if(allStop) jobCursor->state = BACKGROUND_STOP;
			if(allEnded && !allExit) jobCursor->state = INTERRUPT;
			
			if(!noPrint) printAJob(i);
		}
	}
}

void cleanUp_jobControl(){
	for(;JOB_LIST_SIZE>0;){
		removeJobIndex(0);
	}
	free(JOB_LIST);
}
