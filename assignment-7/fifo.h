#ifndef FIFO_H
#define FIFO_H

#define MYFIFO_BUFSIZ 4000

struct fifo {
	struct sem s_r, s_w, s_rk, s_wk;
	int *start, *end;
	unsigned long *queue;
};

void fifo_init(struct fifo *f);
void fifo_destroy(struct fifo *f);
void fifo_wr(struct fifo *f,unsigned long d);
unsigned long fifo_rd(struct fifo *f);

#endif