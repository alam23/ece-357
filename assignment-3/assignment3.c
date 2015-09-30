#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>

int return_code = 0;
FILE *scriptfile;

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
    printf("Executing command %s with arguments ", command);
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
        if (add_to_list) {
            *next++ = token;
            printf("%s ", token);
        }
        token = strtok_r(NULL, " ", &save_ptr1);
    }
    *next++ = NULL;
    printf("\n");
    int p = fork();
    if (p > 0) {
        // Success
        if (fd_out != STDOUT_FILENO) {
            close(fd_out);
        }
        if (fd_in != STDIN_FILENO) {
            close(fd_in);
        }
        if (fd_err != STDERR_FILENO) {
            close(fd_err);
        }
        int status;
        struct rusage ru;
        struct timeval start, end;
        gettimeofday(&start, NULL);
        if (wait3(&status, 0, &ru) != -1) {
            if ((char)WEXITSTATUS(status) == -25) {
                printf("Execution failed.\n");
                return_code = -1;
            } else {
                gettimeofday(&end, NULL);
                struct timeval diff;
                timersub(&end, &start, &diff);
                printf("Command returned with return code %hhd,\n", WEXITSTATUS(status));
                printf("Consuming %ld.%.3d real seconds, %ld.%.3d user, %ld.%.3d system.\n", (long int)diff.tv_sec, (int)diff.tv_usec, (long int)ru.ru_utime.tv_sec, (int)ru.ru_utime.tv_usec, (long int)ru.ru_stime.tv_sec, (int)ru.ru_stime.tv_usec);
            }
        } else {
            printf("Wait failed\n");
            return_code = -1;
        }
    }
    if (p == 0) {
        if (scriptfile != stdin)
            fclose(scriptfile);
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
        if (execvp(command, arguments) < 0) {
            fprintf(stderr, "Execv: unable to execute\n");
            exit(-25);
        }
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
    scriptfile=stdin;
    if (argc = 2 && argv[1] != NULL) {
        scriptfile = fopen(argv[1], "r");
        if (!scriptfile) {
            fprintf(stderr, "Main: invalid script\n");
            return -1;
        }
    }
    while ((read = getline(&line, &len, scriptfile)) != -1) {
        if (*line == '#')
            continue;
        if (line[read - 1] == '\n')
            line[read - 1] = '\0';
        parsecommand(line);
    }
    fclose(scriptfile);
    return return_code;
}