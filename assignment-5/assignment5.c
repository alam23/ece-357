#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

void elseek(int fd, off_t offset, int whence) {
	if (lseek(fd, offset, whence) < 0)
		fprintf(stderr, "Error while seeking file descriptor %d: %s\n", fd, strerror(errno));
}

void eclose(int fd) {
	if (close(fd))
		fprintf(stderr, "Error while closing file descriptor %d: %s\n", fd, strerror(errno));
}

void sig_handler(int signo) {
	printf("Signal Caught (%d): %s\n", signo, strsignal(signo));
	exit(-1);
}

int main(int argc, char **argv) {
	int fd;
	time_t t;
	char buf[8195];
	if (argc < 2) {
		fprintf(stderr, "Enter A-E, F1-F2 to Execute Test Cases.\n");
		exit(0);
	}
	// Generate Random File (8195)
	srand((unsigned int)time(&t));
	for (int i = 0; i < 8195; i++)
		buf[i] = rand() % 0xFF;
	fd = open("file_8195.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd < 0) {
		fprintf(stderr, "Error while creating file_8195.txt: %s\n", strerror(errno));
		return -1;
	}
	char *buf_write = buf;
	int size = 8195;
	int res;
	while (size > 0 && (res = write(fd, buf_write, size)) != size) {
		if (res < 0) {
			fprintf(stderr, "Error while writing to file_8195.txt: %s\n", strerror(errno));
			return -1;
		}
		size -= res;
		buf_write += res;
	}
	eclose(fd);
	// Generate Random File (10)
	fd = open("file_10.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd < 0) {
		fprintf(stderr, "Error while creating file_10.txt: %s\n", strerror(errno));
		return -1;
	}
	buf_write = buf;
	size = 10;
	while (size > 0 && (res = write(fd, buf_write, size)) != size) {
		if (res < 0) {
			fprintf(stderr, "Error while writing to file_10.txt: %s\n", strerror(errno));
			return -1;
		}
		size -= res;
		buf_write += res;
	}
	eclose(fd);
	// Register All Signals
	for (int i = 1; i <= 15; i++) {
		if (i == 9)
			continue;
		if (signal(i, sig_handler) == SIG_ERR)
			fprintf(stderr, "Can't register handler for signal #%d!\n", i);
	}
	// Part A
	if (argv[1][0] == 'A') {
		printf("Part A\n");
		fd = open("file_8195.txt", O_RDONLY);
		if (fd < 0) {
			fprintf(stderr, "Error while opening file_8195.txt: %s\n", strerror(errno));
			return -1;
		}
		char* map = mmap(0, 8195, PROT_READ, MAP_PRIVATE, fd, 0);
		if (map == MAP_FAILED) {
			fprintf(stderr, "Error calling mmap: %s\n", strerror(errno));
			eclose(fd);
			return -1;
		}
		map[1] = 1;
		printf("No Signal Sent!\n");
		eclose(fd);
		exit(0);
	}
	// Part B
	if (argv[1][0] == 'B') {
		printf("Part B\n");
		fd = open("file_8195.txt", O_RDWR);
		if (fd < 0) {
			fprintf(stderr, "Error while opening file_8195.txt: %s\n", strerror(errno));
			return -1;
		}
		char* map = mmap(0, 8195, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (map == MAP_FAILED) {
			fprintf(stderr, "Error calling mmap: %s\n", strerror(errno));
			eclose(fd);
			return -1;
		}
		char rd_b, rd_a;
		elseek(fd, 0, SEEK_SET);
		if (read(fd, &rd_b, 1) < 0)
			fprintf(stderr, "Error while reading: %s\n", strerror(errno));
		printf("Before: 0x%02x\n", (unsigned char)rd_b);
		map[0]++;
		elseek(fd, 0, SEEK_SET);
		if (read(fd, &rd_a, 1) < 0)
			fprintf(stderr, "Error while reading: %s\n", strerror(errno));
		printf("After: 0x%02x\n", (unsigned char)rd_a);
		if (rd_b == rd_a)
			printf("Update is not visible.\n");
		else
			printf("Update is visible.\n");
		eclose(fd);
		exit(0);
	}
	// Part C
	if (argv[1][0] == 'C') {
		printf("Part C\n");
		fd = open("file_8195.txt", O_RDWR);
		if (fd < 0) {
			fprintf(stderr, "Error while opening file_8195.txt: %s\n", strerror(errno));
			return -1;
		}
		char* map = mmap(0, 8195, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
		if (map == MAP_FAILED) {
			fprintf(stderr, "Error calling mmap: %s\n", strerror(errno));
			eclose(fd);
			return -1;
		}
		char rd_b, rd_a;
		elseek(fd, 0, SEEK_SET);
		if (read(fd, &rd_b, 1) < 0)
			fprintf(stderr, "Failed to read: %s\n", strerror(errno));
		printf("Before: 0x%02x\n", (unsigned char)rd_b);
		map[0]++;
		elseek(fd, 0, SEEK_SET);
		if (read(fd, &rd_a, 1) < 0)
			fprintf(stderr, "Failed to read: %s\n", strerror(errno));
		printf("After: 0x%02x\n", (unsigned char)rd_a);
		if (rd_b == rd_a)
			printf("Update is not visible.\n");
		else
			printf("Update is visible.\n");
		eclose(fd);
		exit(0);
	}
	// Part D & E
	if (argv[1][0] == 'D' || argv[1][0] == 'E') {
		printf("Part D\n");
		fd = open("file_8195.txt", O_RDWR);
		if (fd < 0) {
			fprintf(stderr, "Error while opening file_8195.txt: %s\n", strerror(errno));
			return -1;
		}
		char* map = mmap(0, 8195, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (map == MAP_FAILED) {
			fprintf(stderr, "Error calling mmap: %s\n", strerror(errno));
			eclose(fd);
			return -1;
		}
		struct stat sb;
		int fs_b, fs_a;
		if (stat("file_8195.txt", &sb) < 0) {
			fprintf(stderr, "Error calling stat: %s\n", strerror(errno));
			eclose(fd);
			return -1;
		}
		fs_b = sb.st_size;
		printf("File Size: %d\n", fs_b);
		map[8196] = 0x37;
		if (stat("file_8195.txt", &sb) < 0) {
			fprintf(stderr, "Error calling stat: %s\n", strerror(errno));
			eclose(fd);
			return -1;
		}
		fs_a = sb.st_size;
		printf("File Size: %d\n", fs_a);
		if (fs_b == fs_a)
			// THIS IS CUZ WHEN U MMAP THE SYSTEM THINGY NOES THAT U GO UP TO 8195 SO IT DOESN'T RLY CARE IF U GO BEYOND THAT BOUNDARY. LIEK IT'S NOT GUNNA KNOW IF U WRITE OUTSIDE IT CUZ ITS NOT MAPPED SO IT DUN CARE.
			printf("File size is the same.\n");
		else
			printf("File size is different.\n");
		printf("Part E\n");
		elseek(fd, 8197, SEEK_SET);
		char wt = 0xFF;
		if (write(fd, &wt, 1) < 0)
			fprintf(stderr, "Failed to write: %s\n", strerror(errno));
		elseek(fd, 8196, SEEK_SET);
		char rd;
		if (read(fd, &rd, 1) < 0)
			fprintf(stderr, "Failed to read: %s\n", strerror(errno));
		printf("Read Data: 0x%02x\n", (unsigned char)rd);
		if (rd == 0x37)
			// The data is saved because it was on the page that it was mapped on and since the size of the file is extended, it is written to file
			printf("Data In The Hole Has Been Saved.\n");
		else
			printf("Data In The Hole Is Lost And Replaced With 0x%02x.\n", (unsigned char)rd);
		eclose(fd);
		exit(0);
	}
	// Part F1, F2
	// You can establish the mmap (it returns no error) but it causes issues on page faults.
	// You get bus error on the second page because the kernel can't satisfy the page fault when I try accessing it.
	// Accessing the same page is ok because it's still within the boundaries.
	if (argv[1][0] == 'F' && argv[1][1] == '1') {
		printf("Part F1\n");
		fd = open("file_10.txt", O_RDWR);
		if (fd < 0) {
			fprintf(stderr, "Error while opening file_10.txt: %s\n", strerror(errno));
			return -1;
		}
		char* map = mmap(0, 8192, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (map == MAP_FAILED) {
			fprintf(stderr, "Error calling mmap: %s\n", strerror(errno));
			eclose(fd);
			return -1;
		}
		char c = map[8000];
		printf("No Signal Sent!\n");
		eclose(fd);
		exit(0);
	}
	if (argv[1][0] == 'F' && argv[1][1] == '2') {
		printf("Part F2\n");
		fd = open("file_10.txt", O_RDWR);
		if (fd < 0) {
			fprintf(stderr, "Error while opening file_10.txt: %s\n", strerror(errno));
			return -1;
		}
		char* map = mmap(0, 8192, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (map == MAP_FAILED) {
			fprintf(stderr, "Error calling mmap: %s\n", strerror(errno));
			eclose(fd);
			return -1;
		}
		char c = map[2000];
		printf("No Signal Sent!\n");
		eclose(fd);
		exit(0);
	}
}