#include "shell.h"

int main() {
    char *cmdline;
    char **arglist;
    using_history();
    while ((cmdline = readline(PROMPT)) != NULL) {
        if (cmdline[0] != '\0') {
            add_history(cmdline);
        }

        if (cmdline[0] == '!') {
            char *endptr;
            long number = strtol(cmdline + 1, &endptr, 10);

            if (endptr == cmdline + 1 || *endptr != '\0' || number < 1) {
                fprintf(stderr, "Invalid history reference: %s\n", cmdline);
                free(cmdline);
                continue;
            }

            const char *saved_command =
                get_history_command((int)number);

            if (saved_command == NULL) {
                fprintf(stderr, "History entry not found: %ld\n", number);
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

        if ((arglist = tokenize(cmdline)) != NULL) {
            if (!handle_builtin(arglist)) {
                execute(arglist);
            }

            for (int i = 0; arglist[i] != NULL; i++) {
                free(arglist[i]);
            }

            free(arglist);
        }

        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
