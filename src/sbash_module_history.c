/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sbash_module_history.h"
#include "util_array_list.h"

#define INITIAL_HISTORY_CAPA 10
int* HISTORY_RECORD = NULL;
int HISTORY_UNSAVED_BOUND = 0;

void history_put(char* str){
	if(HISTORY_RECORD == NULL) 
		HISTORY_RECORD = arrayList_init(INITIAL_HISTORY_CAPA, sizeof(char*));

	// compare if the last history is repeated
	if(arrayList_getSize(&HISTORY_RECORD)>0 && strcmp(history_get(0), str) == 0) return;
	
	char* newStr = malloc( (strlen(str)+1)*sizeof(char) );
	strcpy(newStr, str);
	arrayList_insert(&newStr, 0, &HISTORY_RECORD);
	HISTORY_UNSAVED_BOUND++;
}

char* history_get(int index){
	if(HISTORY_RECORD == NULL) return NULL;
	void* his = arrayList_get(index, &HISTORY_RECORD);
	if(his == NULL) return NULL;
	return *(char**)his;
}

int history_getSize(){
	if(HISTORY_RECORD == NULL) return -1;
	return arrayList_getSize(&HISTORY_RECORD);
}

void history_clean(){
	if(HISTORY_RECORD!=NULL){
		int i;
		for(i=0; i<arrayList_getSize(&HISTORY_RECORD); i++){
			free(*(char**)arrayList_get(i, &HISTORY_RECORD));
		}
		arrayList_free(&HISTORY_RECORD);
		HISTORY_RECORD = NULL;
		HISTORY_UNSAVED_BOUND = 0;
	}
}

char* history_rSearch(int* startFrom, char* query){
	int queryLength = strlen(query);
	int queryCursor = 0;
	int histListCursor = (*startFrom)+1;
	if(histListCursor >= arrayList_getSize(&HISTORY_RECORD)) histListCursor = 0;
	char* historyCursor;
	int loppNum = -1;
	for(;;){
		if(histListCursor == loppNum) return NULL;
		else if(loppNum == -1) loppNum = histListCursor;
		
		historyCursor = *(char**)arrayList_get(histListCursor, &HISTORY_RECORD);
		queryCursor = 0;
		while((*historyCursor) != '\0'){
			if((*historyCursor) == query[queryCursor]){
				queryCursor++;
			}else if(queryCursor!=0 && (*historyCursor) != query[queryCursor]){
				queryCursor = 0;
			}
			if(queryCursor == queryLength){
				*startFrom = histListCursor;
				return *(char**)arrayList_get(histListCursor, &HISTORY_RECORD);
			}
			historyCursor++;
		}
		
		
		histListCursor++;
		if(histListCursor >= arrayList_getSize(&HISTORY_RECORD)) histListCursor = 0;
	}
	return NULL;
}

int history_save(char* path){
	FILE* fp = fopen(path, "a");
	if(fp == NULL) return -1;
	if(HISTORY_RECORD != NULL){
		int i;
		for(i=HISTORY_UNSAVED_BOUND; i>=0; i--){
			fprintf(fp, "%s\n", *(char**)arrayList_get(i, &HISTORY_RECORD));
		}
	}
	HISTORY_UNSAVED_BOUND = 0;
	fclose(fp);
	return 0;
}

int history_initAndLoad(char* path){
	history_clean();
	if(HISTORY_RECORD == NULL) 
		HISTORY_RECORD = arrayList_init(INITIAL_HISTORY_CAPA, sizeof(char*));
	
	FILE* fp = fopen(path, "r");
	if(fp == NULL) return -1;
	while(1){
		char* lineBuffer = NULL;
		size_t size = 0;
		if(getline(&lineBuffer, &size, fp) == -1){
			// meet the end or failed
			free(lineBuffer);
			break;
		}else{
			lineBuffer[strlen(lineBuffer)-1] = '\0';
			history_put(lineBuffer);
			free(lineBuffer);
		}
	}
	HISTORY_UNSAVED_BOUND = -1;
	fclose(fp);
	return 0;
}