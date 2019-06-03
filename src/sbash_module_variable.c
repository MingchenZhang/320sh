#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>

#include "sbash_module_variable.h"

void variable_init(){
	QuestionMark.value = malloc(sizeof(char)*4);
}

void* variable_get(char* key){
	if(strcmp(key, "?") == 0) return QuestionMark.value;
	else return getenv(key); 
}

void variable_set(char* key, void* value){
	if(strcmp(key, "?") == 0) strncpy(QuestionMark.value, value, 4);
	else setenv(key, value, 1);
}
