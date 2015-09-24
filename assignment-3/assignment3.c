#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>

void parsecommand(char *line) {
    char *token, *save_ptr1, *save_ptr2;
    char *command, *runcommand;
    char *arguments[1024];
    char **next = arguments;
    int fd_out = STDOUT_FILENO;
    int fd_in = STDIN_FILENO;
    int fd_err = STDERR_FILENO;
    size_t read = 0;
    token = strtok_r(line, " ", &save_ptr1);
    command = token;
    // Check for shebang
    FILE *file = fopen(command, "r");
    if (file != NULL) {
        char first[4096];
        if (fgets(first, 4096, file) != NULL) {
            read = strlen(first);
            if (first[read - 1] == '\n')
                first[read - 1] = '\0';
            if(first[0] == '#' && first[1] == '!') {
                *next++ = command;
                runcommand = strtok_r(&first[2], " ", &save_ptr2);
                token = strtok_r(NULL, " ", &save_ptr2);
                while (token != NULL) {
                    *next++ = token;
                    token = strtok_r(NULL, " ", &save_ptr2);
                }
                *next++ = command;
                command = runcommand;
            }
        }
        fclose(file);
    }
    if (next == arguments)
        *next++ = command;
    token = strtok_r(NULL, " ", &save_ptr1);
    while (token != NULL) {
        int len = strlen(token);
        int add_to_list = 1;
        if (len >= 1) {
            if (token[0] == '<') {
                fd_in = open(&token[1], O_RDONLY);
                if (fd_in < 0) {
                    fprintf(stderr, "Open: unable to open input file\n");
                    return;
                }
                add_to_list = 0;
            }
            if (token[0] == '>') {
                if (len >= 2) {
                    if (token[1] == '>') {
                        fd_out = open(&token[2], O_WRONLY | O_APPEND | O_CREAT, 0664);
                        if (fd_out < 0) {
                            fprintf(stderr, "Open: unable to append to output file\n");
                            return;
                        }
                        add_to_list = 0;
                    } else {
                        fd_out = open(&token[1], O_WRONLY | O_TRUNC | O_CREAT, 0664);
                        if (fd_out < 0) {
                            fprintf(stderr, "Open: unable to create output file\n");
                            return;
                        }
                        add_to_list = 0;
                    }
                }
            }
            if (token[0] == '2') {
                if (len >= 2) {
                    if (token[1] == '>') {
                        if (len >=3 ) {
                            if (token[2] == '>') {
                                fd_err = open(&token[3], O_WRONLY | O_APPEND | O_CREAT, 0664);
                                if (fd_err < 0) {
                                    fprintf(stderr, "Open: unable to append to error file\n");
                                    return;
                                }
                                add_to_list = 0;
                            } else {
                                fd_err = open(&token[2], O_WRONLY | O_TRUNC | O_CREAT, 0664);
                                if (fd_err < 0) {
                                    fprintf(stderr, "Open: unable to create error file\n");
                                    return;
                                }
                                add_to_list = 0;
                            }
                        }
                    }
                }
            }
        }
        if (add_to_list)
            *next++ = token;
        token = strtok_r(NULL, " ", &save_ptr1);
    }
    *next++ = NULL;
    int p = fork();
    if (p > 0) {
        if (fd_out != STDOUT_FILENO) {
            close(fd_out);
        }
        if (fd_in != STDIN_FILENO) {
            close(fd_in);
        }
        if (fd_err != STDERR_FILENO) {
            close(fd_err);
        }
    }
    if (p == 0) {
        if (fd_out != STDOUT_FILENO) {
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }
        if (fd_in != STDIN_FILENO) {
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }
        if (fd_err != STDERR_FILENO) {
            dup2(fd_err, STDERR_FILENO);
            close(fd_err);
        }
        if (execvp(command, arguments) < 0)
            fprintf(stderr, "Execv: unable to execute\n");
        exit(0);
    }
    if (p < 0) {
        fprintf(stderr, "Fork: can't create child process\n");
    }
}

int main (int argc, char **argv) {
    char *line = NULL;
    size_t read = 0;
    size_t len = 0;
    while ((read = getline(&line, &len, stdin)) != -1) {
        if (*line == '#')
            continue;
        if (line[read - 1] == '\n')
            line[read - 1] = '\0';
        parsecommand(line);
    }
    return 0;
}