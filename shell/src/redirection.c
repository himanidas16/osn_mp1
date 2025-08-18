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
int handle_input_redirection(const char *filename)
{
    if (!filename)
    {
        return 0; // No input redirection
    }

    // Open the input file for reading using O_RDONLY flag
    int input_fd = open(filename, O_RDONLY);
    if (input_fd == -1)
    {
        fprintf(stderr, "No such file or directory\n");
        return -1;
    }

    // Redirect stdin to the file using dup2()
    if (dup2(input_fd, STDIN_FILENO) == -1)
    {
        perror("dup2 failed for input redirection");
        close(input_fd);
        return -1;
    }

    // Close the original file descriptor to avoid leaks
    close(input_fd);
    return 0;
}

// Handle output redirection (Part C.2 - for future implementation)
int handle_output_redirection(const char *filename, int append_mode)
{
    if (!filename)
    {
        return 0; // No output redirection
    }

    int output_fd;

    if (append_mode)
    {
        // >> redirection - create/append to file
        output_fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    }
    else
    {
        // > redirection - create/truncate file
        output_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    }

    if (output_fd == -1)
    {
        perror("Failed to open output file");
        return -1;
    }

    // Redirect stdout to the file
    if (dup2(output_fd, STDOUT_FILENO) == -1)
    {
        perror("dup2 failed for output redirection");
        close(output_fd);
        return -1;
    }

    // Close the original file descriptor to avoid leaks
    close(output_fd);
    return 0;
}

// Check if command is a built-in command
// Update the is_builtin_command function in src/redirection.c

// Update the is_builtin_command function in src/redirection.c

static int is_builtin_command(const char *command)
{
    return (strcmp(command, "hop") == 0 ||
            strcmp(command, "reveal") == 0 ||
            strcmp(command, "log") == 0 ||
            strcmp(command, "activities") == 0 ||
            strcmp(command, "ping") == 0 ||
            strcmp(command, "fg") == 0 ||
            strcmp(command, "bg") == 0);
}

// Execute built-in command with arguments
// Update the execute_builtin function in src/redirection.c

// Update the execute_builtin function in src/redirection.c

static int execute_builtin(parsed_command_t *cmd) {
    if (strcmp(cmd->command, "hop") == 0) {
        // Build arguments string for hop
        char args_str[1024] = {0};
        for (int i = 0; i < cmd->arg_count; i++) {
            if (i > 0)
                strcat(args_str, " ");
            strcat(args_str, cmd->args[i]);
        }
        return execute_hop_direct(args_str[0] ? args_str : NULL);
    }
    else if (strcmp(cmd->command, "reveal") == 0) {
        char args_str[1024] = {0};
        for (int i = 0; i < cmd->arg_count; i++) {
            if (i > 0)
                strcat(args_str, " ");
            strcat(args_str, cmd->args[i]);
        }
        return execute_reveal(args_str[0] ? args_str : NULL);
    }
    else if (strcmp(cmd->command, "log") == 0) {
        char args_str[1024] = {0};
        for (int i = 0; i < cmd->arg_count; i++) {
            if (i > 0)
                strcat(args_str, " ");
            strcat(args_str, cmd->args[i]);
        }
        return execute_log(args_str[0] ? args_str : NULL);
    }
    else if (strcmp(cmd->command, "activities") == 0) {
        // Activities command doesn't take arguments
        return execute_activities();
    }
    else if (strcmp(cmd->command, "ping") == 0) {
        // Build arguments string for ping
        char args_str[1024] = {0};
        for (int i = 0; i < cmd->arg_count; i++) {
            if (i > 0)
                strcat(args_str, " ");
            strcat(args_str, cmd->args[i]);
        }
        return execute_ping(args_str[0] ? args_str : NULL);
    }
    else if (strcmp(cmd->command, "fg") == 0) {
        // Build arguments string for fg
        char args_str[1024] = {0};
        for (int i = 0; i < cmd->arg_count; i++) {
            if (i > 0)
                strcat(args_str, " ");
            strcat(args_str, cmd->args[i]);
        }
        return execute_fg(args_str[0] ? args_str : NULL);
    }
    else if (strcmp(cmd->command, "bg") == 0) {
        // Build arguments string for bg
        char args_str[1024] = {0};
        for (int i = 0; i < cmd->arg_count; i++) {
            if (i > 0)
                strcat(args_str, " ");
            strcat(args_str, cmd->args[i]);
        }
        return execute_bg(args_str[0] ? args_str : NULL);
    }
    
    return -1;
}

