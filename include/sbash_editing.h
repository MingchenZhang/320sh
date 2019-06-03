/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   sbash_editing.h
 * Author: mingchen
 *
 * Created on March 12, 2016, 10:23 PM
 */

#ifndef SBASH_EDITING_H
#define SBASH_EDITING_H

#define PROMPT "320sh> "

typedef struct Point{
    int x;
    int y;
} Point;

void getCursorLocation(int* x, int* y);

void refreshString(Point* original, int cursor, int** array);

void prompt(char** env);

int isEmptyStr(char* str);

void moveToNextWord(int* cursor, int** editingBuffer);

void moveToPrevWord(int* cursor, int** editingBuffer);

int historyRSearchMode(Point original, int* executeNow);

void findCurrentEditingToken(int* start, int end, int** editingBuffer);

#endif /* SBASH_EDITING_H */

