#include "shell.h"

#define MAX_JOBS 32
#define MAX_JOB_COMMAND 256

typedef struct {
    pid_t pid;
    char command[MAX_JOB_COMMAND];
} BackgroundJob;

static BackgroundJob jobs[MAX_JOBS];
static int job_count = 0;

static void add_background_job(pid_t pid, char *args[]) {
    if (job_count >= MAX_JOBS) {
        fprintf(stderr, "Too many background jobs\n");
        return;
    }

    jobs[job_count].pid = pid;
    jobs[job_count].command[0] = '\0';

    for (int i = 0; args[i] != NULL; i++) {
        if (strlen(jobs[job_count].command) +
            strlen(args[i]) + 2 >= MAX_JOB_COMMAND) {
            break;
        }

        if (i > 0) {
            strcat(jobs[job_count].command, " ");
        }

        strcat(jobs[job_count].command, args[i]);
    }

    job_count++;
}

static void remove_background_job(pid_t pid) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].pid == pid) {
            for (int j = i + 1; j < job_count; j++) {
                jobs[j - 1] = jobs[j];
            }

            job_count--;
            return;
        }
    }
}

void reap_background_jobs(void) {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        remove_background_job(pid);
    }
}

void print_background_jobs(void) {
    if (job_count == 0) {
        printf("No active background jobs.\n");
        return;
    }

    for (int i = 0; i < job_count; i++) {
        printf("[%d] PID %d  %s\n",
               i + 1,
               jobs[i].pid,
               jobs[i].command);
    }
}

static void run_command(char *args[], int input_fd, int output_fd) {
    if (input_fd != -1) {
        dup2(input_fd, STDIN_FILENO);
    }

    if (output_fd != -1) {
        dup2(output_fd, STDOUT_FILENO);
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

            dup2(fd, STDIN_FILENO);
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

            dup2(fd, STDOUT_FILENO);
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

int execute(char *arglist[], int background) {
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

        if (background) {
            add_background_job(child_pid, arglist);
            printf("[Background PID %d]\n", child_pid);
            return 0;
        }

        int status;
        waitpid(child_pid, &status, 0);

        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }

        return 1;
    }

    if (pipe_position == 0 ||
        arglist[pipe_position + 1] == NULL) {
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

    if (background) {
        add_background_job(left_pid, left);
        add_background_job(right_pid, right);
        printf("[Background PIDs %d, %d]\n",
               left_pid,
               right_pid);
        return 0;
    }

    int left_status;
    int right_status;

    waitpid(left_pid, &left_status, 0);
    waitpid(right_pid, &right_status, 0);

    if (WIFEXITED(right_status)) {
        return WEXITSTATUS(right_status);
    }

    return 1;
}
