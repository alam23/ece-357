#include "sched.h"

int NEED_RESCHED = 0;

void sched_initializetask(int pid, int ppid);
int sched_createtask(int ppid);
void sched_destroytask(int pid);
void adjstack(void *lim0,void *lim1,unsigned long adj);

void sched_switch(int state);
void sched_queue(sched_proc *process, int status);
sched_proc* sched_dequeue();

void idle() {
	while(1);
}

void sched_int(int signum) {
	sched_ps();
	exit(0);
}

int sched_init(void (*init_fn)()) {
	// Allocate active and expired queues
	active = (sched_waitq*)malloc(sizeof(sched_waitq));
	if (active == NULL)
		perror("active malloc failed");
	memset(active, 0, sizeof(sched_waitq));
	expired = (sched_waitq*)malloc(sizeof(sched_waitq));
	if (expired == NULL)
		perror("expired malloc failed");
	memset(expired, 0, sizeof(sched_waitq));
	for (int i = 0; i < 140; i++) {
		// Too lazy to handle null cases, thus the first element is always a dummy
		active->first[i] = malloc(sizeof(sched_proc));
		memset(active->first[i], 0, sizeof(sched_proc));
		active->last[i] = active->first[i];
		expired->first[i] = malloc(sizeof(sched_proc));
		memset(expired->first[i], 0, sizeof(sched_proc));
		expired->last[i] = expired->first[i];
	}
	// Start alarm
	struct itimerval new;
	new.it_interval.tv_usec = 1000;
	new.it_interval.tv_sec = 0;
	new.it_value.tv_usec = 1000;
	new.it_value.tv_sec = 0;
	signal(SIGVTALRM, &sched_tick);
	signal(SIGABRT, &sched_int);
	signal(SIGINT, &sched_int);
	if (setitimer(ITIMER_VIRTUAL, &new, NULL) < 0) {
		perror("setitimer failed");
		return 0;
	}
	// Initialize idle task
	sched_initializetask(0, 0);
	tasks[0].ctx.regs[JB_SP] = tasks[0].stack;
    tasks[0].ctx.regs[JB_PC] = idle;
	// Create task 1, set as current task, and jump to it
	int init = sched_createtask(0);
	struct savectx initctx;
    initctx.regs[JB_SP] = tasks[init].stack;
    initctx.regs[JB_PC] = init_fn;
    current = &tasks[init];
    restorectx(&initctx, 0);
}

int sched_fork() {
	sigset_t block;
	sigfillset(&block);
	sigprocmask(SIG_BLOCK, &block, NULL);
	int newpid = sched_createtask(current->pid);
	int return_value = newpid;
	sched_proc *newproc = &tasks[newpid];
	// Inherit priority
	newproc->priority = current->priority;
	// Copy and fix stack
	memcpy(newproc->stack - STACK_SIZE, current->stack - STACK_SIZE, STACK_SIZE);
	adjstack(newproc->stack - STACK_SIZE, newproc->stack, newproc->stack - current->stack);
	sched_queue(newproc, SCHED_READY);
	if (savectx(&newproc->ctx)) {
		return_value = 0;
	} else {
		// Fix base and stack pointers
		newproc->ctx.regs[JB_BP] += newproc->stack - current->stack;
    	newproc->ctx.regs[JB_SP] += newproc->stack - current->stack;
    }
    sigprocmask(SIG_UNBLOCK, &block, NULL);
	return return_value;
}

void sched_exit(int code) {
	sigset_t block;
	sigfillset(&block);
	sigprocmask(SIG_BLOCK, &block, NULL);
	// Increase cpu time since we are switching
	(current->cputime)++;
	current->exitcode = code;
	// Mark sleeping parents as ready
	struct sched_proc *cur, *prev, *next;
	for (int i = 0; i < 140; i++) {
		if (active->count[i] == 0)
			continue;
		cur = active->first[i];
		while ((cur = cur->next) != NULL)
			if (cur->status == SCHED_SLEEPING && cur->pid == current->ppid) {
				cur->status = SCHED_READY;
			}
	}
	for (int i = 0; i < 140; i++) {
		if (expired->count[i] == 0)
			continue;
		cur = expired->first[i];
		while ((cur = cur->next) != NULL)
			if (cur->status == SCHED_SLEEPING && cur->pid == current->ppid) {
				cur->status = SCHED_READY;
			}
	}
	sched_switch(SCHED_ZOMBIE);
}

