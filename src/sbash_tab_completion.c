/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sbash_tab_completion.h"
#include "util_array_list.h"

tab_node TAB_ROOT_NODE = {NULL,0, {NULL}};

void tab_collectInfo(char* preSetPath, int noSlash){
	int* path;
	if(preSetPath == NULL) path = getAllPath();
	else{
		path = arrayList_init(2, sizeof(char*));
		arrayList_add(&preSetPath, &path);
	}
	int i;
	for(i=0; i<arrayList_getSize(&path); i++){
		tab_listAllExecutable(*(char**)arrayList_get(i, &path), &TAB_ROOT_NODE, i==0 && !noSlash);
		free(*(char**)arrayList_get(i, &path));
	}
	
	// clean up all path list
	arrayList_free(&path);
}

void tab_free(){
	int i;
	for(i=0; TAB_ROOT_NODE.children[i] != NULL; i++){
		tab_free_(TAB_ROOT_NODE.children[i]);
		TAB_ROOT_NODE.children[i] = NULL;
	}
}

void tab_free_(tab_node* node){
	int i;
	for(i=0; node->children[i] != NULL; i++){
		tab_free_(node->children[i]);
	}
	free(node);
}

int tab_listAllExecutable(char* path, tab_node* info, int includeSlash){
	DIR *d;
	struct dirent *dir;
	(d = opendir(path));
	if(d == NULL) return -1;
	while ((dir = readdir(d)) != NULL){
		// todo: check executable
		char* name = dir->d_name;
		tab_node* infoCursor = info;
		int i;
		for((includeSlash)?(i=-2):(i=0); i<0 || name[i-1]!='\0'; i++){
			char currChar = name[i];
			if(i==-2) currChar = '.';
			else if(i==-1) currChar = '/';
			int lastfound = -1;
			if(!nodeHasLetter(currChar, infoCursor, &lastfound)){
				tab_node nodeModel = {infoCursor, currChar, {NULL}};
				tab_node* newNode = malloc(sizeof(tab_node));
				*newNode = nodeModel;
				infoCursor->children[lastfound] = newNode;
			}
			infoCursor = infoCursor->children[lastfound];
		}
	}
	closedir(d);
	return 0;
}

int nodeHasLetter(char c, tab_node* info, int* lastFound){
	int i;
	for(i=0; info->children[i]!=NULL && i<128; i++){
		if(info->children[i]->c == c){
			*lastFound = i;
			return 1;
		}
	}
	*lastFound = i;
	return 0;
}

char* findLeadingPath(char* token, int* end){
	int tokenStart = *end;
	int i;
	int pathIndex =-1;
	for(i=0; token[i] != '\0' && token[i] != ' '; i++){
		if(token[i] == '/') pathIndex = i;
	}
	if(pathIndex != -1){
		*end = tokenStart+pathIndex+1;
		char* result = malloc(pathIndex+2);
		strncpy(result, token, pathIndex+1);
		result[pathIndex+1] = '\0';
		return result;
	}else{
		return NULL;
	}
}

int* getAllPath(){
	int* array = arrayList_init(10, sizeof(char*));
	
	// here is a malloc in getcwd()
	char* cwd = getcwd(NULL, 0);
	arrayList_add(&cwd, &array);
	char* strtokPtr;
	char* tempPATH = getenv("PATH");
	char* PATH = malloc(strlen(tempPATH)+1);
	char* PATHsave = PATH;
	strcpy(PATH, tempPATH);
	while(1){
		char* path = strtok_r(PATH, ":", &strtokPtr);
		if(path == NULL) break;
		PATH = NULL;
		char* myPath = malloc(strlen(path)*sizeof(char));
		strcpy(myPath, path);
		arrayList_add(&myPath, &array);
	}
	free(PATHsave);
	return array;
}

int getNodeSize(tab_node* node){
	int i;
	for(i=0; node->children[i] != NULL; i++);
	return i;
}

void gotoBranch(tab_node* node,int* start, int** strBuffer){
	while(1){
		arrayList_insert(&(node->c), (*start)++, strBuffer);
		int nodeSize = getNodeSize(node);
		if(nodeSize == 1 && node->children[0]->c=='\0'){
			arrayList_insert(" ", (*start)++, strBuffer);
			break;
		}else if(nodeSize == 1){
			node = node->children[0];
		}else{
			// this is the end
			break;
		}
	}
}

void printOnePath(tab_node* node){
	int* baseStr = arrayList_init(10, sizeof(char));
	arrayList_add("\0", &baseStr);
	tab_node* cursor = node;
	while(cursor != &TAB_ROOT_NODE){
		arrayList_insert(&(cursor->c), 0, &baseStr);
		cursor = cursor->parent;
	}
	fprintf(stderr, "%s\n", (char*)baseStr);
	arrayList_free(&baseStr);
}

void printAllOption(tab_node* node){
	int nodeSize = getNodeSize(node);
	if(nodeSize==0){
		printOnePath(node);
		return;
	}
	int i;
	for(i=0; i<nodeSize; i++){
		printAllOption(node->children[i]);
	}
}