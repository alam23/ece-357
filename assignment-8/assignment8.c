#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define LOOP_COUNT 100000000
#define BILLION 1000000000

void emptycall() {
}

int sys_call() {
	return getpid();
}

int main() {
	struct timespec start, stop;
	double delta, delta_user;
	// Part A: Empty loop
	clock_gettime(CLOCK_REALTIME, &start);
	for(int i = 0; i < LOOP_COUNT; i++) {
	}
	clock_gettime(CLOCK_REALTIME, &stop);
	if (stop.tv_nsec - start.tv_nsec < 0) {
		delta = (stop.tv_sec - start.tv_sec - 1) * BILLION;
		delta += BILLION + stop.tv_nsec - start.tv_nsec;
	} else {
		delta = (stop.tv_sec - start.tv_sec) * BILLION;
		delta += stop.tv_nsec - start.tv_nsec;
	}
	printf("Empty loop takes %0.3lf nanoseconds.\n", delta / LOOP_COUNT);
	// Part B: Empty call
	clock_gettime(CLOCK_REALTIME, &start);
	for(int i = 0; i < LOOP_COUNT; i++) {
		emptycall();
	}
	clock_gettime(CLOCK_REALTIME, &stop);
	if (stop.tv_nsec - start.tv_nsec < 0) {
		delta = (stop.tv_sec - start.tv_sec - 1) * BILLION;
		delta += BILLION + stop.tv_nsec - start.tv_nsec;
	} else {
		delta = (stop.tv_sec - start.tv_sec) * BILLION;
		delta += stop.tv_nsec - start.tv_nsec;
	}
	printf("Empty call takes %0.3lf nanoseconds.\n", delta / LOOP_COUNT);
	delta_user = delta / LOOP_COUNT;
	// Part C: Syscall
	clock_gettime(CLOCK_REALTIME, &start);
	for(int i = 0; i < LOOP_COUNT; i++) {
		sys_call();
	}
	clock_gettime(CLOCK_REALTIME, &stop);
	if (stop.tv_nsec - start.tv_nsec < 0) {
		delta = (stop.tv_sec - start.tv_sec - 1) * BILLION;
		delta += BILLION + stop.tv_nsec - start.tv_nsec;
	} else {
		delta = (stop.tv_sec - start.tv_sec) * BILLION;
		delta += stop.tv_nsec - start.tv_nsec;
	}
	printf("Syscall takes %0.3lf nanoseconds.\n", delta / LOOP_COUNT - delta_user);
}