int sched_wait(int *exit_code) {
	sigset_t block;
	sigfillset(&block);
	sigprocmask(SIG_BLOCK, &block, NULL);
	// Increase cpu time since we are switching
	(current->cputime)++;
	// Find a zombie and return OR go back to sleep
	int childleft;
	struct sched_proc *cur, *prev, *next;
	do {
		childleft = 0;
		for (int i = 0; i < 140; i++) {
			if (active->count[i] == 0)
				continue;
			cur = active->first[i];
			while ((cur = cur->next) != NULL) {
				if (cur->ppid == current->pid)
					childleft = 1;
				if (cur->status == SCHED_ZOMBIE && cur->ppid == current->pid) {
					prev = cur->prev; next = cur->next;
					prev->next = cur->next;
					if (next != NULL)
						next->prev = prev;
					active->last[i] = next;
					if (active->last[i] == NULL)
						active->last[i] = prev;
					active->count[i]--;
					*exit_code = cur->exitcode;
					sched_destroytask(cur->pid);
					sigprocmask(SIG_UNBLOCK, &block, NULL);
					return 0;
				}
			}
		}
		for (int i = 0; i < 140; i++) {
			if (expired->count[i] == 0)
				continue;
			cur = expired->first[i];
			while ((cur = cur->next) != NULL) {
				if (cur->ppid == current->pid)
					childleft = 1;
				if (cur->status == SCHED_ZOMBIE && cur->ppid == current->pid) {
					prev = cur->prev; next = cur->next;
					prev->next = cur->next;
					if (next != NULL)
						next->prev = prev;
					expired->last[i] = next;
					if (expired->last[i] == NULL)
						expired->last[i] = prev;
					expired->count[i]--;
					*exit_code = cur->exitcode;
					sched_destroytask(cur->pid);
					sigprocmask(SIG_UNBLOCK, &block, NULL);
					return 0;
				}
			}
		}
		if (childleft) {
			if (!savectx(&current->ctx)) {
				sigprocmask(SIG_UNBLOCK, &block, NULL);
				sched_switch(SCHED_SLEEPING);
				sigprocmask(SIG_BLOCK, &block, NULL);
			}
		}
	} while(childleft);
	sigprocmask(SIG_UNBLOCK, &block, NULL);
	return -1;
}

void sched_sleep(struct sched_waitq1 *waitq) {
	for (int i = 0; i < 140; i++) {
		if (waitq->id[i] == 0) {
			waitq->id[i] = current->pid;
			break;
		}
	}
	sigset_t block;
	sigfillset(&block);
	sigprocmask(SIG_BLOCK, &block, NULL);
	// Increase cpu time since we are switching
	(current->cputime)++;
	current->wait = waitq;
	// Schedule
	if (!savectx(&current->ctx)) {
		sigprocmask(SIG_UNBLOCK, &block, NULL);
		sched_switch(SCHED_SLEEPING);
	}
}

void sched_wakeup(struct sched_waitq1 *waitq) {
	for (int i = 0; i < 140; i++) {
		if (waitq->id[i] != 0) {
			tasks[waitq->id[i]].status = SCHED_READY;
			tasks[waitq->id[i]].wait = NULL;
			waitq->id[i] = 0;
		}
	}
	sigset_t block;
	sigfillset(&block);
	sigprocmask(SIG_BLOCK, &block, NULL);
	// Increase cpu time since we are switching
	(current->cputime)++;
	// Schedule
	if (!savectx(&current->ctx)) {
		sigprocmask(SIG_UNBLOCK, &block, NULL);
		sched_switch(SCHED_READY);
	}
}

void sched_nice(int niceval) {
	if (niceval < -20)
		niceval = -20;
	if (niceval > 19)
		niceval = 19;
	current->priority = 120 + niceval;
}

int sched_getpid() {
	return current->pid;
}

int sched_getppid() {
	return current->ppid;
}

int sched_gettick() {
	return current->cputime;
}

int min(int i1, int i2) {
	if (i1 < i2)
		return i1;
	else
		return i2;
}

