#include "shell.h"
static char *command_history[HISTORY_SIZE];
static int history_count = 0;
#include <string.h>
char* read_cmd(char* prompt, FILE* fp) {
    printf("%s", prompt);
    char* cmdline = (char*) malloc(sizeof(char) * MAX_LEN);
    int c, pos = 0;

    while ((c = getc(fp)) != EOF) {
        if (c == '\n') break;
        cmdline[pos++] = c;
    }

    if (c == EOF && pos == 0) {
        free(cmdline);
        return NULL; // Handle Ctrl+D
    }
    
    cmdline[pos] = '\0';
    return cmdline;
}

char** tokenize(char* cmdline) {
    // Edge case: empty command line
    if (cmdline == NULL || cmdline[0] == '\0' || cmdline[0] == '\n') {
        return NULL;
    }

    char** arglist = (char**)malloc(sizeof(char*) * (MAXARGS + 1));
    for (int i = 0; i < MAXARGS + 1; i++) {
        arglist[i] = (char*)malloc(sizeof(char) * ARGLEN);
        memset(arglist[i], 0, ARGLEN);
    }

    char* cp = cmdline;
    char* start;
    int len;
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS) {
        while (*cp == ' ' || *cp == '\t') cp++; // Skip leading whitespace
        
        if (*cp == '\0') break; // Line was only whitespace

        start = cp;
        len = 1;
        while (*++cp != '\0' && !(*cp == ' ' || *cp == '\t')) {
            len++;
        }
        strncpy(arglist[argnum], start, len);
        arglist[argnum][len] = '\0';
        argnum++;
    }

    if (argnum == 0) { // No arguments were parsed
        for(int i = 0; i < MAXARGS + 1; i++) free(arglist[i]);
        free(arglist);
        return NULL;
    }

    arglist[argnum] = NULL;
    return arglist;
}
int handle_builtin(char **arglist) {
    if (strcmp(arglist[0], "exit") == 0) {
        exit(0);
    }

    if (strcmp(arglist[0], "cd") == 0) {
        if (arglist[1] == NULL) {
            fprintf(stderr, "cd: missing directory\n");
        } else if (chdir(arglist[1]) != 0) {
            perror("cd");
        }
        return 1;
    }

    if (strcmp(arglist[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("  exit              Exit the shell\n");
        printf("  cd <directory>    Change directory\n");
        printf("  help              Show this help message\n");
        printf("  jobs              Show background jobs\n");
        printf("  history           Show command history\n");
        return 1;
    }
     if (strcmp(arglist[0], "history") == 0) {
          print_command_history();
           return 1;
     }

    if (strcmp(arglist[0], "jobs") == 0) {
       reap_background_jobs();
       print_background_jobs();
       return 1;
     }

    return 0;
}
void add_history_command(const char *command) {
    if (command == NULL || command[0] == '\0') {
        return;
    }

    if (history_count == HISTORY_SIZE) {
        free(command_history[0]);

        for (int i = 1; i < HISTORY_SIZE; i++) {
            command_history[i - 1] = command_history[i];
        }

        history_count--;
    }

    command_history[history_count] = malloc(strlen(command) + 1);
    strcpy(command_history[history_count], command);
    history_count++;
}

void print_command_history(void) {
    for (int i = 0; i < history_count; i++) {
        printf("%d %s\n", i + 1, command_history[i]);
    }
}

const char *get_history_command(int number) {
    if (number < 1 || number > history_count) {
        return NULL;
    }

    return command_history[number - 1];
}
