/**
  * SO, 2016
  * Lab #5
  *
  * Task #9, lin
  *
  * Working with the stack
  */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"

static char *myfun(void)
{
	static char lab_so[6]; /* static / dinamic / global */

	sprintf(lab_so, "%s-%s", "Lab", "4");
	printf("myfun: %s\n", lab_so);

	return &lab_so[0];
}

int main(void)
{
	char *str;

	str = myfun();
	printf("main first: %s\n", str);

	printf("main second: ");
	printf("%s\n", str);

	return 0;
}
