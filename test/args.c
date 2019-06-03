#include <stdio.h>

int main(int argc, char *argv[])
{
	int count;

	dprintf(1,"This program was called with \"%s\".\n", argv[0]);
	dprintf(3,"This program was called with \"%s\".\n", argv[0]);
	dprintf(4,"This program was called with \"%s\".\n", argv[0]);

	if (argc > 0) {
		for (count = 0; count < argc; count++) {
			dprintf(1, "1:argv[%d] = %s\n", count, argv[count]);
			dprintf(3, "3:argv[%d] = %s\n", count, argv[count]);
			dprintf(4, "4:argv[%d] = %s\n", count, argv[count]);
		}
	} else {
		printf("The command had no other arguments.\n");
	}

	return 0;
}
