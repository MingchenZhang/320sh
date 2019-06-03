/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   sbash_tab_completion.h
 * Author: mingchen
 *
 * Created on March 27, 2016, 8:50 PM
 */

#ifndef SBASH_TAB_COMPLETION_H
#define SBASH_TAB_COMPLETION_H

typedef struct tab_node{
	struct tab_node* parent;
	char c;
	struct tab_node* children[128];
} tab_node;

tab_node TAB_ROOT_NODE;

void tab_collectInfo(char* preSetPath, int noSlash);

void tab_free();

void tab_free_(tab_node* node);

int tab_listAllExecutable(char* path, tab_node* info, int includeSlash);

int nodeHasLetter(char c, tab_node* info, int* lastFound);

char* findLeadingPath(char* token, int* end);

int* getAllPath();

int getNodeSize(tab_node* node);

void gotoBranch(tab_node* node,int* start, int** strBuffer);

void printAllOption(tab_node* node);

#endif /* SBASH_TAB_COMPLETION_H */

