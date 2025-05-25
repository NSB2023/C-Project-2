#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#define MAX_CMD_LEN 1024
#define MAX_ARGS 64

char history[100][MAX_CMD_LEN];
int history_count = 0;

void print_prompt() {
    printf("speak> ");
}

void add_to_history(char *cmd) {
    if (history_count < 100) {
        strcpy(history[history_count++], cmd);
    }
}

void show_history() {
    for (int i = 0; i < history_count; i++) {
        printf("%d: %s\n", i + 1, history[i]);
    }
}

void handle_signal(int sig) {
    printf("\nCaught SIGINT. Press Ctrl+D to exit shell.\n");
}

int parse_input(char *input, char **args) {
    int i = 0;
    args[i] = strtok(input, " \t\n");
    while (args[i] != NULL && i < MAX_ARGS - 1) {
        args[++i] = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
    return i;
}

int is_redirection(char **args) {
    for (int i = 0; args[i]; i++) {
        if (strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0 || strcmp(args[i], "<") == 0)
            return i;
    }
    return -1;
}

int execute_builtin_command(char **args) {
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
    if (strcmp(args[0], "history") == 0) {
        show_history();
        return 1;
    }
    return 0;
}

void execute_simple_command(char **args) {
    if (execute_builtin_command(args)) return;

    pid_t pid = fork();
    if (pid == 0) {
        execvp(args[0], args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        wait(NULL);
    }
}

void execute_redirection(char **args, int index) {
    char *symbol = args[index];
    char *file = args[index + 1];

    if (!file) {
        fprintf(stderr, "Redirection syntax error: missing filename\n");
        return;
    }

    args[index] = NULL;

    int fd;
    if (strcmp(symbol, ">") == 0) {
        fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    } else if (strcmp(symbol, ">>") == 0) {
        fd = open(file, O_WRONLY | O_CREAT | O_APPEND, 0644);
    } else if (strcmp(symbol, "<") == 0) {
        fd = open(file, O_RDONLY);
    } else {
        fprintf(stderr, "Unknown redirection symbol\n");
        return;
    }

    if (fd < 0) {
        perror("open");
        return;
    }

    pid_t pid = fork();
    if (pid == 0) {
        if (strcmp(symbol, "<") == 0)
            dup2(fd, STDIN_FILENO);
        else
            dup2(fd, STDOUT_FILENO);
        close(fd);
        execvp(args[0], args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        close(fd);
        wait(NULL);
    }
}

void execute_multiple_pipes(char *input) {
    char *commands[10];
    int num_cmds = 0;

    commands[num_cmds] = strtok(input, "|");
    while (commands[num_cmds] != NULL && num_cmds < 9) {
        num_cmds++;
        commands[num_cmds] = strtok(NULL, "|");
    }

    int pipefd[2 * (num_cmds - 1)];

    for (int i = 0; i < num_cmds - 1; i++) {
        pipe(pipefd + i * 2);
    }

    for (int i = 0; i < num_cmds; i++) {
        char *args[MAX_ARGS];
        int j = 0;
        args[j] = strtok(commands[i], " \t\n");
        while (args[j] != NULL && j < MAX_ARGS - 1) {
            args[++j] = strtok(NULL, " \t\n");
        }
        args[j] = NULL;

        pid_t pid = fork();
        if (pid == 0) {
            if (i > 0) {
                dup2(pipefd[(i - 1) * 2], 0);
            }
            if (i < num_cmds - 1) {
                dup2(pipefd[i * 2 + 1], 1);
            }

            for (int k = 0; k < 2 * (num_cmds - 1); k++) {
                close(pipefd[k]);
            }

            execvp(args[0], args);
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < 2 * (num_cmds - 1); i++) {
        close(pipefd[i]);
    }

    for (int i = 0; i < num_cmds; i++) {
        wait(NULL);
    }
}

void process_input(char *input) {
    add_to_history(input);

    char *and_segments[10];
    int and_count = 0;
    and_segments[and_count] = strtok(input, "&&");
    while (and_segments[and_count] != NULL && and_count < 9) {
        and_segments[++and_count] = strtok(NULL, "&&");
    }

    for (int a = 0; a < and_count; a++) {
        char *commands[10];
        int count = 0;
        commands[count] = strtok(and_segments[a], ";");
        while (commands[count] != NULL && count < 9) {
            commands[++count] = strtok(NULL, ";");
        }

        for (int i = 0; i < count; i++) {
            char *segment = commands[i];
            if (strchr(segment, '|') != NULL) {
                execute_multiple_pipes(segment);
                continue;
            }

            char *args[MAX_ARGS];
            parse_input(segment, args);

            int redir = is_redirection(args);

            if (redir != -1)
                execute_redirection(args, redir);
            else {
                if (execute_builtin_command(args)) continue;

                pid_t pid = fork();
                int status;
                if (pid == 0) {
                    execvp(args[0], args);
                    perror("execvp");
                    exit(EXIT_FAILURE);
                } else {
                    waitpid(pid, &status, 0);
                    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                        return;
                    }
                }
            }
        }
    }
}

int main() {
    char input[MAX_CMD_LEN];
    signal(SIGINT, handle_signal);

    while (1) {
        print_prompt();
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\nExiting shell.\n");
            break;
        }
        process_input(input);
    }
    return 0;
}
