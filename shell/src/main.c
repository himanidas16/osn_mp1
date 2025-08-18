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
char g_shell_prev[PATH_MAX] = {0}; // Previous directory for hop -

// Background job storage
background_job_t g_background_jobs[MAX_BACKGROUND_JOBS];
int g_next_job_id = 1;

// Log storage
char g_log_commands[MAX_LOG_COMMANDS][1024];
int g_log_count = 0;
int g_log_start = 0;

// Signal handling globals
pid_t g_foreground_pid = 0;
pid_t g_foreground_pgid = 0;
char g_foreground_command[256] = {0};

// Signal handler flags
static volatile sig_atomic_t interrupted = 0;
static volatile sig_atomic_t suspended = 0;

// Replace the signal handler functions in main.c with these:

void sigint_handler_simple(int sig) {
    (void)sig;
    
    // Send SIGINT to foreground process group if it exists
    if (g_foreground_pgid > 0) {
        killpg(g_foreground_pgid, SIGINT);
    }
    
    // Set flag for main loop
    interrupted = 1;
}

void sigtstp_handler_simple(int sig) {
    (void)sig;
    
    // Send SIGTSTP to foreground process group if it exists
    if (g_foreground_pgid > 0) {
        killpg(g_foreground_pgid, SIGTSTP);
    }
    
    // Set flag for main loop  
    suspended = 1;
}
int main(void) {
    if (prompt_init() != 0) {
        fprintf(stderr, "Failed to initialize prompt\n");
        return 1;
    }
    
    // Initialize log system
    log_init();
    
    // Initialize background job management
    init_background_jobs();
    
    // Set up signal handlers
    signal(SIGINT, sigint_handler_simple);   // Ctrl-C
    signal(SIGTSTP, sigtstp_handler_simple); // Ctrl-Z

    for (;;) {
        // Reset interrupt flags
        interrupted = 0;
        suspended = 0;
        
        // Check for completed background jobs BEFORE showing prompt
        check_background_jobs();
        
        char p[SHELL_PROMPT_MAX];
        if (prompt_build(p, sizeof p) == 0) {
            printf("%s", p);
            fflush(stdout);
        }

        char *line = NULL; 
        size_t cap = 0;
        
        ssize_t n = getline(&line, &cap, stdin);
        
        // Check if we were interrupted by Ctrl-C
        if (interrupted) {
            printf("\n");
            // Clear foreground process info
            g_foreground_pid = 0;
            g_foreground_pgid = 0;
            g_foreground_command[0] = '\0';
            if (line) free(line);
            continue;
        }
        
        // Check if we were suspended by Ctrl-Z  
        if (suspended) {
            printf("\n");
            
            // If there was a foreground process, it should have been stopped
            // The waitpid() in execute_command_with_redirection will handle adding it to background jobs
            
            if (line) free(line);
            continue;
        }
        
        if (n < 0) {
            if (feof(stdin)) {
                // EOF (Ctrl-D) detected
                printf("\nlogout\n");
                // Send SIGKILL to all active background processes
                for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
                    if (g_background_jobs[i].is_active && g_background_jobs[i].pid > 0) {
                        kill(g_background_jobs[i].pid, SIGKILL);
                    }
                }
                if (line) free(line);
                exit(0);
            } else if (errno == EINTR) {
                // Signal interrupted getline
                printf("\n");
                if (line) free(line);
                
                // Clear the interrupted state of stdin
                clearerr(stdin);
                errno = 0;
                continue; 
            } else {
                perror("getline");
                if (line) free(line);
                break;
            }
        }

        // Remove trailing newline if present
        if (n > 0 && line[n-1] == '\n') {
            line[n-1] = '\0';
            n--;
        }

        // Check for completed background jobs AFTER user input
        check_background_jobs();

        // Parse and execute the command
        if (strlen(line) > 0) {  // Only parse non-empty input
            if (parse_command(line) != 0) {
                printf("Invalid Syntax!\n");
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
    }
    return 0;
}