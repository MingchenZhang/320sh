/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "sbash_init.h"
#include "sbash_debug.h"
#include "sbash_editing.h"
#include "sbash_processing.h"
#include "util_array_list.h"
#include "sbash_module_history.h"
#include "sbash_tab_completion.h"
#include "sbash_module_job_control.h"

#define clear() fprintf(stderr, "\033[H\033[J");fflush(stdout);
#define gotoxy(x,y) fprintf(stderr, "\033[%d;%dH", (y+1), (x+1));fflush(stdout);
#define report() fprintf(stderr, "\033[6n");fflush(stdout);
#define setInsert() fprintf(stderr, "\033[4h");fflush(stdout);
#define moveLeft() fprintf(stderr, "\033[1D");fflush(stdout);
#define moveRight() fprintf(stderr, "\033[1C");fflush(stdout);
#define moveRight2(x) fprintf(stderr, "\033[%dC", x);fflush(stdout);
#define createNewLine() fprintf(stderr, "\033[L");fflush(stdout);
#define saveCursor() fprintf(stderr, "\033[s");fflush(stdout);
#define loadCursor() fprintf(stderr, "\033[u");fflush(stdout);
#define eraseAfter() fprintf(stderr, "\033[J");fflush(stdout);
#define autowrap() fprintf(stderr, "\033[7h");fflush(stdout);
#define deleteChar(x) fprintf(stderr, "\033[%dP", x);fflush(stdout);

void getCursorLocation(int* x, int* y){
	report();
	fflush(stdout);
	if(scanf("\033[%d", y) < 1) *y = 0;
	if(scanf(";%d", x) < 1) *x = 0;
	(*x)--; (*y)--;
	scanf("R");
}

Point getWindowSize(){
    int x = 80;
    int y = 24;

#ifdef TIOCGSIZE
    struct ttysize term;
    ioctl(STDIN_FILENO, TIOCGSIZE, &term);
    x = term.ts_cols;
    y = term.ts_lines;
#elif defined(TIOCGWINSZ)
    struct winsize term;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &term);
    x = term.ws_col;
    y = term.ws_row;
#endif /* TIOCGSIZE */

	Point size = {x,y};
    return size;
}

void refreshString(Point* original, int cursor, int** array){
	if(cursor<0 || cursor> arrayList_getSize(array)){
		debug("cursor out of bound: %d\n", cursor);
		return;
	}
	
	gotoxy(original->x, original->y);
	eraseAfter();
	fflush(stdout);
	fprintf(stderr, "%s", *(char**)array);
	fflush(stdout);
	
	Point size = getWindowSize();
	Point newPoint = *original;
	int lineCount = arrayList_getSize(array)-1;
	while(lineCount>0){
		if(lineCount<size.x-newPoint.x){// this line is fit
			newPoint.x += lineCount;
			break;
		}
		lineCount -= size.x - newPoint.x;
		newPoint.y++;
		newPoint.x = 0;
		if(newPoint.y >= size.y){
			original->y--;
			newPoint.y--;
			if(lineCount == 0){
				fprintf(stderr, " ");
				fflush(stdout);
			}
		}
	}
	
	newPoint = *original;
	while(cursor>0){
		if(cursor<size.x-newPoint.x){// this line is fit
			newPoint.x += cursor;
			break;
		}
		cursor -= size.x - newPoint.x;
		newPoint.y++;
		newPoint.x = 0;
	}
	gotoxy(newPoint.x, newPoint.y);
	fflush(stdout);
}

