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
/* ############## LLM Generated Code Begins ############## */

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
        printf("Unable to create file for writing\n");
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
static int execute_builtin(parsed_command_t *cmd) {
    if (strcmp(cmd->command, "hop") == 0) {
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
        return execute_activities();
    }
    else if (strcmp(cmd->command, "ping") == 0) {
        char args_str[1024] = {0};
        for (int i = 0; i < cmd->arg_count; i++) {
            if (i > 0)
                strcat(args_str, " ");
            strcat(args_str, cmd->args[i]);
        }
        return execute_ping(args_str[0] ? args_str : NULL);
    }
    else if (strcmp(cmd->command, "fg") == 0) {
        char args_str[1024] = {0};
        for (int i = 0; i < cmd->arg_count; i++) {
            if (i > 0)
                strcat(args_str, " ");
            strcat(args_str, cmd->args[i]);
        }
        return execute_fg(args_str[0] ? args_str : NULL);
    }
    else if (strcmp(cmd->command, "bg") == 0) {
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

// Enhanced execute_command_with_redirection function in src/redirection.c
int execute_command_with_redirection(parsed_command_t *cmd)
{
    if (!cmd || !cmd->command)
    {
        return -1;
    }

    if (is_builtin_command(cmd->command))
    {
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

        int result = execute_builtin(cmd);

        fflush(stdout);
        fflush(stderr);

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

    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork failed");
        return -1;
    }

    if (pid == 0)
    {
        if (setpgid(0, 0) == -1)
        {
            perror("setpgid failed");
            exit(1);
        }

        if (handle_input_redirection(cmd->input_file) == -1)
        {
            exit(1);
        }

        if (handle_output_redirection(cmd->output_file, cmd->append_mode) == -1)
        {
            exit(1);
        }

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

        execvp(cmd->command, args);

        printf("Command not found!\n");
        free(args);
        exit(1);
    }
    else
    {
        g_foreground_pid = pid;
        g_foreground_pgid = pid;

        // strncpy(g_foreground_command, cmd->command, sizeof(g_foreground_command) - 1);
        // Build full command string with arguments
char full_cmd[256] = {0};
strncpy(full_cmd, cmd->command, sizeof(full_cmd) - 1);
for (int i = 0; i < cmd->arg_count; i++) {
    strncat(full_cmd, " ", sizeof(full_cmd) - strlen(full_cmd) - 1);
    strncat(full_cmd, cmd->args[i], sizeof(full_cmd) - strlen(full_cmd) - 1);
}
strncpy(g_foreground_command, full_cmd, sizeof(g_foreground_command) - 1);
g_foreground_command[sizeof(g_foreground_command) - 1] = '\0';


        g_foreground_command[sizeof(g_foreground_command) - 1] = '\0';

        int status;
        pid_t result;

        while (1) {
            result = waitpid(pid, &status, WUNTRACED);

            if (result == -1) {
                if (errno == EINTR) {
                    if (g_foreground_pid == 0) {
                        return 0;
                    }
                    continue;
                } else {
                    perror("waitpid failed");
                    break;
                }
            } else if (result == pid) {
                if (WIFSTOPPED(status)) {
                    return 0;
                } else {
                    break;
                }
            }
        }

        if (g_foreground_pid == pid) {
            g_foreground_pid = 0;
            g_foreground_pgid = 0;
            g_foreground_command[0] = '\0';
        }

        return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    }
}

// Execute a single command in a pipeline
static int execute_pipeline_command(parsed_command_t *cmd, int input_fd, int output_fd, pid_t pgid)
{
    if (is_builtin_command(cmd->command))
    {
        int saved_stdin = dup(STDIN_FILENO);
        int saved_stdout = dup(STDOUT_FILENO);

        if (input_fd != -1 && input_fd != STDIN_FILENO)
        {
            dup2(input_fd, STDIN_FILENO);
        }

        if (output_fd != -1 && output_fd != STDOUT_FILENO)
        {
            dup2(output_fd, STDOUT_FILENO);
        }

        if (cmd->input_file)
        {
            handle_input_redirection(cmd->input_file);
        }
        if (cmd->output_file)
        {
            handle_output_redirection(cmd->output_file, cmd->append_mode);
        }

        int result = execute_builtin(cmd);

        fflush(stdout);
        fflush(stderr);

        dup2(saved_stdin, STDIN_FILENO);
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdin);
        close(saved_stdout);

        return result;
    }

    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork failed");
        return -1;
    }

    if (pid == 0)
    {
        if (pgid == 0)
        {
            if (setpgid(0, 0) == -1)
            {
                perror("setpgid failed");
                exit(1);
            }
        }
        else
        {
            if (setpgid(0, pgid) == -1)
            {
                perror("setpgid failed");
                exit(1);
            }
        }

        if (input_fd != -1 && input_fd != STDIN_FILENO)
        {
            if (dup2(input_fd, STDIN_FILENO) == -1)
            {
                perror("dup2 failed for pipe input");
                exit(1);
            }
        }

        if (output_fd != -1 && output_fd != STDOUT_FILENO)
        {
            if (dup2(output_fd, STDOUT_FILENO) == -1)
            {
                perror("dup2 failed for pipe output");
                exit(1);
            }
        }

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

        if (input_fd != -1 && input_fd != STDIN_FILENO)
            close(input_fd);
        if (output_fd != -1 && output_fd != STDOUT_FILENO)
            close(output_fd);

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

        execvp(cmd->command, args);

        printf("Command not found!\n");
        free(args);
        exit(1);
    }

    return pid;
}

