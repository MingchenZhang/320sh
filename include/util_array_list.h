/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   util_array_list.h
 * Author: mingchen
 *
 * Created on March 15, 2016, 11:10 PM
 */

#ifndef UTIL_ARRAY_LIST_H
#define UTIL_ARRAY_LIST_H

#define ARRAY_LIST_SIZE ((*array)[-3])
#define ARRAY_LIST_CAPA ((*array)[-2])
#define ARRAY_LIST_ELEMENT_SIZE ((*array)[-1])

int* arrayList_init(int initialCapacity, int elementSize);

void* arrayList_add(void* newElement, int** array);

void arrayList_addArray(void* newElement, int size, int** array);

void* arrayList_insert(void* newElement, int index, int** array);

void* arrayList_set(void* newElement, int index, int** array);

void* arrayList_get(int index, int** array);

void arrayList_remove(int index, int** array);

void arrayList_chop(int index, int** array);

void arrayList_fall(int index, int** array);

void arrayList_empty(int** array);

void arrayList_free(int** array);

int arrayList_getSize(int** array);

#endif /* UTIL_ARRAY_LIST_H */

