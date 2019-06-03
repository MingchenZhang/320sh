/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "util_array_list.h"

int* arrayList_init(int initialCapacity, int elementSize){
	int* array = malloc(elementSize*initialCapacity + sizeof(int)*3 );
	array[0] = 0;
	array[1] = initialCapacity;
	array[2] = elementSize;
	return array+3;
}

void* arrayList_add(void* newElement, int** array){
	if(ARRAY_LIST_SIZE == ARRAY_LIST_CAPA){// new space needed
		*array = realloc((*array)-3, ARRAY_LIST_ELEMENT_SIZE*(ARRAY_LIST_CAPA*2) + sizeof(int)*3);
		*array += 3;
		ARRAY_LIST_CAPA *= 2;
	}
	void* targetAddress = ((uint8_t*)((*array))+(ARRAY_LIST_ELEMENT_SIZE*ARRAY_LIST_SIZE));
	memmove(targetAddress, newElement, ARRAY_LIST_ELEMENT_SIZE);
	ARRAY_LIST_SIZE++;
	return targetAddress;
}

void arrayList_addArray(void* newElement, int size, int** array){
	uint8_t* elemCursor =  newElement;
	int i;
	for(i=0; i<size; i++){
		arrayList_add(elemCursor, array);
		elemCursor += ARRAY_LIST_ELEMENT_SIZE;
	}
}

void* arrayList_insert(void* newElement, int index, int** array){
	if(index > ARRAY_LIST_SIZE || index < 0) return NULL;
	if(ARRAY_LIST_SIZE == ARRAY_LIST_CAPA){// new space needed
		*array = realloc((*array)-3, ARRAY_LIST_ELEMENT_SIZE*(ARRAY_LIST_CAPA*2) + sizeof(int)*3);
		*array += 3;
		ARRAY_LIST_CAPA *= 2;
	}
	uint8_t* targetAddress = ((uint8_t*)((*array))+(ARRAY_LIST_ELEMENT_SIZE*index));
	memmove(targetAddress + ARRAY_LIST_ELEMENT_SIZE, targetAddress, ARRAY_LIST_ELEMENT_SIZE*(ARRAY_LIST_SIZE-index));
	memmove(targetAddress, newElement, ARRAY_LIST_ELEMENT_SIZE);
	ARRAY_LIST_SIZE++;
	return targetAddress;
}

void* arrayList_set(void* newElement, int index, int** array){
	if(index >= ARRAY_LIST_SIZE || index < 0) return NULL;
	void* targetAddress = ((uint8_t*)((*array))+(ARRAY_LIST_ELEMENT_SIZE*index));
	memmove(targetAddress, newElement, ARRAY_LIST_ELEMENT_SIZE);
	return targetAddress;
}

void* arrayList_get(int index, int** array){
	if(index >= ARRAY_LIST_SIZE || index < 0) return NULL;
	void* targetAddress = ((uint8_t*)((*array))+(ARRAY_LIST_ELEMENT_SIZE*index));
	return targetAddress;
}

void arrayList_remove(int index, int** array){
	if(index >= ARRAY_LIST_SIZE || index < 0) return;
	void* targetAddress = ((uint8_t*)((*array))+(ARRAY_LIST_ELEMENT_SIZE*index));
	memmove(targetAddress, ((uint8_t*)(targetAddress))+ARRAY_LIST_ELEMENT_SIZE, ARRAY_LIST_ELEMENT_SIZE*(ARRAY_LIST_SIZE-index-1) );
	ARRAY_LIST_SIZE--;
}

void arrayList_chop(int index, int** array){
	if(index >= ARRAY_LIST_SIZE || index < 0) return;
	ARRAY_LIST_SIZE = index;
	// fill a 0 in case of sequential search (like strlen())
	void* targetAddress = ((uint8_t*)((*array))+(ARRAY_LIST_ELEMENT_SIZE*index));
	memset(targetAddress, 0, ARRAY_LIST_ELEMENT_SIZE);
}

void arrayList_fall(int index, int** array){
	if(index >= ARRAY_LIST_SIZE || index < 0) return;
	void* targetAddress = ((uint8_t*)((*array))+(ARRAY_LIST_ELEMENT_SIZE*index));
	memmove(*array, targetAddress, (ARRAY_LIST_SIZE-index)*ARRAY_LIST_ELEMENT_SIZE);
	ARRAY_LIST_SIZE -= index;
}

void arrayList_empty(int** array){
	ARRAY_LIST_SIZE = 0;
}

void arrayList_free(int** array){
	free((*array)-3);
}

int arrayList_getSize(int** array){
	return ARRAY_LIST_SIZE;
}
