#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "sched.h"

struct sched_waitq1 wait1, wait2;

void signal1(int signum) {
	sched_wakeup(&wait1);
}

void init() {
	signal(SIGUSR1, &signal1);
	printf("[Init][Pid: %d] Scheduler Testing Program\n", sched_getpid());
	sched_nice(-20);
	for (int i = 0; i < 15; i++) {
		if (sched_fork() == 0) {
			printf("[Child][Pid: %d] Parent Pid: %d\n", sched_getpid(), sched_getppid());
			sched_nice(-19 + i);
			struct timespec start, stop;
			clock_gettime(CLOCK_REALTIME, &start);
			for (long int j = 0; j < 1000000000; j++);
			clock_gettime(CLOCK_REALTIME, &stop);
			printf("[Child][Pid: %d] Execution Complete. Took %ld Seconds.\n", sched_getpid(), (long int)(stop.tv_sec - start.tv_sec));
			sched_exit(i);
			printf("[Child][Pid: %d] This Will Never Get Executed\n", sched_getpid());
		}
	}
	printf("[Init][Pid: %d] Process Information\n", sched_getpid());
	sched_ps();
	for (int i = 0; i < 15; i++) {
		int returncode;
		int cc = sched_wait(&returncode);
		printf("[Init][Pid: %d] Child Returned [%d] With Exit Code [%d]\n", sched_getpid(), cc, returncode);
	}
	int returncode;
	int cc = sched_wait(&returncode);
	printf("[Init][Pid: %d] Calling Wait With No Children Returns [%d]\n", sched_getpid(), cc);
	sched_sleep(&wait1);
	for (int i = 0; i < 15; i++) {
		if (sched_fork() == 0) {
			printf("[Child][Pid: %d] Parent Pid: %d\n", sched_getpid(), sched_getppid());
			sched_nice(-19 + i);
			if (i % 2 == 1)
				sched_sleep(&wait1);
			else
				sched_sleep(&wait2);
			printf("[Child][Pid: %d] Execution Complete.\n", sched_getpid());
			sched_exit(i);
		}
	}
	for (int i = 0; i < 1000000000; i++);
	printf("[Init][Pid: %d] Process Information\n", sched_getpid());
	sched_ps();
	printf("Wakeup 2\n");
	sched_wakeup(&wait2);
	printf("Wakeup 1\n");
	sched_wakeup(&wait1);
	for (int i = 0; i < 15; i++) {
		int returncode;
		int cc = sched_wait(&returncode);
		printf("[Init][Pid: %d] Child Returned [%d] With Exit Code [%d]\n", sched_getpid(), cc, returncode);
	}
	printf("[Init][Pid: %d] Exiting Testing Program. Passing Control Back To Idle\n", sched_getpid());
	sched_exit(0);
}

int main() {
	sched_init(&init);
	printf("[Bug] Should Not Be Here\n");
	return 0;
}