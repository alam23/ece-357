#include <stdio.h>

int main(int argc, char **argv) {
	int i;
	size_t len;
	char *line = NULL;
	for (i = 0; i < argc; i++)
		printf("Parameter: %s\n", *argv++);
	fprintf(stdout, "Standard Output Output\n");
	fprintf(stderr, "Standard Error Output\n");
	while (getline(&line, &len, stdin) != -1)
		printf("Stdin: %s", line);
}
