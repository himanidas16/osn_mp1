// File: src/redirection.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include "../include/redirection.h"
#include "../include/shell.h"
#include "../include/commands.h"

// Handle input redirection (Part C.1)
int handle_input_redirection(const char *filename) {
    if (!filename) {
        return 0; // No input redirection
    }
    
    // Open the input file for reading using O_RDONLY flag
    int input_fd = open(filename, O_RDONLY);
    if (input_fd == -1) {
        fprintf(stderr, "No such file or directory\n");
        return -1;
    }
    
    // Redirect stdin to the file using dup2()
    if (dup2(input_fd, STDIN_FILENO) == -1) {
        perror("dup2 failed for input redirection");
        close(input_fd);
        return -1;
    }
    
    // Close the original file descriptor to avoid leaks
    close(input_fd);
    return 0;
}

// Handle output redirection (Part C.2 - for future implementation)
int handle_output_redirection(const char *filename, int append_mode) {
    if (!filename) {
        return 0; // No output redirection
    }
    
    int output_fd;
    
    if (append_mode) {
        // >> redirection - create/append to file
        output_fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    } else {
        // > redirection - create/truncate file
        output_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    }
    
    if (output_fd == -1) {
        perror("Failed to open output file");
        return -1;
    }
    
    // Redirect stdout to the file
    if (dup2(output_fd, STDOUT_FILENO) == -1) {
        perror("dup2 failed for output redirection");
        close(output_fd);
        return -1;
    }
    
    // Close the original file descriptor to avoid leaks
    close(output_fd);
    return 0;
}

// Check if command is a built-in command
static int is_builtin_command(const char *command) {
    return (strcmp(command, "hop") == 0 || 
            strcmp(command, "reveal") == 0 || 
            strcmp(command, "log") == 0);
}

// Execute built-in command with arguments
static int execute_builtin(parsed_command_t *cmd) {
    if (strcmp(cmd->command, "hop") == 0) {
        // Build arguments string for hop
        char args_str[1024] = {0};
        for (int i = 0; i < cmd->arg_count; i++) {
            if (i > 0) strcat(args_str, " ");
            strcat(args_str, cmd->args[i]);
        }
        return execute_hop_direct(args_str[0] ? args_str : NULL);
    } else if (strcmp(cmd->command, "reveal") == 0) {
        char args_str[1024] = {0};
        for (int i = 0; i < cmd->arg_count; i++) {
            if (i > 0) strcat(args_str, " ");
            strcat(args_str, cmd->args[i]);
        }
        return execute_reveal(args_str[0] ? args_str : NULL);
    } else if (strcmp(cmd->command, "log") == 0) {
        char args_str[1024] = {0};
        for (int i = 0; i < cmd->arg_count; i++) {
            if (i > 0) strcat(args_str, " ");
            strcat(args_str, cmd->args[i]);
        }
        return execute_log(args_str[0] ? args_str : NULL);
    }
    return -1;
}

// Execute command with redirection
int execute_command_with_redirection(parsed_command_t *cmd) {
    if (!cmd || !cmd->command) {
        return -1;
    }
    
// For built-in commands, handle redirection differently
if (is_builtin_command(cmd->command)) {
    // Save original stdin/stdout
    int saved_stdin = -1, saved_stdout = -1;
    
    if (cmd->input_file) {
        saved_stdin = dup(STDIN_FILENO);
        if (handle_input_redirection(cmd->input_file) == -1) {
            if (saved_stdin != -1) close(saved_stdin);
            return -1;
        }
    }
    
    if (cmd->output_file) {
        saved_stdout = dup(STDOUT_FILENO);
        if (handle_output_redirection(cmd->output_file, cmd->append_mode) == -1) {
            if (saved_stdin != -1) {
                dup2(saved_stdin, STDIN_FILENO);
                close(saved_stdin);
            }
            if (saved_stdout != -1) close(saved_stdout);
            return -1;
        }
    }
    
    // Execute built-in command while redirection is active
    int result = execute_builtin(cmd);
    
    // CRITICAL: Flush output before restoring stdout
    fflush(stdout);
    fflush(stderr);
    
    // Restore original stdin/stdout
    if (saved_stdin != -1) {
        dup2(saved_stdin, STDIN_FILENO);
        close(saved_stdin);
    }
    if (saved_stdout != -1) {
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
    }
    
    return result;
}
    // For external commands, use fork/exec
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        return -1;
    }
    
    if (pid == 0) {
        // Child process
        
        // Handle input redirection
        if (handle_input_redirection(cmd->input_file) == -1) {
            exit(1);
        }
        
        // Handle output redirection
        if (handle_output_redirection(cmd->output_file, cmd->append_mode) == -1) {
            exit(1);
        }
        
        // Prepare arguments for execvp
        char **args = malloc((cmd->arg_count + 2) * sizeof(char*));
        if (!args) {
            perror("malloc failed");
            exit(1);
        }
        
        args[0] = cmd->command;
        for (int i = 0; i < cmd->arg_count; i++) {
            args[i + 1] = cmd->args[i];
        }
        args[cmd->arg_count + 1] = NULL;
        
        // Execute the command
        execvp(cmd->command, args);
        
        // If execvp returns, there was an error
        perror("execvp failed");
        free(args);
        exit(1);
    } else {
        // Parent process - wait for child to complete
        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    }
}

// Add this to src/redirection.c

