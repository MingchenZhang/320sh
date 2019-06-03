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
#include <dirent.h>

#include "sbash_debug.h"
#include "sbash_processing.h"
#include "sbash_execution.h"

#include "sbash_module_job_control.h"
#include "util_array_list.h"
#include "sbash_module_variable.h"

int process_test(char* command, char** env){
	char * args[1] = {NULL};
	RedirectionInfo testRedirect = {
		0,
		NULL,
		-1,
		0,
		NULL,
		-1,
	};
	Process testProcess = {
		NULL,
		"test/infinity_loop",
		args,
		env,
		-1,
		P_INIT,
		0,
		testRedirect,
	};
	Job testJob = {
		"test command: test/infinity_loop",
		&testProcess,
		-1,
		TOBE_FOREGROUND,
		0,
		getDefaultJobTerm()
	};
	
	
	if(strcmp(command, "infinity_loop\n") == 0){
		info("infinity_loop execution started\n");
		executeJob(&testJob);
		info("infinity_loop execution finished\n");
	}else if(strcmp(command, "infinity_loop_bg\n") == 0){
		testJob.state = TOBE_BACKGROUND;
		info("infinity_loop_bg execution started\n");
		executeJob(&testJob);
		info("infinity_loop_bg execution finished\n");
	}else if(strcmp(command, "FDE\n") == 0){
		testProcess.path = "file_not_exist";
		info("FDE execution started\n");
		executeJob(&testJob);
		info("FDE execution finished\n");
	}else if(strcmp(command, "jobs\n") == 0){
		  printAllJob();
	}else if(strcmp(command, "fg 1\n") == 0){
		foregroundJob(0);
	}else if(strcmp(command, "fg 2\n") == 0){
		foregroundJob(1);
	}
	return 0;
}

int process(char* command, char** env){
	char* commandBackup = malloc((strlen(command)+1)*sizeof(char));
	strcpy(commandBackup, command);
	quotationReplacement(command);
	
	int* commandTokenArray = tokenizeCommandStr(command);
	
	Process* lastProc = NULL;
	Process* firstProc = NULL;
	int isBackground = 0;
	int j;
	for(j=0; j<arrayList_getSize(&commandTokenArray); j++){
		char * processStr = *(char**)arrayList_get(j, &commandTokenArray);
		
		debug("process scanning started\n");
		int* procTokenArray = tokenizeProcessStr(processStr);
		debug("process scanning completed\n");
		int size = arrayList_getSize(&procTokenArray);
		if(size == 0) continue; // this is invalid process command
		debug("array size: %d\n", size);
		int i;
		for(i=0; i<size; i++){
			debug("text:%s\n", *(char**)arrayList_get(i, &procTokenArray));
		}
		//debug("print complete\n");

		RedirectionInfo redirect = buildFileRedirect(&procTokenArray);
		
		if(!isBackground && isProcessBackground(&procTokenArray)) isBackground = 1;

		quotationReversionForCPP((char**)procTokenArray, arrayList_getSize(&procTokenArray));
		
		expandVariable(&procTokenArray);
		
		expandGlobbing(&procTokenArray);

		void* null = NULL;
		arrayList_add(&null, &procTokenArray);
		Process* proc = malloc(sizeof(Process));
		Process procStruct = {
			NULL,
			*(char**)arrayList_get(0, &procTokenArray),
			(char**)procTokenArray,
			env,
			-1,
			P_INIT,
			0,
			redirect,
		};
		if(lastProc != NULL) lastProc->next = proc;
		if(lastProc != NULL) lastProc->redirect.out_proc = proc;
		procStruct.redirect.in_proc = lastProc;
		*proc = procStruct;
		
		lastProc = proc;
		if(firstProc == NULL) firstProc = proc;
	}
	
	Job job = {
		commandBackup,
		firstProc,
		-1,
		(isBackground)?TOBE_BACKGROUND:TOBE_FOREGROUND,
		0,
		getDefaultJobTerm()
	};
	
	if(isBackground)debug("this is a background job\n");
	debug("RUNNING:%s\n", job.command);
	if(firstProc != NULL) executeJob(&job);
	debug("RUNNING FINISHED\n");
	
	return 0;
}