// Execute command with redirection
// Replace the execute_command_with_redirection function in src/redirection.c
// This version adds foreground process tracking for signal handling

// Replace execute_command_with_redirection in src/redirection.c

// Replace execute_command_with_redirection in src/redirection.c with this debug version

// Replace the entire execute_command_with_redirection function in src/redirection.c with this:

int execute_command_with_redirection(parsed_command_t *cmd)
{
    if (!cmd || !cmd->command)
    {
        return -1;
    }

    // For built-in commands, handle redirection differently
    if (is_builtin_command(cmd->command))
    {
        // Save original stdin/stdout
        int saved_stdin = -1, saved_stdout = -1;

        if (cmd->input_file)
        {
            saved_stdin = dup(STDIN_FILENO);
            if (handle_input_redirection(cmd->input_file) == -1)
            {
                if (saved_stdin != -1)
                    close(saved_stdin);
                return -1;
            }
        }

        if (cmd->output_file)
        {
            saved_stdout = dup(STDOUT_FILENO);
            if (handle_output_redirection(cmd->output_file, cmd->append_mode) == -1)
            {
                if (saved_stdin != -1)
                {
                    dup2(saved_stdin, STDIN_FILENO);
                    close(saved_stdin);
                }
                if (saved_stdout != -1)
                    close(saved_stdout);
                return -1;
            }
        }

        // Execute built-in command while redirection is active
        int result = execute_builtin(cmd);

        // CRITICAL: Flush output before restoring stdout
        fflush(stdout);
        fflush(stderr);

        // Restore original stdin/stdout
        if (saved_stdin != -1)
        {
            dup2(saved_stdin, STDIN_FILENO);
            close(saved_stdin);
        }
        if (saved_stdout != -1)
        {
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);
        }

        return result;
    }

    // For external commands, use fork/exec
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork failed");
        return -1;
    }

    if (pid == 0)
    {
        // Child process

        // Create a new process group for this child
        if (setpgid(0, 0) == -1)
        {
            perror("setpgid failed");
            exit(1);
        }

        // Handle input redirection
        if (handle_input_redirection(cmd->input_file) == -1)
        {
            exit(1);
        }

        // Handle output redirection
        if (handle_output_redirection(cmd->output_file, cmd->append_mode) == -1)
        {
            exit(1);
        }

        // Prepare arguments for execvp
        char **args = malloc((cmd->arg_count + 2) * sizeof(char *));
        if (!args)
        {
            perror("malloc failed");
            exit(1);
        }

        args[0] = cmd->command;
        for (int i = 0; i < cmd->arg_count; i++)
        {
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
    else
    {
        // Parent process - track this as foreground process
        g_foreground_pid = pid;
        g_foreground_pgid = pid; // Process group ID is same as PID for process group leader
        strncpy(g_foreground_command, cmd->command, sizeof(g_foreground_command) - 1);
        g_foreground_command[sizeof(g_foreground_command) - 1] = '\0';
        printf("DEBUG: Set foreground process: PID=%d, CMD='%s'\n", g_foreground_pid, g_foreground_command);
        // Wait for child to complete
        int status;
        pid_t result;

        while (1)
        {
            result = waitpid(pid, &status, WUNTRACED);

            if (result == -1)
            {
                if (errno == EINTR)
                {
                    // Signal interrupted waitpid, try again
                    continue;
                }
                else
                {
                    perror("waitpid failed");
                    g_foreground_pid = 0;
                    g_foreground_pgid = 0;
                    g_foreground_command[0] = '\0';
                    return -1;
                }
            }
            else if (result == pid)
            {
                // Successfully got child status, break out of loop
                break;
            }
            else
            {
                continue;
            }
        }

//         if (WIFSTOPPED(status))
// {
//     // Process was stopped (Ctrl-Z) - use the new function
//     add_background_job_stopped(pid, cmd->command);
    
//     // Clear foreground process info (process is now in background)
//     g_foreground_pid = 0;
//     g_foreground_pgid = 0;
//     g_foreground_command[0] = '\0';
//     return 0;
// }

        // Process completed normally
        g_foreground_pid = 0;
        g_foreground_pgid = 0;
        g_foreground_command[0] = '\0';

        return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    }
}
// Add this to src/redirection.c

// Execute a single command in a pipeline
// Execute a single command in a pipeline
static int execute_pipeline_command(parsed_command_t *cmd, int input_fd, int output_fd, pid_t pgid)
{
    // Check if it's a built-in command
    if (is_builtin_command(cmd->command))
    {
        // Built-in commands in pipes need special handling
        // Save original stdin/stdout
        int saved_stdin = dup(STDIN_FILENO);
        int saved_stdout = dup(STDOUT_FILENO);

        // Handle pipe input
        if (input_fd != -1 && input_fd != STDIN_FILENO)
        {
            dup2(input_fd, STDIN_FILENO);
        }

        // Handle pipe output
        if (output_fd != -1 && output_fd != STDOUT_FILENO)
        {
            dup2(output_fd, STDOUT_FILENO);
        }

        // Handle file redirections (these override pipe redirections)
        if (cmd->input_file)
        {
            handle_input_redirection(cmd->input_file);
        }
        if (cmd->output_file)
        {
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
    if (pid == -1)
    {
        perror("fork failed");
        return -1;
    }

    if (pid == 0)
    {
        // Child process

        // Set process group - first process in pipeline creates new group
        if (pgid == 0)
        {
            // First process in pipeline - create new process group
            if (setpgid(0, 0) == -1)
            {
                perror("setpgid failed");
                exit(1);
            }
        }
        else
        {
            // Join existing process group
            if (setpgid(0, pgid) == -1)
            {
                perror("setpgid failed");
                exit(1);
            }
        }

        // Handle pipe input
        if (input_fd != -1 && input_fd != STDIN_FILENO)
        {
            if (dup2(input_fd, STDIN_FILENO) == -1)
            {
                perror("dup2 failed for pipe input");
                exit(1);
            }
        }

        // Handle pipe output
        if (output_fd != -1 && output_fd != STDOUT_FILENO)
        {
            if (dup2(output_fd, STDOUT_FILENO) == -1)
            {
                perror("dup2 failed for pipe output");
                exit(1);
            }
        }

        // Handle file redirections (these override pipe redirections)
        if (cmd->input_file)
        {
            if (handle_input_redirection(cmd->input_file) == -1)
            {
                exit(1);
            }
        }
        if (cmd->output_file)
        {
            if (handle_output_redirection(cmd->output_file, cmd->append_mode) == -1)
            {
                exit(1);
            }
        }

        // Close all pipe file descriptors in child
        if (input_fd != -1 && input_fd != STDIN_FILENO)
            close(input_fd);
        if (output_fd != -1 && output_fd != STDOUT_FILENO)
            close(output_fd);

        // Prepare arguments for execvp
        char **args = malloc((cmd->arg_count + 2) * sizeof(char *));
        if (!args)
        {
            perror("malloc failed");
            exit(1);
        }

        args[0] = cmd->command;
        for (int i = 0; i < cmd->arg_count; i++)
        {
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
// Replace your execute_pipeline function in src/redirection.c

// Replace your execute_pipeline function in src/redirection.c

// Execute pipeline of commands
int execute_pipeline(command_pipeline_t *pipeline)
{
    if (!pipeline || pipeline->cmd_count == 0)
    {
        return -1;
    }

    // Single command case
    if (pipeline->cmd_count == 1)
    {
        if (pipeline->is_background)
        {
            return execute_command_background(&pipeline->commands[0]);
        }
        else
        {
            return execute_command_with_redirection(&pipeline->commands[0]);
        }
    }

    // Multiple commands - set up pipes
    int **pipes = malloc((pipeline->cmd_count - 1) * sizeof(int *));
    pid_t *pids = malloc(pipeline->cmd_count * sizeof(pid_t));

    if (!pipes || !pids)
    {
        perror("malloc failed");
        free(pipes);
        free(pids);
        return -1;
    }

    // Create all pipes
    for (int i = 0; i < pipeline->cmd_count - 1; i++)
    {
        pipes[i] = malloc(2 * sizeof(int));
        if (!pipes[i] || pipe(pipes[i]) == -1)
        {
            perror("pipe failed");
            // Cleanup allocated pipes
            for (int j = 0; j < i; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
                free(pipes[j]);
            }
            free(pipes);
            free(pids);
            return -1;
        }
    }

    pid_t pgid = 0; // Process group ID for pipeline

    // Execute each command in the pipeline
    for (int i = 0; i < pipeline->cmd_count; i++)
    {
        int input_fd = -1, output_fd = -1;

        // Set up input (from previous pipe or stdin)
        if (i > 0)
        {
            input_fd = pipes[i - 1][0]; // Read end of previous pipe
        }

        // Set up output (to next pipe or stdout)
        if (i < pipeline->cmd_count - 1)
        {
            output_fd = pipes[i][1]; // Write end of current pipe
        }

        // Execute the command
        int result = execute_pipeline_command(&pipeline->commands[i], input_fd, output_fd, pgid);

        if (is_builtin_command(pipeline->commands[i].command))
        {
            pids[i] = -1; // Built-in commands don't have PIDs
        }
        else
        {
            pids[i] = result; // Store PID for external commands
            if (pgid == 0)
            {
                pgid = pids[i]; // First external process sets the process group
            }
        }

        // Close pipe ends in parent after forking
        if (i > 0)
        {
            close(pipes[i - 1][0]); // Close read end of previous pipe
        }
        if (i < pipeline->cmd_count - 1)
        {
            close(pipes[i][1]); // Close write end of current pipe
        }
    }

    // Close remaining pipe ends
    for (int i = 0; i < pipeline->cmd_count - 1; i++)
    {
        close(pipes[i][1]); // Close any remaining write ends
        if (i > 0)
            close(pipes[i - 1][0]); // Close any remaining read ends
    }

    int final_status = 0;

    // Handle background vs foreground execution
    if (pipeline->is_background)
    {
        // For background pipelines, add the last process to job management
        // and don't wait for any processes
        if (pids[pipeline->cmd_count - 1] > 0)
        {
            // Build command string for job tracking
            char cmd_str[256] = {0};
            strncpy(cmd_str, pipeline->commands[0].command, sizeof(cmd_str) - 1);
            if (pipeline->cmd_count > 1)
            {
                strncat(cmd_str, " | ...", sizeof(cmd_str) - strlen(cmd_str) - 1);
            }
            add_background_job_running(pids[pipeline->cmd_count - 1], cmd_str);
        }
        // For background processes, we don't wait, so status is 0
        final_status = 0;
    }
    else
    {
        // Track foreground process group
        if (pgid > 0)
        {
            g_foreground_pgid = pgid;
            g_foreground_pid = pids[0]; // Use first process PID

            // Build command string for signal handling
            char cmd_str[256] = {0};
            strncpy(cmd_str, pipeline->commands[0].command, sizeof(cmd_str) - 1);
            if (pipeline->cmd_count > 1)
            {
                strncat(cmd_str, " | ...", sizeof(cmd_str) - strlen(cmd_str) - 1);
            }
            strncpy(g_foreground_command, cmd_str, sizeof(g_foreground_command) - 1);
            g_foreground_command[sizeof(g_foreground_command) - 1] = '\0';
        }

        // Wait for all child processes to complete (foreground)
        for (int i = 0; i < pipeline->cmd_count; i++)
        {
            if (pids[i] > 0)
            { // Only wait for external commands
                int status;
                pid_t result = waitpid(pids[i], &status, WUNTRACED);
                if (result == -1 && errno != EINTR)
                {
                    perror("waitpid failed");
                }
                if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
                {
                    final_status = WEXITSTATUS(status);
                }
                if (WIFSTOPPED(status))
                {
                    // Process was stopped, handled by signal handler
                    // final_status = 0;
                    return 0;
                }
            }
        }

        // Clear foreground process info
        g_foreground_pid = 0;
        g_foreground_pgid = 0;
        g_foreground_command[0] = '\0';
    }

    // Cleanup
    for (int i = 0; i < pipeline->cmd_count - 1; i++)
    {
        free(pipes[i]);
    }
    free(pipes);
    free(pids);

    return final_status;
}
// part d
//  Add this function to the end of src/redirection.c

// Execute sequential commands (Part D.1)
int execute_sequential_commands(sequential_commands_t *seq_cmds)
{
    if (!seq_cmds || seq_cmds->pipeline_count == 0)
    {
        return -1;
    }

    int overall_status = 0;

    // Execute each pipeline in sequence
    for (int i = 0; i < seq_cmds->pipeline_count; i++)
    {
        // Execute the current pipeline
        int status = execute_pipeline(&seq_cmds->pipelines[i]);

        // Record if any command failed, but continue executing
        if (status != 0)
        {
            overall_status = status;
        }

        // Wait for current pipeline to complete before starting next
        // (This is handled by execute_pipeline, but we ensure it here)
    }

    return overall_status;
}

// Add this function to src/redirection.c

// Execute command in background
int execute_command_background(parsed_command_t *cmd)
{
    if (!cmd || !cmd->command)
    {
        return -1;
    }

    // Built-in commands cannot run in background (they need the shell context)
    if (is_builtin_command(cmd->command))
    {
        printf("Built-in command '%s' cannot run in background\n", cmd->command);
        return -1;
    }

    // Fork for background execution
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork failed");
        return -1;
    }

    if (pid == 0)
    {
        // Child process

        // Background processes should not have access to terminal input
        // Redirect stdin to /dev/null
        int null_fd = open("/dev/null", O_RDONLY);
        if (null_fd != -1)
        {
            dup2(null_fd, STDIN_FILENO);
            close(null_fd);
        }

        // Handle file redirections
        if (cmd->input_file)
        {
            if (handle_input_redirection(cmd->input_file) == -1)
            {
                exit(1);
            }
        }

        if (cmd->output_file)
        {
            if (handle_output_redirection(cmd->output_file, cmd->append_mode) == -1)
            {
                exit(1);
            }
        }

        // Prepare arguments for execvp
        char **args = malloc((cmd->arg_count + 2) * sizeof(char *));
        if (!args)
        {
            perror("malloc failed");
            exit(1);
        }

        args[0] = cmd->command;
        for (int i = 0; i < cmd->arg_count; i++)
        {
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
    else
    {
        // Parent process - add to background jobs and don't wait
        char full_command[256] = {0};
        strncpy(full_command, cmd->command, sizeof(full_command) - 1);
        
        // Add arguments to the command string
        for (int i = 0; i < cmd->arg_count; i++) {
            strncat(full_command, " ", sizeof(full_command) - strlen(full_command) - 1);
            strncat(full_command, cmd->args[i], sizeof(full_command) - strlen(full_command) - 1);
        }
        
        add_background_job_running(pid, full_command);
        return 0;
    }
}
