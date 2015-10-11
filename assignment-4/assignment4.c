#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include <setjmp.h>

int dwFilesProcessed = 0, dwBytesProcessed = 0;
int grep_pid, less_pid;
jmp_buf int_jb;

void int_handler(int signo) {
	if (signo == SIGPIPE) {
		siglongjmp(int_jb, 1);
	}
	if (signo == SIGINT) {
		// I need to kill less so less can wipe the screen
		if (!kill(less_pid, SIGTERM))
			waitpid(less_pid, NULL, 0);
		else
			fprintf(stderr, "Error while killing less: %s\n", strerror(errno));
		// Less disables echo so I need to re-enable it
		struct termios tp;
		if (!tcgetattr(STDIN_FILENO, &tp)) {
			tp.c_lflag |= ECHO;
			if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &tp) < 0)
				fprintf(stderr, "Error while setting terminal attributes: %s\n", strerror(errno));
		} else {
			fprintf(stderr, "Error while getting terminal attributes: %s\n", strerror(errno));
		}
		printf("\nFiles Processed: %d, Bytes Processed: %d!\n", dwFilesProcessed, dwBytesProcessed);
		exit(1);
	}
}

void eclose(int fd) {
	if (close(fd))
		fprintf(stderr, "Error while closing %d: %s\n", fd, strerror(errno));
}

int main (int argc, char **argv) {
	char buf[4096];
	char *pattern;
	int fd_in, bytes_read;
	int grep_pipe[2], less_pipe[2];
	if (argc < 3)
		return -1;
	signal(SIGPIPE, int_handler);
	signal(SIGINT, int_handler);
	pattern = argv[1];
	for (int i = 2; i < argc; i++) {
		if ((fd_in = open(argv[i], O_RDONLY)) < 0) {
			fprintf(stderr, "Error while opening '%s' for reading: %s\n", argv[i], strerror(errno));
			continue;
		}
		if (pipe(grep_pipe) < 0) {
			fprintf(stderr, "Error while creating grep pipe: %s\n", strerror(errno));
			return -1;
		}
		if (pipe(less_pipe) < 0) {
			fprintf(stderr, "Error while creating less pipe: %s\n", strerror(errno));
			return -1;
		}
		if ((grep_pid = fork()) < 0) {
			eclose(fd_in);
			eclose(grep_pipe[0]);
			eclose(grep_pipe[1]);
			eclose(less_pipe[0]);
			eclose(less_pipe[1]);
			fprintf(stderr, "Error while forking: %s\n", strerror(errno));
			return -1;
		} else if (grep_pid == 0) {
			dup2(grep_pipe[0], STDIN_FILENO);
			dup2(less_pipe[1], STDOUT_FILENO);
			eclose(fd_in);
			eclose(grep_pipe[0]);
			eclose(grep_pipe[1]);
			eclose(less_pipe[0]);
			eclose(less_pipe[1]);
			if (execl("/bin/grep", "grep", "-e", pattern, NULL) < 0) {
				fprintf(stderr, "Error while executing grep: %s\n", strerror(errno));
				return -1;
			}
		} else {
			eclose(grep_pipe[0]);
		}
		if ((less_pid = fork()) < 0) {
			eclose(fd_in);
			eclose(grep_pipe[1]);
			eclose(less_pipe[0]);
			eclose(less_pipe[1]);
			fprintf(stderr, "Error while forking: %s\n", strerror(errno));
			continue;
		} else if (less_pid == 0) {
			dup2(less_pipe[0], STDIN_FILENO);
			eclose(fd_in);
			eclose(grep_pipe[1]);
			eclose(less_pipe[0]);
			eclose(less_pipe[1]);
			if (execl("/bin/less", "less", NULL) < 0) {
				fprintf(stderr, "Error while executing less: %s\n", strerror(errno));
				return -1;
			}
		} else {
			eclose(less_pipe[0]);
			eclose(less_pipe[1]);
		}
		if (sigsetjmp(int_jb, 1) != 0) {
			// SIGPIPE returns here
			dwFilesProcessed++;
			eclose(fd_in);
			eclose(grep_pipe[1]);
			continue;
		}
		dwFilesProcessed++;
		do {
			bytes_read = read(fd_in, buf, 4096);
			if (bytes_read < 0) {
				fprintf(stderr, "Error while reading '%s': %s\n", argv[i], strerror(errno));
				return -1;
			}
			if (bytes_read > 0) {
				int res, size;
				char *buf_write;
				buf_write = buf;
				size = bytes_read;
				while (size > 0) {
					res = write(grep_pipe[1], buf_write, size);
					if (res < 0) {
						fprintf(stderr, "Error while writing to pipe: %s\n", strerror(errno));
						return -1;
					}
					size -= res;
					buf_write += res;
					dwBytesProcessed += res;
				}
			}
		} while (bytes_read != 0);
		eclose(fd_in);
		eclose(grep_pipe[1]);
		// No deadlocks here because if a state already has changed, waitpid instantly returns
		waitpid(grep_pid, NULL, 0);
		waitpid(less_pid, NULL, 0);
	}
	return 0;
}