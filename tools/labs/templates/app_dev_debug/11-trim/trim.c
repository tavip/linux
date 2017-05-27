/**
 * SO, 2016
 * Lab #5
 *
 * Task #7, lin
 *
 * mcheck usage
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char first_name[] = "  Harry";
char last_name[]  = "    Potter";

static char *trim(char *s)
{
	/* We correct trim by using an auxiliary variable aux to save p.
	 * free is issued on aux to free the exact allocated memory.
	 */
	char *aux, *p = malloc(strlen(s) + 1);

	if (p == NULL) {
		fprintf(stderr, "malloc failed\n");
		exit(EXIT_FAILURE);
	}

	aux = p;
	strcpy(p, s);
	while (*p == ' ')
		p++;

	strcpy(s, p);
	free(aux);

	return s;
}

int main(void)
{
	printf("%s %s is learning SO!\n", trim(first_name),
			trim(last_name));
	return 0;
}
