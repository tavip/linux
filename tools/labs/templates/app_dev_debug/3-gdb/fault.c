/**
  * Application development and debugging lab
  *
  * Task 03 - Use of gdb to solve "Segmentation fault" problems
  */

#include <stdlib.h>
#include <stdio.h>

int main(void)
{
	char *buf;

	buf = malloc(1<<31);

	printf("Give input string:");
	fgets(buf, 1024, stdin);
	printf("\n\nString is %s\n", buf);

	return 0;
}