// Updated execute_pipeline function without DEBUG lines
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
            int result = execute_command_background(&pipeline->commands[0]);
            return result;
        }
        else
        {
            int result = execute_command_with_redirection(&pipeline->commands[0]);
            return result;
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
    close(pipes[i][0]); // Close read end of each pipe
    close(pipes[i][1]); // Close write end of each pipe (if not already closed)
    free(pipes[i]);
}

    int final_status = 0;

    // Handle background vs foreground execution
    if (pipeline->is_background)
    {
        // For background pipelines, add the process group leader to job management
        // and don't wait for any processes
        if (pgid > 0)
        {
            char cmd_str[256] = {0};
            strncpy(cmd_str, pipeline->commands[0].command, sizeof(cmd_str) - 1);
            if (pipeline->cmd_count > 1)
            {
                strncat(cmd_str, " | ...", sizeof(cmd_str) - strlen(cmd_str) - 1);
            }
            add_background_job_running(pgid, cmd_str);
        }
        final_status = 0;
    }
    else
    {
        // Track foreground process group
        if (pgid > 0)
        {
            g_foreground_pgid = pgid;
            g_foreground_pid = pgid; // Use process group leader PID

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
                pid_t result;

                while (1) {
                    result = waitpid(pids[i], &status, WUNTRACED);

                    if (result == -1) {
                        if (errno == EINTR) {
                            if (g_foreground_pid == 0) {
                                break;
                            }
                            continue;
                        } else {
                            perror("waitpid failed");
                            break;
                        }
                    } else if (result == pids[i]) {
                        if (WIFSTOPPED(status)) {
                            break;
                        }
                        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                            final_status = WEXITSTATUS(status);
                        }
                        break;
                    }
                }
            }
        }

        // Clear foreground process info
        g_foreground_pid = 0;
        g_foreground_pgid = 0;
        g_foreground_command[0] = '\0';
    }

    // Cleanup
    free(pipes);
    free(pids);

    return final_status;
}

// Updated execute_sequential_commands function without DEBUG lines
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
        int status = execute_pipeline(&seq_cmds->pipelines[i]);
        
        // Add explicit flushing to separate stderr from stdout
        fflush(stderr);
        fflush(stdout);

        // Record if any command failed, but continue executing
        if (status != 0)
        {
            overall_status = status;
        }
    }

    return overall_status;
}

// Execute command in background
int execute_command_background(parsed_command_t *cmd)
{
    if (!cmd || !cmd->command)
    {
        return -1;
    }

    if (is_builtin_command(cmd->command))
    {
        printf("Built-in command '%s' cannot run in background\n", cmd->command);
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork failed");
        return -1;
    }

    if (pid == 0)
    {
        int null_fd = open("/dev/null", O_RDONLY);
        if (null_fd != -1)
        {
            dup2(null_fd, STDIN_FILENO);
            close(null_fd);
        }

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

        execvp(cmd->command, args);

        printf("Command not found!\n");
        free(args);
        exit(1);
    }
    else
    {
        char full_command[256] = {0};
        strncpy(full_command, cmd->command, sizeof(full_command) - 1);
        
        for (int i = 0; i < cmd->arg_count; i++) {
            strncat(full_command, " ", sizeof(full_command) - strlen(full_command) - 1);
            strncat(full_command, cmd->args[i], sizeof(full_command) - strlen(full_command) - 1);
        }
        
        add_background_job_running(pid, full_command);
        return 0;
    }
}
/* ############## LLM Generated Code Ends ################ */