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

void fifo_init(struct fifo *f) {
	sem_init(&f->s_r, 0);
	sem_init(&f->s_w, MYFIFO_BUFSIZ);
	sem_init(&f->s_wk, 1);
	sem_init(&f->s_rk, 1);
	void *map = mmap(NULL, sizeof(int) * 2 + MYFIFO_BUFSIZ * sizeof(long), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (map == MAP_FAILED) {
		fprintf(stderr, "Error while creating shared map: %s\n", strerror(errno));
		exit(-1);
	}
	f->start = map;
	*f->start = 0;
	map += sizeof(int);
	f->end = map;
	*f->end = 0;
	map += sizeof(int);
	f->queue = map;
}

void fifo_destroy(struct fifo *f) {
	sem_destroy(&f->s_r);
	sem_destroy(&f->s_w);
	sem_destroy(&f->s_wk);
	sem_destroy(&f->s_rk);
	munmap(f->start, sizeof(int) * 2 + MYFIFO_BUFSIZ * sizeof(long));
}

void fifo_wr(struct fifo *f, unsigned long d) {
	sem_wait(&f->s_wk);
	sem_wait(&f->s_w);
	f->queue[*f->end % MYFIFO_BUFSIZ] = d;
	(*f->end)++;
	sem_inc(&f->s_wk);
	sem_inc(&f->s_r);
}

unsigned long fifo_rd(struct fifo *f) {
	sem_wait(&f->s_rk);
	sem_wait(&f->s_r);
	unsigned long result = f->queue[*(f->start) % MYFIFO_BUFSIZ];
	(*f->start)++;
	sem_inc(&f->s_rk);
	sem_inc(&f->s_w);
	return result;
}