#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "semlib.h"
#include "fifo.h"

#define iteration_count 100000

extern int tas(volatile char *lock);

extern int process_id;
extern int SigUsr_Trigger;

void Problem1() {
	printf("Problem 1 - Testing TAS\n");
	int *share = mmap(NULL, 5, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (share == MAP_FAILED) {
		fprintf(stderr, "Error while creating shared map: %s\n", strerror(errno));
		exit(-1);
	}
	char *lock = (char*)share++;
	*share = 0;
	printf("Part 1 - No Locks\n");
	for (process_id = 0; process_id < N_PROC; process_id++) {
		if (fork() == 0) {
			for (int i = 0; i < iteration_count; i++)
				(*share)++;
			exit(0);
		}
	}
	while (wait(NULL) > 0);
	if (*share == N_PROC * iteration_count)
		printf("No synchronization issues detected!\n");
	else
		printf("Synchronization issue detected! Expected: %d Actual: %d\n", N_PROC * iteration_count, *share);

	printf("Part 2 - TAS Lock\n");
	*share = 0;
	*lock = 0;
	for (process_id = 0; process_id < N_PROC; process_id++) {
		if (fork() == 0) {
			for (int i = 0; i < iteration_count; i++) {
				while (tas(lock) == 1);
				(*share)++;
				*lock = 0;
			}
			exit(0);
		}
	}
	while (wait(NULL) > 0);
	if (*share == N_PROC * iteration_count)
		printf("No synchronization issues detected!\n");
	else
		printf("Synchronization issue detected! Expected: %d Actual: %d\n", N_PROC * iteration_count, *share);
	munmap(share, 5);
}

void Problem2() {
	printf("Problem 2 - Implementing Semaphores\n");
	int *share = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (share == MAP_FAILED) {
		fprintf(stderr, "Error while creating shared map: %s\n", strerror(errno));
		exit(-1);
	}
	*share = 0;
	printf("Part 1 - No Locks\n");
	for (process_id = 0; process_id < N_PROC; process_id++) {
		if (fork() == 0) {
			for (int i = 0; i < iteration_count; i++)
				(*share)++;
			exit(0);
		}
	}
	while (wait(NULL) > 0);
	if (*share == N_PROC * iteration_count)
		printf("No synchronization issues detected!\n");
	else
		printf("Synchronization issue detected! Expected: %d Actual: %d\n", N_PROC * iteration_count, *share);

	printf("Part 2 - Semaphore Lock\n");
	*share = 0;
	struct sem s;
	sem_init(&s, 1);
	for (process_id = 0; process_id < N_PROC; process_id++) {
		if (fork() == 0) {
			for (int i = 0; i < iteration_count; i++) {
				sem_wait(&s);
				(*share)++;
				sem_inc(&s);
			}
			exit(0);
		}
	}
	while (wait(NULL) > 0);
	if (*share == N_PROC * iteration_count)
		printf("No synchronization issues detected!\n");
	else
		printf("Synchronization issue detected! Expected: %d Actual: %d\n", N_PROC * iteration_count, *share);
	sem_destroy(&s);
	munmap(share, 4);
}

void Problem4() {
	printf("Problem 4 - Testing FIFO Pipe\n");
	struct fifo f;
	fifo_init(&f);
	printf("Part 1 - Single Reader, Single Writer\n");
	process_id = 0;
	if (fork() == 0) {
		for(int i = 0; i < iteration_count; i++)
			fifo_wr(&f, i);
		exit(0);
	}
	process_id = 1;
	int read_error = 0;
	for (int i = 0; i < iteration_count; i++) {
		if (fifo_rd(&f) != i) {
			read_error = 1;
			break;
		}
	}
	if (read_error) {
		printf("Read error detected!\n");
	} else {
		printf("No read errors detected!\n");
	}
	fifo_destroy(&f);

	printf("Part 2 - Single Reader, Multiple Writer\n");
	fifo_init(&f);
	for (process_id = 0; process_id < N_PROC; process_id++) {
		if (fork() == 0) {
			for (int i = 0; i < iteration_count; i++) {
				fifo_wr(&f, ((unsigned long)process_id << 32) + i);
			}
			exit(0);
		}
	}
	int counter[N_PROC];
	memset(&counter, 0, sizeof(counter));
	read_error = 0;
	for (int i = 1; i <= N_PROC * iteration_count; i++) {
		unsigned long rd = fifo_rd(&f);
		int pid = rd >> 32;
		int number = rd & 0xFFFFFFFF;
		fprintf(stderr, "Current Iteration: %d/%d\r", i, N_PROC * iteration_count);
		if (counter[pid]++ != number) {
			read_error = 1;
			break;
		}
	}
	printf("\n");
	if (read_error) {
		printf("Read error detected!\n");
	} else {
		printf("No read errors detected!\n");
	}
}

void SigUsrHandler(int signum) {
	SigUsr_Trigger = 1;
}

int main() {
	if (signal(SIGUSR1, &SigUsrHandler) == SIG_ERR) {
		fprintf(stderr, "Error while creating signal handler: %s\n", strerror(errno));
		return -1;
	}
	Problem1();
	Problem2();
	Problem4();
	return 0;
}