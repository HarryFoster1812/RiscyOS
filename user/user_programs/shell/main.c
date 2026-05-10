#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT 256
#define MAX_ARGS  16
#define MAX_PATHS 16
#define MAX_PATH_LEN 128

// Parse input into argv
int parse_input(char *input, char *args[]) {
    int argc = 0;

    char *token = strtok(input, " ");
    while (token != NULL && argc < MAX_ARGS - 1) {
        args[argc++] = token;
        token = strtok(NULL, " ");
    }

    args[argc] = NULL;
    return argc;
}

//  Split PATH into directories
int parse_path(char *path_env, char *paths[]) {
    int count = 0;

    char *token = strtok(path_env, ":");
    while (token != NULL && count < MAX_PATHS) {
        paths[count++] = token;
        token = strtok(NULL, ":");
    }

    return count;
}

// Try executing using PATH
void exec_with_path(char *args[]) {
    char *path_env = getenv("PATH");

    if (!path_env) {
        execv(args[0], args);
        return;
    }

    // Copy PATH because strtok modifies it
    char path_copy[512];
    strncpy(path_copy, path_env, sizeof(path_copy));

    char *paths[MAX_PATHS];
    int path_count = parse_path(path_copy, paths);

    char fullpath[MAX_PATH_LEN];

    for (int i = 0; i < path_count; i++) {
        snprintf(fullpath, sizeof(fullpath), "%s/%s", paths[i], args[0]);
        execv(fullpath, args);
    }

    // Final fallback: try as-is
    execv(args[0], args);

    perror("execv");
}

//Built-in commands
int handle_builtin(char *args[]) {
    if (args[0] == NULL) return 1;

    // exit
    if (strcmp(args[0], "exit") == 0) {
        exit(0);
    }

    // cd
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "cd: missing argument\n");
        } else {
            if (chdir(args[1]) != 0) {
                perror("cd");
            }
        }
        return 1;
    }

    return 0; // not a builtin
}

int main() {
    char input[MAX_INPUT];
    char *args[MAX_ARGS];

    while (1) {
        // Prompt
        printf("RiscyOS> ");

        // Read input
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }

        // Remove newline
        input[strcspn(input, "\n")] = '\0';

        // Parse
        parse_input(input, args);

        if (args[0] == NULL) continue;

        // Builtins
        if (handle_builtin(args)) {
            continue;
        }

        // Fork
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            continue;
        }

        if (pid == 0) {
            // Child
            exec_with_path(args);
            exit(1);
        } else {
            // Parent
            wait(NULL);
        }
    }

    return 0;
}
