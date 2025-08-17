#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include "shell.h"
#include "prompt.h"
#include "parser.h"
#include "commands.h"
#include "redirection.h"

char g_shell_home[PATH_MAX];
char g_shell_prev[PATH_MAX] = {0}; // Previous directory for hop -

// Background job storage
background_job_t g_background_jobs[MAX_BACKGROUND_JOBS];
int g_next_job_id = 1;


// Log storage
char g_log_commands[MAX_LOG_COMMANDS][1024];
int g_log_count = 0;
int g_log_start = 0;

// Replace your main function in src/main.c

int main(void) {
    if (prompt_init() != 0) {
        fprintf(stderr, "Failed to initialize prompt\n");
        return 1;
    }
    
    // Initialize log system
    log_init();
    
    // Initialize background job management
    init_background_jobs();

    for (;;) {
        // Check for completed background jobs BEFORE showing prompt
        check_background_jobs();
        
        char p[SHELL_PROMPT_MAX];
        if (prompt_build(p, sizeof p) == 0) {
            write(STDOUT_FILENO, p, strlen(p));
        }

        char *line = NULL; 
        size_t cap = 0;
        ssize_t n = getline(&line, &cap, stdin);
        if (n < 0) {
            if (errno == EINTR) {
                free(line);
                continue; // Retry if interrupted by a signal
            }
            write(STDOUT_FILENO, "\n", 1);
            free(line);
            break;
        }

        // Remove trailing newline if present
        if (n > 0 && line[n-1] == '\n') {
            line[n-1] = '\0';
            n--;
        }

        // Check for completed background jobs AFTER user input
        check_background_jobs();

        // A.3: Parse and execute the command
        if (strlen(line) > 0) {  // Only parse non-empty input
            if (parse_command(line) != 0) {
                write(STDOUT_FILENO, "Invalid Syntax!\n", 16);
            } else {
                // Add to log if it's not a log command and not identical to previous
                if (!log_contains_log_command(line)) {
                    log_add_command(line);
                }
                
                // Check if command has semicolons (sequential execution)
                if (strchr(line, ';') != NULL) {
                    // Parse and execute as sequential commands
                    sequential_commands_t seq_cmds;
                    if (parse_sequential_commands(line, &seq_cmds) == 0) {
                        execute_sequential_commands(&seq_cmds);
                        cleanup_sequential_commands(&seq_cmds);
                    } else {
                        // Sequential parsing failed, try as pipeline
                        if (strchr(line, '|') != NULL) {
                            command_pipeline_t pipeline;
                            if (parse_pipeline(line, &pipeline) == 0) {
                                execute_pipeline(&pipeline);
                                cleanup_pipeline(&pipeline);
                            } else {
                                execute_command(line);
                            }
                        } else {
                            // Try single command with redirection
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
                    // Parse and execute as pipeline (may include background)
                    command_pipeline_t pipeline;
                    if (parse_pipeline(line, &pipeline) == 0) {
                        execute_pipeline(&pipeline);
                        cleanup_pipeline(&pipeline);
                    } else {
                        // Pipeline parsing failed, try single command
                        parsed_command_t cmd;
                        if (parse_command_with_redirection(line, &cmd) == 0) {
                            execute_command_with_redirection(&cmd);
                            cleanup_parsed_command(&cmd);
                        } else {
                            execute_command(line);
                        }
                    }
                } else {
                    // No semicolons, pipes, or background - try single command with redirection
                    parsed_command_t cmd;
                    if (parse_command_with_redirection(line, &cmd) == 0) {
                        execute_command_with_redirection(&cmd);
                        cleanup_parsed_command(&cmd);
                    } else {
                        // Fallback to original execution
                        execute_command(line);
                    }
                }
            }
        }
                  
        free(line);
        // Loop continues, which will display prompt again
    }
    return 0;
}