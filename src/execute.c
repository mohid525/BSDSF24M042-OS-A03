#include "shell.h"

static void run_command(char *args[], int input_fd, int output_fd) {
    if (input_fd != -1) {
        if (dup2(input_fd, STDIN_FILENO) == -1) {
            perror("dup2 input");
            _exit(1);
        }
    }

    if (output_fd != -1) {
        if (dup2(output_fd, STDOUT_FILENO) == -1) {
            perror("dup2 output");
            _exit(1);
        }
    }

    char *argv[MAXARGS + 1];
    int argc = 0;

    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {
            if (args[i + 1] == NULL) {
                fprintf(stderr, "Missing input filename\n");
                _exit(1);
            }

            int fd = open(args[i + 1], O_RDONLY);
            if (fd == -1) {
                perror("open input");
                _exit(1);
            }

            if (dup2(fd, STDIN_FILENO) == -1) {
                perror("dup2 input");
                close(fd);
                _exit(1);
            }

            close(fd);
            i++;
        } else if (strcmp(args[i], ">") == 0) {
            if (args[i + 1] == NULL) {
                fprintf(stderr, "Missing output filename\n");
                _exit(1);
            }

            int fd = open(args[i + 1],
                          O_WRONLY | O_CREAT | O_TRUNC,
                          0644);

            if (fd == -1) {
                perror("open output");
                _exit(1);
            }

            if (dup2(fd, STDOUT_FILENO) == -1) {
                perror("dup2 output");
                close(fd);
                _exit(1);
            }

            close(fd);
            i++;
        } else {
            argv[argc++] = args[i];
        }
    }

    argv[argc] = NULL;

    if (argc == 0) {
        _exit(0);
    }

    execvp(argv[0], argv);
    perror("Command not found");
    _exit(127);
}

int execute(char *arglist[]) {
    int pipe_position = -1;

    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "|") == 0) {
            pipe_position = i;
            break;
        }
    }

    if (pipe_position == -1) {
        pid_t child_pid = fork();

        if (child_pid == -1) {
            perror("fork");
            return -1;
        }

        if (child_pid == 0) {
            run_command(arglist, -1, -1);
        }

        int status;
        waitpid(child_pid, &status, 0);
        return 0;
    }

    if (pipe_position == 0 || arglist[pipe_position + 1] == NULL) {
        fprintf(stderr, "Invalid pipe command\n");
        return -1;
    }

    char *left[MAXARGS + 1];
    char *right[MAXARGS + 1];

    for (int i = 0; i < pipe_position; i++) {
        left[i] = arglist[i];
    }
    left[pipe_position] = NULL;

    int right_count = 0;
    for (int i = pipe_position + 1;
         arglist[i] != NULL;
         i++) {
        right[right_count++] = arglist[i];
    }
    right[right_count] = NULL;

    int pipe_fds[2];

    if (pipe(pipe_fds) == -1) {
        perror("pipe");
        return -1;
    }

    pid_t left_pid = fork();

    if (left_pid == -1) {
        perror("fork");
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        return -1;
    }

    if (left_pid == 0) {
        close(pipe_fds[0]);
        run_command(left, -1, pipe_fds[1]);
    }

    pid_t right_pid = fork();

    if (right_pid == -1) {
        perror("fork");
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        waitpid(left_pid, NULL, 0);
        return -1;
    }

    if (right_pid == 0) {
        close(pipe_fds[1]);
        run_command(right, pipe_fds[0], -1);
    }

    close(pipe_fds[0]);
    close(pipe_fds[1]);

    waitpid(left_pid, NULL, 0);
    waitpid(right_pid, NULL, 0);

    return 0;
}
