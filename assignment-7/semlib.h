#ifndef SEMLIB_H
#define SEMLIB_H

#define N_PROC 64

struct sem {
	char *lock;
	int *count;
	char *process_sleeping;
	int *process_id;
};

void sem_init(struct sem *s, int count);
void sem_destroy(struct sem *s);
int sem_try(struct sem *s);
void sem_wait(struct sem *s);
void sem_inc(struct sem *s);

#endif