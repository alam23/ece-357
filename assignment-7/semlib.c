#include <sys/mman.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "semlib.h"

extern int tas(volatile char *lock);

int process_id;
int SigUsr_Trigger = 0;

void sem_init(struct sem *s, int count) {
	void *map = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (map == MAP_FAILED) {
		fprintf(stderr, "Error while creating shared map: %s\n", strerror(errno));
		exit(-1);
	}
	s->lock = map;
	map += sizeof(char);
	s->count = map;
	*(s->count) = count;
	map += sizeof(int);
	s->process_sleeping = map;
	map += sizeof(char) * (N_PROC + 1);
	s->process_id = map;
}

void sem_destroy(struct sem *s) {
	munmap(s->lock, 4096);
}

int sem_try(struct sem *s) {
	while (tas(s->lock) == 1);
	int return_value = *s->count > 0 ? 1 : 0;
	if (*s->count > 0)
		(*s->count)--;
	*s->lock = 0;
	return return_value;
}

void sem_wait(struct sem *s) {
	while (1) {
		while (tas(s->lock) == 1);
		if (*s->count > 0) {
			(*s->count)--;
			*s->lock = 0;
			break;
		} else {
			if (s->process_id[process_id] == 0)
				s->process_id[process_id] = getpid();
			s->process_sleeping[process_id] = 1;
			SigUsr_Trigger = 0;
			*s->lock = 0;
			sigset_t mask, oldmask;
			sigemptyset(&mask);
			sigaddset(&mask, SIGUSR1);
			sigprocmask(SIG_BLOCK, &mask, &oldmask);
			if (!SigUsr_Trigger)
				sigsuspend(&oldmask);
			sigprocmask(SIG_UNBLOCK, &mask, NULL);
		}
	}
}

void sem_inc(struct sem *s) {
	while (tas(s->lock) == 1);
	(*s->count)++;
	for (int i = 0; i < N_PROC + 1; i++) {
		if (s->process_sleeping[i]) {
			s->process_sleeping[i] = 0;
			kill(s->process_id[i], SIGUSR1);
		}
	}
	*s->lock = 0;
}