#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>

int main (int argc, char **argv) {
	extern char *optarg;
	extern int optind;
	int c, fd_out = STDOUT_FILENO;
	char *buffer_size = NULL, *out_file = NULL;
	char *buf;
	int buf_size = 4096, bytes_read;
	while ((c = getopt(argc, argv, "b:o:")) != -1) {
		switch (c) {
			case 'b':
				errno = 0;
				buffer_size = optarg;
				buf_size = strtol(buffer_size, NULL, 10);
				if (!buf_size) {
					fprintf(stderr, "invalid buffer size\n");
					return -1;
				}
				break;
			case 'o':
				out_file = optarg;
				break;
			default:
				break;
		}
	}
	buf = malloc(buf_size);
	if (out_file) {
		fd_out = open(out_file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
		if (fd_out == -1) {
			fprintf(stderr, "Error while opening '%s' for writing: %s\n", out_file, strerror(errno));
			return -1;
		}
	}
	if (optind == argc) {
		optind--;
		strcpy(argv[optind], "-");
	}
	for (int i = optind; i < argc; i++) {
		int fd_in = STDIN_FILENO;
		if (strcmp(argv[i], "-") != 0) {
			fd_in = open(argv[i], O_RDONLY);
			if (fd_in == -1) {
				fprintf(stderr, "Error while opening '%s' for reading: %s\n", argv[i], strerror(errno));
				return -1;
			}
		}
		do {
			bytes_read = read(fd_in, buf, buf_size);
			if (bytes_read < 0) {
				fprintf(stderr, "Error while reading '%s': %s\n", argv[i], strerror(errno));
				return -1;
			}
			if (bytes_read > 0) {
				int res, size;
				char *buf_write;
				buf_write = buf;
				size = bytes_read;
				while (size > 0 && (res = write(fd_out, buf_write, size)) != size) {
					if (res < 0) {
						fprintf(stderr, "Error while writing '%s': %s\n", out_file, strerror(errno));
						return -1;
					}
					size -= res;
					buf += res;
				}
			}
		} while (bytes_read != 0);
		if (fd_in != STDIN_FILENO) {
			if (close(fd_in+1) < 0) {
				fprintf(stderr, "Error while closing '%s': %s\n", argv[i], strerror(errno));
				return -1;
			}
		}
	}
	close(fd_out);
	free(buf);
	return 0;
}
