#include "shell.h"
#include <ctype.h>

static char *trim_spaces(char *text) {
    while (*text == ' ' || *text == '\t') {
        text++;
    }

    if (*text == '\0') {
        return text;
    }

    char *end = text + strlen(text) - 1;

    while (end > text &&
           (*end == ' ' || *end == '\t')) {
        *end = '\0';
        end--;
    }

    return text;
}

static void process_segment(char *segment) {
    segment = trim_spaces(segment);

    if (*segment == '\0') {
        return;
    }

    int background = 0;
    int length = strlen(segment);

    if (length > 0 && segment[length - 1] == '&') {
        background = 1;
        segment[length - 1] = '\0';
        segment = trim_spaces(segment);
    }

    char **arglist = tokenize(segment);

    if (arglist == NULL) {
        return;
    }

    if (!handle_builtin(arglist)) {
        execute(arglist, background);
    }

    for (int i = 0; arglist[i] != NULL; i++) {
        free(arglist[i]);
    }

    free(arglist);
}

int main() {
    char *cmdline;

    using_history();

    while (1) {
        reap_background_jobs();

        cmdline = readline(PROMPT);

        if (cmdline == NULL) {
            break;
        }

        if (cmdline[0] == '\0') {
            free(cmdline);
            continue;
        }

        add_history(cmdline);

        if (cmdline[0] == '!') {
            char *endptr;
            long number = strtol(cmdline + 1, &endptr, 10);

            if (endptr == cmdline + 1 ||
                *endptr != '\0' ||
                number < 1) {
                fprintf(stderr,
                        "Invalid history reference: %s\n",
                        cmdline);
                free(cmdline);
                continue;
            }

            const char *saved_command =
                get_history_command((int)number);

            if (saved_command == NULL) {
                fprintf(stderr,
                        "History entry not found: %ld\n",
                        number);
                free(cmdline);
                continue;
            }

            char *expanded_command =
                malloc(strlen(saved_command) + 1);

            strcpy(expanded_command, saved_command);
            free(cmdline);
            cmdline = expanded_command;

            printf("%s\n", cmdline);
        }

        add_history_command(cmdline);

        char *segment = strtok(cmdline, ";");

        while (segment != NULL) {
            process_segment(segment);
            segment = strtok(NULL, ";");
        }

        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
