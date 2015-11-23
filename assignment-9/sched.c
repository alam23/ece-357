#include "sched.h"

int sched_createtask(int ppid);
void sched_switch(int state);
void sched_queue(sched_proc *process, int status);
sched_proc* sched_dequeue();
void adjstack(void *lim0,void *lim1,unsigned long adj);

int sched_init(void (*init_fn)()) {
	active = (sched_waitq*)malloc(sizeof(sched_waitq));
	memset(active, 0, sizeof(sched_waitq));
	expired = (sched_waitq*)malloc(sizeof(sched_waitq));
	memset(expired, 0, sizeof(sched_waitq));
	for (int i = 0; i < 140; i++) {
		active->first[i] = malloc(sizeof(sched_proc));
		memset(active->first[i], 0, sizeof(sched_proc));
		active->last[i] = active->first[i];
		expired->first[i] = malloc(sizeof(sched_proc));
		memset(expired->first[i], 0, sizeof(sched_proc));
		expired->last[i] = expired->first[i];
	}
	// Start tick
	struct itimerval new;
	new.it_interval.tv_usec = 1000;
	new.it_interval.tv_sec = 0;
	new.it_value.tv_usec = 1000;
	new.it_value.tv_sec = 0;
	signal(SIGVTALRM, &sched_tick);
	if (setitimer(ITIMER_VIRTUAL, &new, NULL) < 0)
		return 0;
	// Create task 1 and jump to it
	int init = sched_createtask(0);
	struct savectx initctx;
    initctx.regs[JB_SP] = tasks[init].stack;    
    initctx.regs[JB_PC] = init_fn;  
    current = &tasks[init];
    restorectx(&initctx, 0);
}

int sched_fork() {
	int newpid = sched_createtask(current->pid);
	int retcode = newpid;
	sched_proc *newproc = &tasks[newpid];
	memcpy(newproc->stack - STACK_SIZE, current->stack - STACK_SIZE, STACK_SIZE);
	adjstack(newproc->stack - STACK_SIZE, newproc->stack, newproc->stack - current->stack);
	sched_queue(newproc, SCHED_READY);
	if (savectx(&newproc->ctx)) {
		retcode = 0;
	}
	newproc->ctx.regs[JB_BP] += newproc->stack - current->stack;
    newproc->ctx.regs[JB_SP] += newproc->stack - current->stack;  
	return retcode;
}

void sched_exit(int code) {
	sigset_t block;
	sigfillset(&block);
	sigprocmask(SIG_BLOCK, &block, NULL);
	// Increase cpu time since we are switching
	(current->cputime)++;
	current->exitcode = code;
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
	// Find a zombie and return OR go to bed
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

void sched_nice(int niceval) {
	if (niceval < -20)
		niceval = -20;
	if (niceval > 19)
		niceval = 19;
	if (current->priority < 120 + niceval)
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
	printf("PID\tPPID\tSTATE\tSTACK\t\tPRIORITY\tDYNAMIC\tTICKS\n");
	for (int i = 1; i < SCHED_NPROC; i++) {
		if (tasks[i].status != SCHED_NONE) {
			printf("%d\t", tasks[i].pid);
			printf("%d\t", tasks[i].ppid);
			printf("%d\t", tasks[i].status);
			printf("%p\t", tasks[i].stack);
			printf("%d\t\t", tasks[i].priority);
			printf("%d\t", tasks[i].priority - min(tasks[i].bonus, 10));
			printf("%d\n", tasks[i].cputime);
		}
	}
}

void sched_switch(int state) {
	//printf("Task Switch From Process(State: %d): PID: %d\n", state, current->pid);
	// Switch out current process
	if (current != NULL)
		sched_queue(current, state);
	// Get best process to switch in
	sched_proc *best = sched_dequeue();
	if (best == NULL) {
		sched_waitq *swap;
		swap = active;
		active = expired;
		expired = swap;
		best = sched_dequeue();
		if (best == NULL) {
			// Don't look at this part. Idk how to handle idle process so ima just exit the program :D
			current = NULL;
			exit(0);
			return;
		}
	}
	current = best;
	//printf("Task Switching To PID: %d\n", current->pid);
	if (current->priority < 120)
		current->quantum = (140 - current->priority) * 20;
	else
		current->quantum = (140 - current->priority) * 5;
	sigset_t block;
	sigfillset(&block);
	sigprocmask(SIG_UNBLOCK, &block, NULL);
	restorectx(&current->ctx, 1);
}

void sched_tick(int signum) {
	sigset_t block;
	sigfillset(&block);
	sigprocmask(SIG_BLOCK, &block, NULL);
	for (int i = 1; i < SCHED_NPROC; i++)
		tasks[i].bonus++;
	if (current != NULL) {
		(current->cputime)++;
		(current->quantum)--;
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
	int priority = process->priority - min(process->bonus, 10);
	process->bonus = 0;

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
				return cur;
			}
		}
	}
	return NULL;
}

int sched_createtask(int ppid) {
	for (int i = 1; i < SCHED_NPROC; i++) {
		if (tasks[i].status == SCHED_NONE) {
			tasks[i].status = SCHED_READY;
			tasks[i].pid = i;
			tasks[i].ppid = ppid;
			tasks[i].priority = 120;
			tasks[i].cputime = 0;
			tasks[i].stack = mmap(0, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
			if (tasks[i].stack == MAP_FAILED)
				perror("mmap failed");
			tasks[i].stack += STACK_SIZE;
			return i;
		}
	}
}

void sched_destroytask(int pid) {
	tasks[pid].status = SCHED_NONE;
	munmap(tasks[pid].stack, STACK_SIZE);
}