int isProcessBackground(int** tokens){
	if(arrayList_getSize(tokens) > 0){
		//debug("bg check:\"%s\"\n", arrayList_get(arrayList_getSize(tokens)-1, tokens));
		if(strcmp(*(char**)arrayList_get(arrayList_getSize(tokens)-1, tokens), "&") == 0){
			arrayList_remove(arrayList_getSize(tokens)-1, tokens);
			return 1;
		}
	}
	return 0;
}

int* tokenizeCommandStr(char* comm){
	char* strtokPtr;
	int* commArray = arrayList_init(DEFAULT_TOKEN_CACHE_SIZE, sizeof(char*));
	
	char* proc;
	do{
		proc = strtok_r(comm, "|", &strtokPtr);
		if(proc == NULL) break;
		comm = NULL;
		arrayList_add(&proc, &commArray);
	}while(1);
	return commArray;
}

int* tokenizeProcessStr(char* procStr){
	char* strtokPtr;
	int* tokenArray;
	tokenArray = arrayList_init(DEFAULT_TOKEN_CACHE_SIZE, sizeof(char*));
	
	char* token;
	do{
		token = strtok_r(procStr, " \"'", &strtokPtr);
		if(token == NULL) break;
		procStr = NULL;
		
		// add new token into result
		char* tokenCpy = malloc( sizeof(char)*(strlen(token)+1) );
		strcpy(tokenCpy, token);
		arrayList_add(&tokenCpy, &tokenArray);
		
		if(token == NULL) break;
	}while(1);
	return tokenArray;
}

void quotationReplacement(char* str){
	int inQuotation = 0;
	int i;
	for(i=0; str[i]!='\0'; i++){
		if(str[i] == '"' || str[i] == '\'') inQuotation = !inQuotation;
		if(inQuotation && str[i] == ' ')
			str[i] = 127;// replace all spaces with DEL
		else if(inQuotation && str[i] == '<')
			str[i] = 17;
		else if(inQuotation && str[i] == '|')
			str[i] = 18;
		else if(inQuotation && str[i] == '>')
			str[i] = 19;
	}
}

void quotationReversionForCPP(char** strPtr, int strPtrSize){
	int i;
	for(i=0; i<strPtrSize; i++){
		char* token = strPtr[i];
		int j;
		for(j=0; token[j]!='\0'; j++){
			if(token[j] == 127)
				token[j] = ' ';
			else if(token[j] == 17)
				token[j] = '<';
			else if(token[j] == 18)
				token[j] = '|';
			else if(token[j] == 19)
				token[j] = '>';
		}
	}
}

void quotationReversionForCP(char* token){
	int j;
	for(j=0; token[j]!='\0'; j++){
		if(token[j] == 127)
			token[j] = ' ';
		else if(token[j] == 17)
			token[j] = '<';
		else if(token[j] == 18)
			token[j] = '|';
		else if(token[j] == 19)
			token[j] = '>';
	}
}

TokenRediInfo tokenRedirectionInfo(char* token){
	TokenRediInfo result;
	result.direction = 0;
	if(token[strlen(token)-1] == '>'){
		result.direction = 1;
		int parsed = sscanf(token, "%d", &result.fd);
		if(parsed != 1 && strlen(token)>1)
			result.direction = 0;
		else if(parsed < 1)
			result.fd = STDOUT_FILENO;
	}
	else if(token[strlen(token)-1] == '<'){
		result.direction = -1;
		int parsed = sscanf(token, "%d", &result.fd);
		if(parsed != 1 && strlen(token)>1)
			result.direction = 0;
		else if(parsed < 1)
			result.fd = STDIN_FILENO;
	}
	return result;
}

