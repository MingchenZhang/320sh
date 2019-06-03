#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

int isNameMatchGlobbing(char* name, char* globbing){
	//printf("%s\n",name);
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

int main(int argc, char ** argv, char **envp){
	int result = isNameMatchGlobbing(argv[1], argv[2]);
	printf("%d\n", result);
}