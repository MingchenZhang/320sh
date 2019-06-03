/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   sbash_processing.h
 * Author: mingchen
 *
 * Created on March 12, 2016, 10:20 PM
 */

#ifndef SBASH_PROCESSING_H
#define SBASH_PROCESSING_H

#include "sbash_execution.h"

#define DEFAULT_TOKEN_CACHE_SIZE 10

typedef struct TokenRediInfo{
	int direction;			// positive if output, negative if input, 0 if no redirection
	int fd;					// fd that needs to be redirected
} TokenRediInfo;

int process(char* command, char** env);
int process_test(char* command, char** env);
void expandVariable(int** tokens);
void expandGlobbing(int** tokens);
int isProcessBackground(int** tokens);
int* tokenizeCommandStr(char* comm);
int* tokenizeProcessStr(char* procStr);
void quotationReplacement(char* str);
void quotationReversionForCPP(char** strPtr, int strPtrSize);
void quotationReversionForCP(char* token);
TokenRediInfo tokenRedirectionInfo(char* token);
RedirectionInfo buildFileRedirect(int** array);
char isComplete(char* command);
struct termios getDefaultJobTerm();

#endif /* SBASH_PROCESSING_H */

