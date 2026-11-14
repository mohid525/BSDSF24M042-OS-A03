#include "shell.h"

#define MAX_IF_LINES 20

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

static char *copy_text(const char *text) {
    char *copy = malloc(strlen(text) + 1);
    strcpy(copy, text);
    return copy;
}

static int process_segment(char *segment) {
    segment = trim_spaces(segment);

    if (*segment == '\0') {
        return 0;
    }

    int background = 0;
    int length = strlen(segment);

    if (length > 0 && segment[length - 1] == '&') {
        background = 1;
        segment[length - 1] = '\0';
        segment = trim_spaces(segment);
    }

    if (is_variable_assignment(segment)) {
        set_variable(segment);
        return 0;
    }

    char **arglist = tokenize(segment);

    if (arglist == NULL) {
        return 0;
    }

    for (int i = 0; arglist[i] != NULL; i++) {
        char *expanded = expand_variables(arglist[i]);
        free(arglist[i]);
        arglist[i] = expanded;
    }

    int result = 0;

    if (!handle_builtin(arglist)) {
        result = execute(arglist, background);
    }

    for (int i = 0; arglist[i] != NULL; i++) {
        free(arglist[i]);
    }

    free(arglist);
    return result;
}

static void run_line(char *line) {
    char *segment = strtok(line, ";");

    while (segment != NULL) {
        process_segment(segment);
        segment = strtok(NULL, ";");
    }
}

static void run_if_statement(char *condition) {
    char *then_lines[MAX_IF_LINES];
    char *else_lines[MAX_IF_LINES];

    int then_count = 0;
    int else_count = 0;
    int in_else = 0;
    int found_fi = 0;

    int condition_status = process_segment(condition);

    char *line = readline("if> ");

    if (line == NULL ||
        strcmp(trim_spaces(line), "then") != 0) {
        fprintf(stderr, "Expected 'then'\n");

        if (line != NULL) {
            free(line);
        }

        return;
    }

    free(line);

    while ((line = readline("if> ")) != NULL) {
        char *clean_line = trim_spaces(line);

        if (strcmp(clean_line, "else") == 0) {
            in_else = 1;
            free(line);
            continue;
        }

        if (strcmp(clean_line, "fi") == 0) {
            found_fi = 1;
            free(line);
            break;
        }

        if (*clean_line != '\0') {
            if (!in_else && then_count < MAX_IF_LINES) {
                then_lines[then_count++] = copy_text(clean_line);
            } else if (in_else && else_count < MAX_IF_LINES) {
                else_lines[else_count++] = copy_text(clean_line);
            }
        }

        free(line);
    }

    if (!found_fi) {
        fprintf(stderr, "Expected 'fi'\n");
    } else if (condition_status == 0) {
        for (int i = 0; i < then_count; i++) {
            run_line(then_lines[i]);
        }
    } else {
        for (int i = 0; i < else_count; i++) {
            run_line(else_lines[i]);
        }
    }

    for (int i = 0; i < then_count; i++) {
        free(then_lines[i]);
    }

    for (int i = 0; i < else_count; i++) {
        free(else_lines[i]);
    }
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

        char *clean_line = trim_spaces(cmdline);

        if (strncmp(clean_line, "if ", 3) == 0) {
            run_if_statement(clean_line + 3);
            free(cmdline);
            continue;
        }

        if (clean_line[0] == '!') {
            char *endptr;
            long number = strtol(clean_line + 1, &endptr, 10);

            if (endptr == clean_line + 1 ||
                *endptr != '\0' ||
                number < 1) {
                fprintf(stderr,
                        "Invalid history reference: %s\n",
                        clean_line);
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

            char *expanded_command = copy_text(saved_command);
            free(cmdline);
            cmdline = expanded_command;
        }

        add_history_command(cmdline);
        run_line(cmdline);
        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
