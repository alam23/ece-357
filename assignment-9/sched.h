#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/mman.h>

#include "savectx.h"

#define SCHED_NPROC 256
#define SCHED_NONE 0 // Task not defined
#define SCHED_READY 1
#define SCHED_RUNNING 2
#define SCHED_SLEEPING 3
#define SCHED_ZOMBIE 4
#define STACK_SIZE 65536

typedef struct sched_proc sched_proc;

struct sched_proc {
	sched_proc *prev, *next; // Double Linked List
	int status; // Status
	int pid; // Task id
	int ppid; // Parent task id
	int exitcode; // Exit code
	int priority; // 0-139 Priority
	int dynamic; // Not important
	int quantum; // Quantum remaining
	int bonus; // CPU time spent sleeping
	int cputime; // Total CPU time consumed
	void *stack; // Task stack
	void *wait; // Wait Queue Location
	struct savectx ctx; // Task context
};

typedef struct sched_waitq sched_waitq;

struct sched_waitq {
	int count[140];
	sched_proc *first[140];
	sched_proc *last[140];
};

struct sched_waitq1 { // Going full ghetto and just keep 140 ids
	int id[140];
};

sched_proc *current;
sched_proc tasks[SCHED_NPROC];
sched_waitq *active;
sched_waitq *expired;

int sched_init(void (*init_fn)());
void sched_tick(int signum);
int sched_getpid();
int sched_getppid();
int sched_gettick();
void sched_exit(int code);
int sched_fork();
void sched_nice(int niceval);
int sched_wait(int *exit_code);
void sched_ps();
void sched_sleep(struct sched_waitq1 *waitq);
void sched_wakeup(struct sched_waitq1 *waitq);