void prompt(char** env){
	while(1){
		new_prompt:
		updateAllJobStatus(0);
		
		Point promptStartPosition;
		Point original;
		int* editingBuffer = arrayList_init(20, sizeof(char));
		arrayList_add("\0", &editingBuffer);
		int cursor = 0;
		getCursorLocation(&promptStartPosition.x, &promptStartPosition.y);
		char currentBuffer[10];

		int historyIndex = -1;
		int lineFinished = 0;
		int propmtUpdateNeeded = 1;
		int historyUpdateNeeded = 0;
		int executeNow = 0;
		int originUpdateNeeded = 0;
		debug("history size=%d\n", history_getSize());
		while(!lineFinished){
			if(originUpdateNeeded){
				getCursorLocation(&promptStartPosition.x, &promptStartPosition.y);
				originUpdateNeeded = 0;
				propmtUpdateNeeded = 1;
			}
			
			//propmtUpdateNeeded = 1;
			// todo: prompt longer than width
			if(propmtUpdateNeeded){
				gotoxy(promptStartPosition.x, promptStartPosition.y);
				int offset = 0;
				char* currPath = getcwd(NULL, 0);
				offset += fprintf(stderr, "[%s] ", currPath);
				free(currPath);
				offset += fprintf(stderr, PROMPT);
				fflush(stdout);
				original.x = promptStartPosition.x + offset;
				original.y = promptStartPosition.y;
				propmtUpdateNeeded = 0;
			}
			
			if(historyUpdateNeeded && historyIndex != -1){
				char* history = history_get(historyIndex);
				if (history == NULL) {
					historyIndex = 0;
					history = history_get(historyIndex);
				}
				arrayList_empty(&editingBuffer);
				int i;
				for(i=0; i<strlen(history)+1; i++){
					arrayList_add(&(history[i]), &editingBuffer);
				}
				cursor = arrayList_getSize(&editingBuffer)-1;
				historyUpdateNeeded = 0;
			}else if(historyUpdateNeeded && historyIndex == -1) {
				arrayList_empty(&editingBuffer);
				arrayList_add("\0", &editingBuffer);
				historyUpdateNeeded = 0;
				cursor = arrayList_getSize(&editingBuffer)-1;
			}	
			
			refreshString(&original, cursor, &editingBuffer);
			
			if(!executeNow){
				if(read(STDIN_FILENO, &currentBuffer, 1) <0){
					fprintf(stderr, "\n");
					fflush(stdout);
					goto new_prompt;
				}
			}
			else currentBuffer[0] = '\n';

			if(currentBuffer[0] == '\033'){//escape sequence
				read(STDIN_FILENO, currentBuffer+1, 1);
				if(currentBuffer[1] == 102){// alt+f
					moveToNextWord(&cursor, &editingBuffer);
					continue;
				}else if(currentBuffer[1] == 98){//alt+b
					moveToPrevWord(&cursor, &editingBuffer);
					continue;
				}else if(currentBuffer[1] == '['){
					read(STDIN_FILENO, currentBuffer+2, 1);
				}else continue;
				
				
				if(currentBuffer[2] == 'D'){// left key
					if(cursor>0){
						cursor--;
					}
				}else if(currentBuffer[2] == 'C'){// right key
					if(cursor<arrayList_getSize(&editingBuffer)-1){
						cursor++;
					}
				}else if(currentBuffer[2] == 'A'){// up key
					if(historyIndex < history_getSize()-1){
						historyIndex++;
						historyUpdateNeeded = 1;
					}
				}else if(currentBuffer[2] == 'B'){// down key
					if(historyIndex > -1){
						historyIndex--;
						historyUpdateNeeded = 1;
					}
				}
			}else if(currentBuffer[0] == 127){// back space
				if(cursor>0){
					arrayList_remove(cursor-1, &editingBuffer);
					cursor--;
				}
			}else if(currentBuffer[0] == 1){// ctrl-a key
				cursor = 0;
			}else if(currentBuffer[0] == 11){// ctrl-k key
				arrayList_chop(cursor, &editingBuffer);
				arrayList_add("\0", &editingBuffer);
			}else if(currentBuffer[0] == 21){// ctrl-u key
				arrayList_fall(cursor, &editingBuffer);
				cursor = 0;
			}else if(currentBuffer[0] == 12){// ctrl-l key
				arrayList_empty(&editingBuffer);
				char* clear = "clear";
				arrayList_addArray(clear, strlen(clear)+1, &editingBuffer);
				cursor = arrayList_getSize(&editingBuffer)-1;
				process((char*)editingBuffer, env);
				arrayList_free(&editingBuffer);
				lineFinished = 1;
			}else if(currentBuffer[0] == 18){// ctrl-r key
				historyIndex = historyRSearchMode(promptStartPosition, &executeNow);
				historyUpdateNeeded = 1;
				propmtUpdateNeeded = 1;
			}else if(currentBuffer[0] == '\t'){
				int tokenStart = cursor;
				findCurrentEditingToken(&tokenStart, cursor, &editingBuffer);
				char* path = findLeadingPath(((char*)editingBuffer)+tokenStart, &tokenStart);
				tab_collectInfo(path,tokenStart!=0);// todo: actually should check # of token
				
				// first find what we have now
				int i;
				char* eb = ((char*)editingBuffer);
				tab_node* nodeCursor = &TAB_ROOT_NODE;
				for(i=tokenStart; i<cursor; i++){
					int j;
					int found = 0;
					for(j=0; nodeCursor->children[j] != NULL; j++){
						if(nodeCursor->children[j]->c == eb[i]){
							nodeCursor = nodeCursor->children[j];
							found = 1;
							break;
						}
					}
					if(!found) {nodeCursor = NULL; break;}
				}
				if(nodeCursor == NULL) continue;
				// now we know where we are (hopefully)
				// choose go straight down or list options
				int nodeSize = getNodeSize(nodeCursor);
				if(nodeSize == 0){
					// an odd answer
					arrayList_insert(" ", cursor++, &editingBuffer);
					continue;
				}else if(nodeSize == 1 && nodeCursor->children[0]->c == '\0'){
					// this is the end
					arrayList_insert(" ", cursor++, &editingBuffer);
					continue;
				}else if(nodeSize == 1){
					// go straight down
					gotoBranch(nodeCursor->children[0], &cursor, &editingBuffer);
					cursor = arrayList_getSize(&editingBuffer)-1;
				}else{
					// we need to list options
					fprintf(stderr, "\n");
					printAllOption(nodeCursor);
					originUpdateNeeded = 1;
					cursor = arrayList_getSize(&editingBuffer)-1;
				}
				
				tab_free();
			}else if(currentBuffer[0] == '\n'){// new line
				fprintf(stderr, "\n");
				debug("editing finished. string:\"%s\"\n", (char*)editingBuffer);
				if(!isEmptyStr((char*)editingBuffer)) history_put((char*)editingBuffer);
				process((char*)editingBuffer, env);
				arrayList_free(&editingBuffer);
				lineFinished = 1;
			}else{// normal one char input
				arrayList_insert(currentBuffer, cursor++, &editingBuffer);
				//debug("\nstring:%s\n", (char*)editingBuffer);
			}
		}
	}
}

