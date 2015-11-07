#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

extern int tas(volatile char *lock);

const int process_count = 10;
const int iteration_count = 100000;

void Problem1() {
	int *share = mmap(NULL, 8, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (share == MAP_FAILED) {
		fprintf(stderr, "Error while creating shared map: %s\n", strerror(errno));
		exit(-1);
	}
	char *lock = (char*)share++;
	*share = 0;
	*lock = 0;
	for (int i = 0; i < process_count; i++) {
		if (fork() == 0) {
			for (int j = 0; j < iteration_count; j++) {
				(*share)++;
			}
			exit(0);
		}
	}
	while (wait(NULL) > 0);
	if (*share == process_count * iteration_count)
		printf("No lock: no synchronization issues detected!\n");
	else
		printf("No lock: synchronization issue detected! Expected: %d Actual: %d\n", process_count * iteration_count, *share);
	*share = 0;
	*lock = 0;
	for (int i = 0; i < process_count; i++) {
		if (fork() == 0) {
			for (int j = 0; j < iteration_count; j++) {
				while (tas(lock) == 1);
				(*share)++;
				*lock = 0;
			}
			exit(0);
		}
	}
	while (wait(NULL) > 0);
	if (*share == process_count * iteration_count)
		printf("Lock: no synchronization issues detected!\n");
	else
		printf("Lock: synchronization issue detected! Expected: %d Actual: %d\n", process_count * iteration_count, *share);
}

int main() {
	Problem1();
}