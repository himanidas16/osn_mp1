// Replace the entire main.c file
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>
#include "shell.h"
#include "prompt.h"
#include "parser.h"
#include "commands.h"
#include "redirection.h"

char g_shell_home[PATH_MAX];
char g_shell_prev[PATH_MAX] = {0};

background_job_t g_background_jobs[MAX_BACKGROUND_JOBS];
int g_next_job_id = 1;

char g_log_commands[MAX_LOG_COMMANDS][1024];
int g_log_count = 0;
int g_log_start = 0;

pid_t g_foreground_pid = 0;
pid_t g_foreground_pgid = 0;
char g_foreground_command[256] = {0};

volatile sig_atomic_t sigint_received = 0;
volatile sig_atomic_t sigtstp_received = 0;

int main(void) {
    if (prompt_init() != 0) {
        fprintf(stderr, "Failed to initialize prompt\n");
        return 1;
    }

    log_init();
    init_background_jobs();
    setup_signal_handlers();

    for (;;) {
        check_background_jobs();

        char p[SHELL_PROMPT_MAX];
        if (prompt_build(p, sizeof p) == 0) {
            printf("%s", p);
            fflush(stdout);
        }

        char *line = NULL;
        size_t cap = 0;

        ssize_t n = getline(&line, &cap, stdin);

        if (n < 0) {
            if (feof(stdin)) {
                cleanup_and_exit();
            } else if (errno == EINTR) {
                printf("\n");
                if (line) free(line);
                clearerr(stdin);
                errno = 0;
                continue;
            } else {
                perror("getline");
                if (line) free(line);
                break;
            }
        }

        if (n > 0 && line[n-1] == '\n') {
            line[n-1] = '\0';
            n--;
        }

        check_background_jobs();

        if (strlen(line) > 0) {
            if (parse_command(line) != 0) {
                printf("Invalid Syntax!\n");
            } else {
                if (!log_contains_log_command(line)) {
                    log_add_command(line);
                }

                if (strchr(line, ';') != NULL) {
                    sequential_commands_t seq_cmds;
                    if (parse_sequential_commands(line, &seq_cmds) == 0) {
                        execute_sequential_commands(&seq_cmds);
                        cleanup_sequential_commands(&seq_cmds);
                    } else {
                        if (strchr(line, '|') != NULL) {
                            command_pipeline_t pipeline;
                            if (parse_pipeline(line, &pipeline) == 0) {
                                execute_pipeline(&pipeline);
                                cleanup_pipeline(&pipeline);
                            } else {
                                parsed_command_t cmd;
                                if (parse_command_with_redirection(line, &cmd) == 0) {
                                    execute_command_with_redirection(&cmd);
                                    cleanup_parsed_command(&cmd);
                                } else {
                                    execute_command(line);
                                }
                            }
                        } else {
                            parsed_command_t cmd;
                            if (parse_command_with_redirection(line, &cmd) == 0) {
                                execute_command_with_redirection(&cmd);
                                cleanup_parsed_command(&cmd);
                            } else {
                                execute_command(line);
                            }
                        }
                    }
                } else if (strchr(line, '|') != NULL || strchr(line, '&') != NULL) {
                    command_pipeline_t pipeline;
                    if (parse_pipeline(line, &pipeline) == 0) {
                        execute_pipeline(&pipeline);
                        cleanup_pipeline(&pipeline);
                    } else {
                        parsed_command_t cmd;
                        if (parse_command_with_redirection(line, &cmd) == 0) {
                            execute_command_with_redirection(&cmd);
                            cleanup_parsed_command(&cmd);
                        } else {
                            execute_command(line);
                        }
                    }
                } else {
                    parsed_command_t cmd;
                    if (parse_command_with_redirection(line, &cmd) == 0) {
                        execute_command_with_redirection(&cmd);
                        cleanup_parsed_command(&cmd);
                    } else {
                        execute_command(line);
                    }
                }
            }
        }
        free(line);
    }
    return 0;
}