int isEmptyStr(char* str){
	while((*str) != '\0'){
		if((*str) != ' ') return 0;
		str++;
	}
	return 1;
}

void moveToNextWord(int* cursor, int** editingBuffer){
	int metSpace = 0;
	char currentChar = 0;
	while( (currentChar = *(char*)arrayList_get(*cursor, editingBuffer)) != '\0'){
		if(!metSpace && currentChar == ' '){
			metSpace = 1;
		}else if(metSpace && currentChar != ' '){
			return;
		}
		(*cursor)++;
	}
}

void moveToPrevWord(int* cursor, int** editingBuffer){
	int metSpace = 0;
	int thenNotSpace = 0;
	char currentChar = 0;
	while((*cursor)>0){
		currentChar = *(char*)arrayList_get(*cursor, editingBuffer);
		if(!metSpace && currentChar == ' '){
			metSpace = 1;
		}else if(metSpace && currentChar != ' '){
			thenNotSpace = 1;
		}else if(thenNotSpace && currentChar == ' '){
			(*cursor)++;
			return;
		}
		(*cursor)--;
	}
}

int historyRSearchMode(Point original, int* executeNow){
	gotoxy(original.x, original.y);
	fprintf(stderr, "(reverse-i-search)");
	eraseAfter();
	fflush(stdout);
	original.x+=18;
	int* userTypeBuffer = arrayList_init(100, sizeof(char));
	arrayList_add("\0", &userTypeBuffer);
	int* displayBuffer = arrayList_init(100, sizeof(char));
	char currentBuffer[5];
	int lastSearch = -1;
	
	while(1){
		read(STDIN_FILENO, &currentBuffer, 1);
		if(currentBuffer[0] == '`'){ // todo: temporary exit character
			return -1;
		}else if(currentBuffer[0] == '\033'){//escape sequence
			read(STDIN_FILENO, &(currentBuffer[1]), 1);
			if(currentBuffer[1] != '[') continue;
			read(STDIN_FILENO, &(currentBuffer[2]), 1);
			if(currentBuffer[1] != 'D' || currentBuffer[1] != 'C'){
				return lastSearch;
			}
			continue;
		}else if(currentBuffer[0] == '\n'){
			*executeNow = 1;
			return lastSearch;
		}else if(currentBuffer[0] == 18){// ctrl+r
			
		}else if(currentBuffer[0] == 127){// delete
			lastSearch = -1;
			if(arrayList_getSize(&userTypeBuffer) >= 2)
				arrayList_remove(arrayList_getSize(&userTypeBuffer)-2, &userTypeBuffer);
		}else{
			lastSearch = -1;
			arrayList_insert(&(currentBuffer[0]), arrayList_getSize(&userTypeBuffer)-1, &userTypeBuffer);
		}
		// prepare display buffer
		arrayList_empty(&displayBuffer);
		arrayList_addArray("'':", 3, &displayBuffer);
		int i;
		for(i=arrayList_getSize(&userTypeBuffer)-2; i>=0; i--){
			arrayList_insert(arrayList_get(i, &userTypeBuffer), 1, &displayBuffer);
		}

		char* searchResult;
		if((searchResult = history_rSearch(&lastSearch, (char*)userTypeBuffer)) != NULL){
			arrayList_addArray(searchResult, strlen(searchResult), &displayBuffer);
		}
		arrayList_add("\0", &displayBuffer);
		int cursor = arrayList_getSize(&displayBuffer)-1;
		refreshString(&original, cursor, &displayBuffer);
		arrayList_empty(&displayBuffer);
	}
}

void findCurrentEditingToken(int* start, int end, int** editingBuffer){
	*start = end-1;
	while(1){
		if(*start < 0) {(*start)=0;break;}
		if(*(char*)arrayList_get(*start, editingBuffer) == ' ') {(*start)++;break;}
		
		*start -= 1;
	}
}