// Execute a single command in a pipeline
static int execute_pipeline_command(parsed_command_t *cmd, int input_fd, int output_fd) {
    // Check if it's a built-in command
    if (is_builtin_command(cmd->command)) {
        // Built-in commands in pipes need special handling
        // Save original stdin/stdout
        int saved_stdin = dup(STDIN_FILENO);
        int saved_stdout = dup(STDOUT_FILENO);
        
        // Handle pipe input
        if (input_fd != -1 && input_fd != STDIN_FILENO) {
            dup2(input_fd, STDIN_FILENO);
        }
        
        // Handle pipe output
        if (output_fd != -1 && output_fd != STDOUT_FILENO) {
            dup2(output_fd, STDOUT_FILENO);
        }
        
        // Handle file redirections (these override pipe redirections)
        if (cmd->input_file) {
            handle_input_redirection(cmd->input_file);
        }
        if (cmd->output_file) {
            handle_output_redirection(cmd->output_file, cmd->append_mode);
        }
        
        // Execute built-in command
        int result = execute_builtin(cmd);
        
        // Flush output
        fflush(stdout);
        fflush(stderr);
        
        // Restore original stdin/stdout
        dup2(saved_stdin, STDIN_FILENO);
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdin);
        close(saved_stdout);
        
        return result;
    }
    
    // For external commands, fork and exec
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        return -1;
    }
    
    if (pid == 0) {
        // Child process
        
        // Handle pipe input
        if (input_fd != -1 && input_fd != STDIN_FILENO) {
            if (dup2(input_fd, STDIN_FILENO) == -1) {
                perror("dup2 failed for pipe input");
                exit(1);
            }
        }
        
        // Handle pipe output
        if (output_fd != -1 && output_fd != STDOUT_FILENO) {
            if (dup2(output_fd, STDOUT_FILENO) == -1) {
                perror("dup2 failed for pipe output");
                exit(1);
            }
        }
        
        // Handle file redirections (these override pipe redirections)
        if (cmd->input_file) {
            if (handle_input_redirection(cmd->input_file) == -1) {
                exit(1);
            }
        }
        if (cmd->output_file) {
            if (handle_output_redirection(cmd->output_file, cmd->append_mode) == -1) {
                exit(1);
            }
        }
        
        // Close all pipe file descriptors in child
        if (input_fd != -1 && input_fd != STDIN_FILENO) close(input_fd);
        if (output_fd != -1 && output_fd != STDOUT_FILENO) close(output_fd);
        
        // Prepare arguments for execvp
        char **args = malloc((cmd->arg_count + 2) * sizeof(char*));
        if (!args) {
            perror("malloc failed");
            exit(1);
        }
        
        args[0] = cmd->command;
        for (int i = 0; i < cmd->arg_count; i++) {
            args[i + 1] = cmd->args[i];
        }
        args[cmd->arg_count + 1] = NULL;
        
        // Execute the command
        execvp(cmd->command, args);
        
        // If execvp returns, there was an error
        perror("execvp failed");
        free(args);
        exit(1);
    }
    
    return pid; // Return PID for parent to wait on
}

// Execute pipeline of commands
int execute_pipeline(command_pipeline_t *pipeline) {
    if (!pipeline || pipeline->cmd_count == 0) {
        return -1;
    }
    
    // Single command - use existing redirection logic
    if (pipeline->cmd_count == 1) {
        return execute_command_with_redirection(&pipeline->commands[0]);
    }
    
    // Multiple commands - set up pipes
    int **pipes = malloc((pipeline->cmd_count - 1) * sizeof(int*));
    pid_t *pids = malloc(pipeline->cmd_count * sizeof(pid_t));
    
    if (!pipes || !pids) {
        perror("malloc failed");
        free(pipes);
        free(pids);
        return -1;
    }
    
    // Create all pipes
    for (int i = 0; i < pipeline->cmd_count - 1; i++) {
        pipes[i] = malloc(2 * sizeof(int));
        if (!pipes[i] || pipe(pipes[i]) == -1) {
            perror("pipe failed");
            // Cleanup allocated pipes
            for (int j = 0; j < i; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
                free(pipes[j]);
            }
            free(pipes);
            free(pids);
            return -1;
        }
    }
    
    // Execute each command in the pipeline
    for (int i = 0; i < pipeline->cmd_count; i++) {
        int input_fd = -1, output_fd = -1;
        
        // Set up input (from previous pipe or stdin)
        if (i > 0) {
            input_fd = pipes[i-1][0]; // Read end of previous pipe
        }
        
        // Set up output (to next pipe or stdout)
        if (i < pipeline->cmd_count - 1) {
            output_fd = pipes[i][1]; // Write end of current pipe
        }
        
        // Execute the command
        int result = execute_pipeline_command(&pipeline->commands[i], input_fd, output_fd);
        
        if (is_builtin_command(pipeline->commands[i].command)) {
            pids[i] = -1; // Built-in commands don't have PIDs
        } else {
            pids[i] = result; // Store PID for external commands
        }
        
        // Close pipe ends in parent after forking
        if (i > 0) {
            close(pipes[i-1][0]); // Close read end of previous pipe
        }
        if (i < pipeline->cmd_count - 1) {
            close(pipes[i][1]); // Close write end of current pipe
        }
    }
    
    // Close remaining pipe ends
    for (int i = 0; i < pipeline->cmd_count - 1; i++) {
        close(pipes[i][1]); // Close any remaining write ends
        if (i > 0) close(pipes[i-1][0]); // Close any remaining read ends
    }
    
    // Wait for all child processes to complete
    int final_status = 0;
    for (int i = 0; i < pipeline->cmd_count; i++) {
        if (pids[i] > 0) { // Only wait for external commands
            int status;
            waitpid(pids[i], &status, 0);
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                final_status = WEXITSTATUS(status);
            }
        }
    }
    
    // Cleanup
    for (int i = 0; i < pipeline->cmd_count - 1; i++) {
        free(pipes[i]);
    }
    free(pipes);
    free(pids);
    
    return final_status;
}