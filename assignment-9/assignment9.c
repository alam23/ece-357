#include <stdio.h>
#include <unistd.h>
#include "sched.h"

void init() {
	for (int i = 0; i < 5; i++) {
		if (sched_fork() == 0) {
			printf("Child: %d\n", current->pid);
			sched_nice(i);
			for (int i = 0; i < 10000000; i++);
			printf("Child: %d Complete\n", current->pid);
			sched_exit(i);
			printf("Am I Out Again? <%d>\n", current->pid);
		}
	}
	for (int i = 0; i < 5; i++) {
		int returncode;
		int cc = sched_wait(&returncode);
		printf("Function Return: %d, Wait Code: %d\n", cc, returncode);
	}
	int returncode;
	int cc = sched_wait(&returncode);
	printf("Function Return: %d, Wait Code: %d\n", cc, returncode);
	printf("Done!\n");
	sched_ps();
	sched_exit(0);
}

int main() {
	sched_init(&init);
	printf("Holy poopypants on a sandwich how'd I get here\n");
	return 0;
}