RedirectionInfo buildFileRedirect(int** array){
	RedirectionInfo result = {
		arrayList_init(2, sizeof(FileRedirectPair)),
		NULL,
		-1,
		arrayList_init(2, sizeof(FileRedirectPair)),
		NULL,
		-1,
	};
	int index; 
	for(index=0; index<arrayList_getSize(array); index++){
		TokenRediInfo thisToken = tokenRedirectionInfo(*(char**)arrayList_get(index, array));
		if(thisToken.direction != 0 && arrayList_get(index+1, array) != NULL){
			FileRedirectPair tfr;
			tfr.process_fd = thisToken.fd;
			tfr.file = *(char**)arrayList_get(index+1, array);
			quotationReversionForCP(tfr.file);
			free(*(char**)arrayList_get(index, array));
			arrayList_remove(index, array);
			arrayList_remove(index, array);
			index -= 2;
			if(thisToken.direction > 0){// this is output
				arrayList_add(&tfr, &result.out_file_arrayList);
			}else{// this is input
				arrayList_add(&tfr, &result.in_file_arrayList);
			}
		}
	}
	return result;
}

char isComplete(char* command){
	return 1;
}

struct termios getDefaultJobTerm(){
	// todo: find a standard terminal setting
	struct termios defaultJobTerm;
	tcgetattr(STDIN_FILENO, &defaultJobTerm);
	defaultJobTerm.c_lflag |= (ECHO|ECHOE|ECHOK|ECHONL|ICANON|ISIG);
	return defaultJobTerm;
}

void expandVariable(int** tokens){
	int i;
	for(i=0; i<arrayList_getSize(tokens); i++){
		if(**(char**)arrayList_get(i, tokens) == '$'){
			 char* value = variable_get( (*(char**)arrayList_get(i, tokens))+1 );
			if(value == NULL) value = "";
			debug("variable read:%s\n", value);
			char* transValue = malloc(strlen(value)+1);
			strcpy(transValue, value);
			free(*(char**)arrayList_get(i, tokens));
			arrayList_set(&transValue, i, tokens);
		}
	}
}

static int isGlobbing(char* str){
	int i;
	for(i=0; str[i] != '\0'; i++){
		if(str[i] == '*') return 1;
	}
	return 0;
}

static int chopToPath(char* str){
	int i=strlen(str);
	while(i>=0){
		if(str[i] == '/'){
			str[i+1] = '\0';
			return 1;
		}
		i--;
	}
	return 0;
}

static char* mallocFileName(char* str){
	char* filename = str;
	int i=strlen(str);
	while(i>=0){
		if(str[i] == '/'){
			filename = str+i+1;
			break;
		}
		i--;
	}
	char* result = malloc((strlen(filename)+1)*sizeof(char));
	strcpy(result, filename);
	return result;
}

static char* mallocConcatenate(char* str1, char* str2){
	char* result = malloc((strlen(str1)+strlen(str2)+1)*sizeof(char));
	strcpy(result, str1);
	strcat(result, str2);
	return result;
}

static int isNameMatchGlobbing(char* name, char* globbing){
	int nSize = strlen(name);
	int gSize = strlen(globbing);
	int nIndex = 0;
	int gIndex = 0;
	while(1){
		if(gIndex > gSize || nIndex > nSize) return 0;
		if(name[nIndex] == '\0' && globbing[gIndex] == '\0') return 1;
		if(globbing[gIndex] == '*'){
			if(name[nIndex] == globbing[gIndex+1]){
				gIndex++;
			}else{
				nIndex++;
			}
		}else if(name[nIndex] == globbing[gIndex]){
			nIndex++; gIndex++;
		}else return 0;
	}
}

void expandGlobbing(int** tokens){
	int i;
	for(i=0; i<arrayList_getSize(tokens); i++){
		if(!isGlobbing(*(char**)arrayList_get(i, tokens))) continue;
		
		char* filename = mallocFileName(*(char**)arrayList_get(i, tokens));
		char* path;
		if(chopToPath(*(char**)arrayList_get(i, tokens)) == 0)
			path = "./";
		else
			path = *(char**)arrayList_get(i, tokens);
		DIR *d;
		struct dirent *dir;
		(d = opendir(path));
		if(d == NULL) {
			goto expandGlobbing_clean;
		}
		while ((dir = readdir(d)) != NULL){
			if(isNameMatchGlobbing(dir->d_name, filename)){
				char* newPath = mallocConcatenate(path, dir->d_name);
				arrayList_insert(&newPath, i, tokens);
				i++;
			}
		}
		closedir(d);
expandGlobbing_clean:
		free(*(char**)arrayList_get(i, tokens));
		free(filename);
		arrayList_remove(i, tokens);
	}
}