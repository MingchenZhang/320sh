#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int args, char** argv){
	char* abc = malloc(20);
	strcpy(abc, "test");
	setenv("a", abc, 1);
	strcpy(abc, "tsefefefw");
	printf("%s\n", getenv("a"));
}