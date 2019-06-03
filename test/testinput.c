#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main()
{
	char buffer1[11] = " ";
	buffer1[10] = '\0';
	char buffer2[11] = " ";
	buffer2[10] = '\0';
	char buffer3[11] = " ";
	buffer3[10] = '\0';
	ssize_t read1;
	ssize_t read2;
	ssize_t read3;
	// read from standinput
	if ((read1 = read(0, (void*) buffer1, 10)) == -1) {
		perror("not able to read from fd 0");
	}
	if ((read2 = read(3, (void*) buffer2, 10)) == -1) {
		perror("not able to read from fd 3\n");
	}
	if ((read3 = read(4, (void*) buffer3, 10)) == -1) {
		perror("not able to read from fd 3\n");
	}
	// now write to the stdout 
	dprintf(1, "%s, %d\n", buffer1, 10);
	dprintf(1, "%s, %d\n", buffer2, 10);
	dprintf(1, "%s, %d\n", buffer3, 10);

}
