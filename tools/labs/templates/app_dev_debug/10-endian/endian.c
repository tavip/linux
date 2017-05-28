/**
  * SO, 2016
  * Lab #5
  *
  * Task #8, lin
  *
  * Endianess
  */
#include <stdlib.h>
#include <stdio.h>

int main(void)
{
	int i;
	unsigned int n = 0xDEADBEEF;
	unsigned char *w = (unsigned char *)&n;

	/* TODO 0/2: print w byte by byte */
	for (i = 0; i < 4; i++)
		printf("%x ", w[i]);

	printf("\n");

	return 0;
}