void sched_ps() {
	printf("PID\tPPID\tSTATE\t\tSTACK\t\tWAIT\t\tPRIORITY\tDYNAMIC\tTICKS\n");
	for (int i = 1; i < SCHED_NPROC; i++) {
		if (tasks[i].status != SCHED_NONE) {
			printf("%d\t", tasks[i].pid);
			printf("%d\t", tasks[i].ppid);
			switch (tasks[i].status) {
				case SCHED_READY:
					printf("READY\t\t");
					break;
				case SCHED_RUNNING:
					printf("RUNNING\t\t");
					break;
				case SCHED_SLEEPING:
					printf("SLEEPING\t");
					break;
				case SCHED_ZOMBIE:
					printf("ZOMBIE\t\t");
					break;
			}
			printf("%p\t", tasks[i].stack);
			if (tasks[i].status == SCHED_SLEEPING && tasks[i].wait != 0)
				printf("%p\t", tasks[i].wait);
			else
				printf("\t\t");
			printf("%d\t\t", tasks[i].priority);
			printf("%d\t", tasks[i].priority - min(tasks[i].bonus / 500, 10));
			printf("%d\n", tasks[i].cputime);
		}
	}
}

void sched_switch(int state) {
	sigset_t block;
	sigfillset(&block);
	// Queue current process
	if (current != NULL && current->pid != 0)
		sched_queue(current, state);
	// Find best process from active queue
	sched_proc *best = sched_dequeue();
	if (best == NULL) {
		// All out of active processes, so flip queues
		sched_waitq *swap;
		swap = active;
		active = expired;
		expired = swap;
		best = sched_dequeue();
		if (best == NULL) {
			// Processor is idle, restore the idle task
			current = &tasks[0];
			sigprocmask(SIG_UNBLOCK, &block, NULL);
			restorectx(&current->ctx, 1);
			return;
		}
	}
	current = best;
	current->status = SCHED_RUNNING;
	// Allocate quantum (slightly different than linux kernel)
	if (current->dynamic < 120)
		current->quantum = (140 - current->dynamic) * 5;
	else
		current->quantum = (140 - current->dynamic);
	// Start execution
	sigprocmask(SIG_UNBLOCK, &block, NULL);
	restorectx(&current->ctx, 1);
}

void sched_tick(int signum) {
	sigset_t block;
	sigfillset(&block);
	sigprocmask(SIG_BLOCK, &block, NULL);
	// Increase bonus for every tick spent not executing
	for (int i = 1; i < SCHED_NPROC; i++)
		tasks[i].bonus++;
	if (current != NULL) {
		(current->cputime)++;
		(current->quantum)--;
		// Reschedule Flag
		if (NEED_RESCHED) {
			NEED_RESCHED = 0;
			current->quantum = 0;
		}
		if (current->quantum <= 0) {
			// Save context
			if (savectx(&current->ctx)) {
				sigprocmask(SIG_UNBLOCK, &block, NULL);
				return;
			}
			sched_switch(SCHED_READY);
		}
	}
	sigprocmask(SIG_UNBLOCK, &block, NULL);
}

void sched_queue(sched_proc *process, int status) {
	// Generate dynamic priority
	int priority = process->priority - min(process->bonus / 500, 10);
	process->bonus = 0;
	// Jank double linked list implementation
	process->next = NULL;
	process->status = status;
	expired->count[priority]++;
	sched_proc *last = expired->last[priority];
	last->next = process;
	process->prev = last;
	process->next = NULL;
	expired->last[priority] = process;
}

sched_proc* sched_dequeue() {
	// Removes best process from active queue and return
	struct sched_proc *cur, *prev, *next;
	for (int i = 0; i < 140; i++) {
		if (active->count[i] == 0)
			continue;
		cur = active->first[i];
		while ((cur = cur->next) != NULL) {
			if (cur->status == SCHED_READY) {
				prev = cur->prev; next = cur->next;
				prev->next = cur->next;
				if (next != NULL)
					next->prev = prev;
				else
					active->last[i] = prev;
				active->count[i]--;
				cur->dynamic = i;
				return cur;
			}
		}
	}
	return NULL;
}

void sched_initializetask(int pid, int ppid) {
	tasks[pid].status = SCHED_READY;
	tasks[pid].pid = pid;
	tasks[pid].ppid = ppid;
	tasks[pid].priority = 120;
	tasks[pid].cputime = 0;
	tasks[pid].stack = mmap(0, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	if (tasks[pid].stack == MAP_FAILED)
		perror("mmap failed");
	tasks[pid].stack += STACK_SIZE;
}

int sched_createtask(int ppid) {
	for (int i = 1; i < SCHED_NPROC; i++) {
		if (tasks[i].status == SCHED_NONE) {
			sched_initializetask(i, ppid);
			return i;
		}
	}
}

void sched_destroytask(int pid) {
	tasks[pid].status = SCHED_NONE;
	if (munmap(tasks[pid].stack - STACK_SIZE, STACK_SIZE) < 0)
		perror("munmap failed");
}