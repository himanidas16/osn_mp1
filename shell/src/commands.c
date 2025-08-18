#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include "shell.h"
#include "commands.h"
#include <signal.h> // Include this header for struct sigaction

#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>        // Add this line
#include <sys/types.h>    // Add this line

// Helper function to trim whitespace
static char *trim_whitespace(char *str)
{
    char *end;

    // Trim leading space
    while (*str == ' ' || *str == '\t')
        str++;

    if (*str == 0)
        return str; // All spaces?

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t'))
        end--;

    end[1] = '\0';
    return str;
}

// Execute hop command
 int execute_hop(char *args)
{
    char current_dir[PATH_MAX];
    if (!getcwd(current_dir, sizeof(current_dir)))
    {
        perror("hop: getcwd failed");
        return -1;
    }

    // If no arguments, go to home
    if (!args || strlen(trim_whitespace(args)) == 0)
    {
        if (chdir(g_shell_home) != 0)
        {
            perror("hop");
            return -1;
        }
        strncpy(g_shell_prev, current_dir, sizeof(g_shell_prev) - 1);
        g_shell_prev[sizeof(g_shell_prev) - 1] = '\0';
        return 0;
    }

    // Parse arguments (space separated)
    char *token = strtok(args, " \t");
    while (token != NULL)
    {
        char target_dir[PATH_MAX];

        if (strcmp(token, "~") == 0)
        {
            // Go to home directory
            strncpy(target_dir, g_shell_home, sizeof(target_dir) - 1);
        }
        else if (strcmp(token, ".") == 0)
        {
            // Stay in current directory - do nothing
            token = strtok(NULL, " \t");
            continue;
        }
        else if (strcmp(token, "..") == 0)
        {
            // Go to parent directory
            strncpy(target_dir, current_dir, sizeof(target_dir) - 1);
            char *last_slash = strrchr(target_dir, '/');
            if (last_slash && last_slash != target_dir)
            {
                *last_slash = '\0'; // Remove last component
            }
            else if (last_slash == target_dir)
            {
                target_dir[1] = '\0'; // Root directory case
            }
            // If no parent, stay where we are
            if (strcmp(target_dir, current_dir) == 0)
            {
                token = strtok(NULL, " \t");
                continue;
            }
        }
        else if (strcmp(token, "-") == 0)
        {
            // Go to previous directory
            if (g_shell_prev[0] == '\0')
            {
                // No previous directory, do nothing
                token = strtok(NULL, " \t");
                continue;
            }
            strncpy(target_dir, g_shell_prev, sizeof(target_dir) - 1);
        }
        else
        {
            // Regular path (relative or absolute)
            strncpy(target_dir, token, sizeof(target_dir) - 1);
        }

        target_dir[sizeof(target_dir) - 1] = '\0';

        // Save current directory before changing
        char temp_prev[PATH_MAX];
        strncpy(temp_prev, current_dir, sizeof(temp_prev) - 1);
        temp_prev[sizeof(temp_prev) - 1] = '\0';

        // Try to change directory
        if (chdir(target_dir) != 0)
        {
            perror("hop");
            return -1;
        }

        // Update previous directory and current directory
        strncpy(g_shell_prev, temp_prev, sizeof(g_shell_prev) - 1);
        g_shell_prev[sizeof(g_shell_prev) - 1] = '\0';

        // Update current_dir for next iteration
        if (!getcwd(current_dir, sizeof(current_dir)))
        {
            perror("hop: getcwd failed");
            return -1;
        }

        token = strtok(NULL, " \t");
    }

    return 0;
}

// Main command execution function
// Main command execution function
// Update your execute_command function in src/commands.c
// Add this check after the activities command check:

// Main command execution function
int execute_command(const char *input)
{
    if (!input || strlen(input) == 0)
    {
        return 0;
    }

    // Make a copy of input for parsing
    char *input_copy = malloc(strlen(input) + 1);
    if (!input_copy)
    {
        perror("malloc failed");
        return -1;
    }
    strcpy(input_copy, input);

    // Trim leading whitespace
    char *cmd = input_copy;
    while (*cmd == ' ' || *cmd == '\t')
        cmd++;

    // Check if it's a hop command
    if (strncmp(cmd, "hop", 3) == 0 && (cmd[3] == ' ' || cmd[3] == '\t' || cmd[3] == '\0'))
    {
        char *args = NULL;
        if (cmd[3] != '\0')
        {
            args = cmd + 3;
        }
        int result = execute_hop(args);
        free(input_copy);
        return result;
    }

     
    // Check if it's a reveal command
    if (strncmp(cmd, "reveal", 6) == 0 && (cmd[6] == ' ' || cmd[6] == '\t' || cmd[6] == '\0')) {
        char *args = NULL;
        if (cmd[6] != '\0') {
            args = cmd + 6;
        }
        int result = execute_reveal(args);
        free(input_copy);
        return result;
    }

    // Check if it's a log command
    if (strncmp(cmd, "log", 3) == 0 && (cmd[3] == ' ' || cmd[3] == '\t' || cmd[3] == '\0')) {
        char *args = NULL;
        if (cmd[3] != '\0') {
            args = cmd + 3;
        }
        int result = execute_log(args);
        free(input_copy);
        return result;
    }

    // Check if it's an activities command
    if (strncmp(cmd, "activities", 10) == 0 && (cmd[10] == ' ' || cmd[10] == '\t' || cmd[10] == '\0')) {
        int result = execute_activities();
        free(input_copy);
        return result;
    }

    // Check if it's a ping command
    if (strncmp(cmd, "ping", 4) == 0 && (cmd[4] == ' ' || cmd[4] == '\t' || cmd[4] == '\0')) {
        char *args = NULL;
        if (cmd[4] != '\0') {
            args = cmd + 4;
        }
        int result = execute_ping(args);
        free(input_copy);
        return result;
    }
 // Check if it's a fg command
 if (strncmp(cmd, "fg", 2) == 0 && (cmd[2] == ' ' || cmd[2] == '\t' || cmd[2] == '\0')) {
    char *args = NULL;
    if (cmd[2] != '\0') {
        args = cmd + 2;
    }
    int result = execute_fg(args);
    free(input_copy);
    return result;
}

// Check if it's a bg command
if (strncmp(cmd, "bg", 2) == 0 && (cmd[2] == ' ' || cmd[2] == '\t' || cmd[2] == '\0')) {
    char *args = NULL;
    if (cmd[2] != '\0') {
        args = cmd + 2;
    }
    int result = execute_bg(args);
    free(input_copy);
    return result;
}

    // For now, other commands do nothing (will implement later)
    free(input_copy);
    return 0;
}

//reveal

// Helper function to check if a file is hidden (starts with .)
static int is_hidden_file(const char *name) {
    return name[0] == '.';
}

// Comparison function for qsort (lexicographic order)
static int compare_strings(const void *a, const void *b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

// Execute reveal command
int execute_reveal(char *args) {
    int show_all = 0;    // -a flag
    int line_format = 0; // -l flag
    char target_dir[PATH_MAX];

    // Default to current directory
    strncpy(target_dir, ".", sizeof(target_dir) - 1);
    target_dir[sizeof(target_dir) - 1] = '\0';

    // Parse flags and (optional) path argument
    if (args) {
        char *args_copy = strdup(args);
        if (!args_copy) {
            perror("reveal: malloc failed");
            return -1;
        }

        char *token = strtok(args_copy, " \t");
        while (token) {
            if (token[0] == '-') {
                // Accept combined/duplicated flags like -lalalaa -aaaa
                for (int i = 1; token[i] != '\0'; i++) {
                    if (token[i] == 'a') show_all = 1;
                    else if (token[i] == 'l') line_format = 1;
                    // ignore unknown chars
                }
            } else {
                // Single directory argument (behaves like hop's argument)
                if (strcmp(token, "~") == 0) {
                    strncpy(target_dir, g_shell_home, sizeof(target_dir) - 1);
                } else if (strcmp(token, ".") == 0) {
                    strncpy(target_dir, ".", sizeof(target_dir) - 1);
                } else if (strcmp(token, "..") == 0) {
                    strncpy(target_dir, "..", sizeof(target_dir) - 1);
                } else if (strcmp(token, "-") == 0) {
                    if (g_shell_prev[0] != '\0') {
                        strncpy(target_dir, g_shell_prev, sizeof(target_dir) - 1);
                    } else {
                        // No previous dir → stay in current dir
                        strncpy(target_dir, ".", sizeof(target_dir) - 1);
                    }
                } else {
                    strncpy(target_dir, token, sizeof(target_dir) - 1);
                }
                target_dir[sizeof(target_dir) - 1] = '\0';
                break; // only first non-flag token is a path
            }
            token = strtok(NULL, " \t");
        }

        free(args_copy);
    }

    // Open directory
    DIR *dir = opendir(target_dir);
    if (!dir) {
        perror("reveal");
        return -1;
    }

    // Collect entries
    struct dirent *entry;
    int capacity = 16, count = 0;
    char **filenames = (char **)malloc(capacity * sizeof(char *));
    if (!filenames) {
        perror("reveal: malloc failed");
        closedir(dir);
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        // Skip hidden files unless -a is set (this includes "." and "..")
        if (!show_all && is_hidden_file(entry->d_name)) continue;

        if (count == capacity) {
            capacity <<= 1;
            char **tmp = (char **)realloc(filenames, capacity * sizeof(char *));
            if (!tmp) {
                perror("reveal: realloc failed");
                for (int i = 0; i < count; i++) free(filenames[i]);
                free(filenames);
                closedir(dir);
                return -1;
            }
            filenames = tmp;
        }

        filenames[count] = strdup(entry->d_name);
        if (!filenames[count]) {
            perror("reveal: malloc failed");
            for (int i = 0; i < count; i++) free(filenames[i]);
            free(filenames);
            closedir(dir);
            return -1;
        }
        count++;
    }
    closedir(dir);

    // Sort lexicographically
    qsort(filenames, count, sizeof(char *), compare_strings);

    // Print
    if (line_format) {
        for (int i = 0; i < count; i++) {
            printf("%s\n", filenames[i]);
        }
    } else {
        for (int i = 0; i < count; i++) {
            if (i) printf(" ");
            printf("%s", filenames[i]);
        }
        if (count) printf("\n");
    }

    // Cleanup
    for (int i = 0; i < count; i++) free(filenames[i]);
    free(filenames);

    return 0;
}


// Initialize log system - load from file
int log_init(void) {
    // Initialize log arrays
    g_log_count = 0;
    g_log_start = 0;
    
    // Try to load existing log from file
    char log_path[PATH_MAX];
    strcpy(log_path, g_shell_home);
    strcat(log_path, "/");
    strcat(log_path, LOG_FILENAME);
    
    FILE *file = fopen(log_path, "r");
    if (!file) {
        // File doesn't exist, start with empty log
        return 0;
    }
    
    char line[1024];
    while (fgets(line, sizeof(line), file) && g_log_count < MAX_LOG_COMMANDS) {
        // Remove newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        strncpy(g_log_commands[g_log_count], line, sizeof(g_log_commands[g_log_count]) - 1);
        g_log_commands[g_log_count][sizeof(g_log_commands[g_log_count]) - 1] = '\0';
        g_log_count++;
    }
    
    fclose(file);
    return 0;
}

// Save log to file
static int log_save(void) {
    char log_path[PATH_MAX];
    strcpy(log_path, g_shell_home);
    strcat(log_path, "/");
    strcat(log_path, LOG_FILENAME);
    
    FILE *file = fopen(log_path, "w");
    if (!file) {
        perror("log: failed to save history");
        return -1;
    }
    
    // Write commands in order (oldest to newest)
    for (int i = 0; i < g_log_count; i++) {
        int idx = (g_log_start + i) % MAX_LOG_COMMANDS;
        fprintf(file, "%s\n", g_log_commands[idx]);
    }
    
    fclose(file);
    return 0;
}

// Check if command contains 'log' as a command name
int log_contains_log_command(const char *command) {
    if (!command) return 0;
    
    // Simple check: if command starts with "log" followed by space, tab, or end
    char *cmd_copy = malloc(strlen(command) + 1);
    if (!cmd_copy) return 0;
    
    strcpy(cmd_copy, command);
    
    // Trim leading whitespace
    char *cmd = cmd_copy;
    while (*cmd == ' ' || *cmd == '\t') cmd++;
    
    int contains_log = 0;
    if (strncmp(cmd, "log", 3) == 0 && (cmd[3] == ' ' || cmd[3] == '\t' || cmd[3] == '\0')) {
        contains_log = 1;
    }
    
    free(cmd_copy);
    return contains_log;
}

// Add command to log
void log_add_command(const char *command) {
    if (!command || strlen(command) == 0) return;
    
    // Check if identical to previous command
    if (g_log_count > 0) {
        int last_idx = (g_log_start + g_log_count - 1) % MAX_LOG_COMMANDS;
        if (strcmp(g_log_commands[last_idx], command) == 0) {
            return; // Don't add identical command
        }
    }
    
    if (g_log_count < MAX_LOG_COMMANDS) {
        // Still have space
        strncpy(g_log_commands[g_log_count], command, sizeof(g_log_commands[g_log_count]) - 1);
        g_log_commands[g_log_count][sizeof(g_log_commands[g_log_count]) - 1] = '\0';
        g_log_count++;
    } else {
        // Overwrite oldest command
        strncpy(g_log_commands[g_log_start], command, sizeof(g_log_commands[g_log_start]) - 1);
        g_log_commands[g_log_start][sizeof(g_log_commands[g_log_start]) - 1] = '\0';
        g_log_start = (g_log_start + 1) % MAX_LOG_COMMANDS;
    }
    
    // Save to file
    log_save();
}

// Execute log command
int execute_log(char *args) {
    if (!args || strlen(trim_whitespace(args)) == 0) {
        // No arguments: print commands oldest to newest
        for (int i = 0; i < g_log_count; i++) {
            int idx = (g_log_start + i) % MAX_LOG_COMMANDS;
            printf("%s\n", g_log_commands[idx]);
        }
        return 0;
    }
    
    // Parse arguments
    char *args_copy = malloc(strlen(args) + 1);
    if (!args_copy) {
        perror("log: malloc failed");
        return -1;
    }
    strcpy(args_copy, args);
    
    char *token = strtok(args_copy, " \t");
    if (!token) {
        free(args_copy);
        return 0;
    }
    
    if (strcmp(token, "purge") == 0) {
        // Clear history
        g_log_count = 0;
        g_log_start = 0;
        
        // Clear file
        char log_path[PATH_MAX];
        strcpy(log_path, g_shell_home);
        strcat(log_path, "/");
        strcat(log_path, LOG_FILENAME);
        FILE *file = fopen(log_path, "w");
        if (file) {
            fclose(file);
        }
        
        free(args_copy);
        return 0;
    } else if (strcmp(token, "execute") == 0) {
        // Execute command at index
        token = strtok(NULL, " \t");
        if (!token) {
            printf("log: execute requires an index\n");
            free(args_copy);
            return -1;
        }
        
        int index = atoi(token);
        if (index < 1 || index > g_log_count) {
            printf("log: invalid index %d\n", index);
            free(args_copy);
            return -1;
        }
        
        // Convert to 0-based index (newest to oldest)
        int cmd_idx = (g_log_start + g_log_count - index) % MAX_LOG_COMMANDS;
        
        // Execute the command without adding to log
        printf("%s\n", g_log_commands[cmd_idx]); // Show what we're executing
        execute_command(g_log_commands[cmd_idx]);
        
        free(args_copy);
        return 0;
    } else {
        printf("log: unknown argument '%s'\n", token);
        free(args_copy);
        return -1;
    }
}





// Add this function to src/commands.c after the existing execute_hop function

// Direct hop execution for redirection
int execute_hop_direct(char *args) {
    return execute_hop(args);
}


//part e1

// Initialize background job management
void init_background_jobs(void) {
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        g_background_jobs[i].is_active = 0;
        g_background_jobs[i].job_id = 0;
        g_background_jobs[i].pid = 0;
        g_background_jobs[i].command[0] = '\0';
        g_background_jobs[i].state = PROCESS_TERMINATED;
    }
    g_next_job_id = 1;
}

// Add a new background job
// Add a new background job
int add_background_job(pid_t pid, const char *command) {
    // Find an empty slot
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (!g_background_jobs[i].is_active) {
            g_background_jobs[i].job_id = g_next_job_id++;
            g_background_jobs[i].pid = pid;
            g_background_jobs[i].is_active = 1;
            g_background_jobs[i].state = PROCESS_RUNNING;
            
            // Copy command name (truncate if too long)
            strncpy(g_background_jobs[i].command, command, sizeof(g_background_jobs[i].command) - 1);
            g_background_jobs[i].command[sizeof(g_background_jobs[i].command) - 1] = '\0';
            
            // DON'T print job information here anymore - let the caller handle it
            
            return g_background_jobs[i].job_id;
        }
    }
    return -1; // No available slots
}


// Add these RIGHT AFTER your existing add_background_job function in src/commands.c

// For background jobs (sleep 30 &)
int add_background_job_running(pid_t pid, const char *command) {
    int job_id = add_background_job(pid, command);
    if (job_id > 0) {
        printf("[%d] %d\n", job_id, pid);
        fflush(stdout);
    }
    return job_id;
}

// For stopped jobs (Ctrl-Z) 
int add_background_job_stopped(pid_t pid, const char *command) {
    int job_id = add_background_job(pid, command);
    if (job_id > 0) {
        // Update state to stopped
        for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
            if (g_background_jobs[i].is_active && g_background_jobs[i].pid == pid) {
                g_background_jobs[i].state = PROCESS_STOPPED;
                break;
            }
        }
        printf("[%d] Stopped %s\n", job_id, command);
        fflush(stdout);
    }
    return job_id;
}


// Check for completed background jobs (non-blocking)
void check_background_jobs(void) {
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (g_background_jobs[i].is_active) {
            int status;
            #ifdef WCONTINUED
                        pid_t result = waitpid(g_background_jobs[i].pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
            #else
                        pid_t result = waitpid(g_background_jobs[i].pid, &status, WNOHANG | WUNTRACED);
            #endif
            
            if (result == g_background_jobs[i].pid) {
                if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    // Process has terminated
                    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                        printf("%s with pid %d exited normally\n", 
                               g_background_jobs[i].command, g_background_jobs[i].pid);
                    } else {
                        printf("%s with pid %d exited abnormally\n", 
                               g_background_jobs[i].command, g_background_jobs[i].pid);
                    }
                    fflush(stdout);
                    
                    // Mark job as inactive (remove from list)
                    g_background_jobs[i].is_active = 0;
                    g_background_jobs[i].state = PROCESS_TERMINATED;
                } else if (WIFSTOPPED(status)) {
                    // Process has been stopped (Ctrl+Z)
                    g_background_jobs[i].state = PROCESS_STOPPED;
                } else if (WIFCONTINUED(status)) {
                    // Process has been continued
                    g_background_jobs[i].state = PROCESS_RUNNING;
                }
            } else if (result == -1) {
                // Error or process doesn't exist anymore
                printf("%s with pid %d exited abnormally\n", 
                       g_background_jobs[i].command, g_background_jobs[i].pid);
                fflush(stdout);
                g_background_jobs[i].is_active = 0;
                g_background_jobs[i].state = PROCESS_TERMINATED;
            }
            // result == 0 means process is still running (no state change)
        }
    }
}

// Cleanup a specific background job
void cleanup_background_job(int index) {
    if (index >= 0 && index < MAX_BACKGROUND_JOBS) {
        g_background_jobs[index].is_active = 0;
        g_background_jobs[index].job_id = 0;
        g_background_jobs[index].pid = 0;
        g_background_jobs[index].command[0] = '\0';
        g_background_jobs[index].state = PROCESS_TERMINATED;
    }
}

// Helper structure for sorting activities
typedef struct {
    pid_t pid;
    char command[256];
    process_state_t state;
} activity_entry_t;

// Comparison function for sorting activities by command name
static int compare_activities(const void *a, const void *b) {
    const activity_entry_t *entry_a = (const activity_entry_t *)a;
    const activity_entry_t *entry_b = (const activity_entry_t *)b;
    return strcmp(entry_a->command, entry_b->command);
}

// Execute activities command
int execute_activities(void) {
    activity_entry_t activities[MAX_BACKGROUND_JOBS];
    int activity_count = 0;
    
    // Collect all active processes
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (g_background_jobs[i].is_active) {
            activities[activity_count].pid = g_background_jobs[i].pid;
            strncpy(activities[activity_count].command, g_background_jobs[i].command, 
                   sizeof(activities[activity_count].command) - 1);
            activities[activity_count].command[sizeof(activities[activity_count].command) - 1] = '\0';
            activities[activity_count].state = g_background_jobs[i].state;
            activity_count++;
        }
    }
    
    // Sort activities lexicographically by command name
    qsort(activities, activity_count, sizeof(activity_entry_t), compare_activities);
    
    // Print sorted activities
    for (int i = 0; i < activity_count; i++) {
        const char *state_str;
        switch (activities[i].state) {
            case PROCESS_RUNNING:
                state_str = "Running";
                break;
            case PROCESS_STOPPED:
                state_str = "Stopped";
                break;
            default:
                state_str = "Unknown";
                break;
        }
        
        printf("[%d] : %s - %s\n", activities[i].pid, activities[i].command, state_str);
    }
    
    return 0;
}


//part e2
// Add this function to the END of src/commands.c

// Execute ping command
int execute_ping(char *args) {
    if (!args || strlen(trim_whitespace(args)) == 0) {
        printf("ping: requires <pid> <signal_number>\n");
        return -1;
    }
    
    // Parse arguments
    char *args_copy = malloc(strlen(args) + 1);
    if (!args_copy) {
        perror("ping: malloc failed");
        return -1;
    }
    strcpy(args_copy, args);
    
    char *token = strtok(args_copy, " \t");
    if (!token) {
        printf("ping: requires <pid> <signal_number>\n");
        free(args_copy);
        return -1;
    }
    
    // Parse PID
    char *endptr;
    long pid_long = strtol(token, &endptr, 10);
    if (*endptr != '\0' || pid_long <= 0) {
        printf("ping: invalid PID '%s'\n", token);
        free(args_copy);
        return -1;
    }
    pid_t pid = (pid_t)pid_long;
    
    // Parse signal number
    token = strtok(NULL, " \t");
    if (!token) {
        printf("ping: requires <pid> <signal_number>\n");
        free(args_copy);
        return -1;
    }
    
    long signal_long = strtol(token, &endptr, 10);
    if (*endptr != '\0') {
        printf("ping: invalid signal number '%s'\n", token);
        free(args_copy);
        return -1;
    }
    
    // Apply modulo 32 to signal number (Requirement 1)
    int original_signal = (int)signal_long;
    int actual_signal = original_signal % 32;
    
    // Check for extra arguments
    token = strtok(NULL, " \t");
    if (token) {
        printf("ping: too many arguments\n");
        free(args_copy);
        return -1;
    }
    
    // Send signal to process
    if (kill(pid, actual_signal) == -1) {
        // Check if process exists
        if (errno == ESRCH) {
            printf("No such process found\n");
        } else {
            perror("ping: failed to send signal");
        }
        free(args_copy);
        return -1;
    }
    
    // Success message (Requirement 3)
    printf("Sent signal %d to process with pid %d\n", original_signal, pid);
    
    free(args_copy);
    return 0;
}
  //part e3 

  // Add these functions to the END of src/commands.c

// Add these signal handling functions to the END of src/commands.c
// Replace the existing signal handling functions with these:

// Setup signal handlers
// Alternative signal handling using signal() instead of sigaction()

// Setup signal handlers
// Add these to commands.c - COMPLETE REPLACEMENT

// Add these to commands.c - COMPLETE REPLACEMENT

// Global flags to indicate if we received signals
// volatile sig_atomic_t sigint_received = 0;
// volatile sig_atomic_t sigtstp_received = 0;

// SIGINT handler (Ctrl-C) - MINIMAL VERSION
// SIGINT handler (Ctrl-C) - MINIMAL VERSION
// void sigint_handler(int sig) {
//     (void)sig;
//     write(STDERR_FILENO, "DEBUG: SIGINT handler called\n", 30);
    
//     // Only send signal to foreground process group if one exists
//     if (g_foreground_pgid > 0) {
//         killpg(g_foreground_pgid, SIGINT);
//         write(STDERR_FILENO, "DEBUG: Sent SIGINT to process group\n", 37);
//     } else {
//         write(STDERR_FILENO, "DEBUG: No foreground process group\n", 36);
//     }
    
//     // Set flag for main loop
//     sigint_received = 1;
// }

// void sigtstp_handler(int sig) {
//     (void)sig;
//     write(STDERR_FILENO, "DEBUG: SIGTSTP handler called\n", 31);
    
//     // Only send signal to foreground process group if one exists
//     if (g_foreground_pgid > 0) {
//         killpg(g_foreground_pgid, SIGTSTP);
//         write(STDERR_FILENO, "DEBUG: Sent SIGTSTP to process group\n", 38);
//     } else {
//         write(STDERR_FILENO, "DEBUG: No foreground process group\n", 36);
//     }
    
//     // Set flag for main loop  
//     sigtstp_received = 1;
// }
void sigint_handler(int sig) {
    (void)sig;
    
    // Only send signal to foreground process group if one exists
    if (g_foreground_pgid > 0) {
        killpg(g_foreground_pgid, SIGINT);
    }
    
    // Clear foreground process info immediately
    g_foreground_pid = 0;
    g_foreground_pgid = 0;
    g_foreground_command[0] = '\0';
}

void sigtstp_handler(int sig) {
    (void)sig;
    
    // Only send signal to foreground process group if one exists
    if (g_foreground_pgid > 0) {
        killpg(g_foreground_pgid, SIGTSTP);
        
        // Add stopped job immediately (THIS is the key fix)
        add_background_job_stopped(g_foreground_pid, g_foreground_command);
        
        // Clear foreground process info immediately
        g_foreground_pid = 0;
        g_foreground_pgid = 0;
        g_foreground_command[0] = '\0';
    }
}
// Setup signal handlers using simple signal() function
void setup_signal_handlers(void) {
    // Setup SIGINT handler (Ctrl-C)
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("signal SIGINT");
    }
    
    // Setup SIGTSTP handler (Ctrl-Z)
    if (signal(SIGTSTP, sigtstp_handler) == SIG_ERR) {
        perror("signal SIGTSTP");
    }
    
    // Ignore SIGTTOU to avoid being stopped when writing to terminal
    signal(SIGTTOU, SIG_IGN);
}

// Function to handle signals after main loop iteration
// void handle_pending_signals(void) {
//     if (sigint_received) {
//         printf("DEBUG: Processing SIGINT flag\n");
//         sigint_received = 0;
        
//         // Clear foreground process info
//         g_foreground_pid = 0;
//         g_foreground_pgid = 0;
//         g_foreground_command[0] = '\0';
//     }
    
//     if (sigtstp_received) {
//         printf("DEBUG: Processing SIGTSTP flag, fg_pid=%d, fg_cmd='%s'\n", 
//                g_foreground_pid, g_foreground_command);
//         sigtstp_received = 0;
        
//         if (g_foreground_pid > 0) {
//             printf("DEBUG: Adding stopped job for PID %d\n", g_foreground_pid);
//             add_background_job_stopped(g_foreground_pid, g_foreground_command);
            
//             // Clear foreground process info
//             g_foreground_pid = 0;
//             g_foreground_pgid = 0;
//             g_foreground_command[0] = '\0';
//         } else {
//             printf("DEBUG: No foreground process to stop\n");
//         }
//     }
// }

// Cleanup and exit function (for Ctrl-D)
void cleanup_and_exit(void) {
    printf("logout\n");
    
    // Send SIGKILL to all active background processes
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (g_background_jobs[i].is_active && g_background_jobs[i].pid > 0) {
            kill(g_background_jobs[i].pid, SIGKILL);
        }
    }
    
    exit(0);
}



//part e3
// Add these functions to the end of src/commands.c

// Helper function to find job by job ID
background_job_t* find_job_by_id(int job_id) {
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (g_background_jobs[i].is_active && g_background_jobs[i].job_id == job_id) {
            return &g_background_jobs[i];
        }
    }
    return NULL;
}

// Helper function to find most recent job (highest job_id)
background_job_t* find_most_recent_job(void) {
    background_job_t* most_recent = NULL;
    int highest_job_id = 0;
    
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (g_background_jobs[i].is_active && g_background_jobs[i].job_id > highest_job_id) {
            highest_job_id = g_background_jobs[i].job_id;
            most_recent = &g_background_jobs[i];
        }
    }
    return most_recent;
}

// Execute fg command
// Replace your execute_fg function with this improved version

// Replace your execute_fg function with this improved version

int execute_fg(char *args) {
    background_job_t* job = NULL;
    
    if (!args || strlen(trim_whitespace(args)) == 0) {
        // No job number provided, use most recent job
        job = find_most_recent_job();
        if (!job) {
            printf("No jobs in background\n");
            return -1;
        }
    } else {
        // Parse job number
        char *args_copy = malloc(strlen(args) + 1);
        if (!args_copy) {
            perror("fg: malloc failed");
            return -1;
        }
        strcpy(args_copy, args);
        
        char *token = strtok(args_copy, " \t");
        if (!token) {
            printf("fg: invalid job number\n");
            free(args_copy);
            return -1;
        }
        
        char *endptr;
        long job_id_long = strtol(token, &endptr, 10);
        if (*endptr != '\0' || job_id_long <= 0) {
            printf("fg: invalid job number '%s'\n", token);
            free(args_copy);
            return -1;
        }
        
        int job_id = (int)job_id_long;
        job = find_job_by_id(job_id);
        if (!job) {
            printf("No such job\n");
            free(args_copy);
            return -1;
        }
        
        free(args_copy);
    }
    
    // Check if the process still exists
    if (kill(job->pid, 0) == -1) {
        if (errno == ESRCH) {
            printf("No such job\n");
            job->is_active = 0;
            return -1;
        }
    }
    
    // Print the command being brought to foreground
    printf("%s\n", job->command);
    fflush(stdout);
    
    // Save job info before removing from background list
    pid_t job_pid = job->pid;
    char job_command[256];
    strncpy(job_command, job->command, sizeof(job_command) - 1);
    job_command[sizeof(job_command) - 1] = '\0';
    process_state_t job_state = job->state;
    int original_job_id = job->job_id;  // SAVE ORIGINAL JOB ID
    
    // Remove job from background jobs list BEFORE setting as foreground
    job->is_active = 0;
    
    // Set this job as the foreground job
    g_foreground_pid = job_pid;
    g_foreground_pgid = job_pid;
    strncpy(g_foreground_command, job_command, sizeof(g_foreground_command) - 1);
    g_foreground_command[sizeof(g_foreground_command) - 1] = '\0';
    
    // If job is stopped, send SIGCONT to resume it
    if (job_state == PROCESS_STOPPED) {
        if (kill(job_pid, SIGCONT) == -1) {
            if (errno == ESRCH) {
                printf("No such job\n");
                g_foreground_pid = 0;
                g_foreground_pgid = 0;
                g_foreground_command[0] = '\0';
                return -1;
            }
            perror("fg: failed to send SIGCONT");
            g_foreground_pid = 0;
            g_foreground_pgid = 0;
            g_foreground_command[0] = '\0';
            return -1;
        }
    }
    
    // Wait for the job to complete or stop again
    int status;
    pid_t result;
    
    while (1) {
        result = waitpid(job_pid, &status, WUNTRACED);
        
        if (result == -1) {
            if (errno == EINTR) {
                continue;
            } else if (errno == ECHILD) {
                break;
            } else {
                perror("fg: waitpid failed");
                g_foreground_pid = 0;
                g_foreground_pgid = 0;
                g_foreground_command[0] = '\0';
                return -1;
            }
        } else if (result == job_pid) {
            break;
        } else {
            continue;
        }
    }
    
    if (WIFSTOPPED(status)) {
        // Process was stopped again (Ctrl-Z), put it back in background
        // Restore the job with original job ID
        for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
            if (!g_background_jobs[i].is_active) {
                g_background_jobs[i].job_id = original_job_id;  // Use original job ID
                g_background_jobs[i].pid = job_pid;
                g_background_jobs[i].is_active = 1;
                g_background_jobs[i].state = PROCESS_STOPPED;
                strncpy(g_background_jobs[i].command, job_command, sizeof(g_background_jobs[i].command) - 1);
                g_background_jobs[i].command[sizeof(g_background_jobs[i].command) - 1] = '\0';
                
                printf("[%d] Stopped %s\n", original_job_id, job_command);
                fflush(stdout);
                break;
            }
        }
    }
    
    // Clear foreground process info
    g_foreground_pid = 0;
    g_foreground_pgid = 0;
    g_foreground_command[0] = '\0';
    
    return WIFEXITED(status) ? WEXITSTATUS(status) : 0;
}

// Execute bg command
int execute_bg(char *args) {
    background_job_t* job = NULL;
    
    if (!args || strlen(trim_whitespace(args)) == 0) {
        // No job number provided, use most recent job
        job = find_most_recent_job();
        if (!job) {
            printf("No jobs in background\n");
            return -1;
        }
    } else {
        // Parse job number
        char *args_copy = malloc(strlen(args) + 1);
        if (!args_copy) {
            perror("bg: malloc failed");
            return -1;
        }
        strcpy(args_copy, args);
        
        char *token = strtok(args_copy, " \t");
        if (!token) {
            printf("bg: invalid job number\n");
            free(args_copy);
            return -1;
        }
        
        char *endptr;
        long job_id_long = strtol(token, &endptr, 10);
        if (*endptr != '\0' || job_id_long <= 0) {
            printf("bg: invalid job number '%s'\n", token);
            free(args_copy);
            return -1;
        }
        
        int job_id = (int)job_id_long;
        job = find_job_by_id(job_id);
        if (!job) {
            printf("No such job\n");
            free(args_copy);
            return -1;
        }
        
        free(args_copy);
    }
    
    // Check if job is already running
    if (job->state == PROCESS_RUNNING) {
        printf("Job already running\n");
        return 0;
    }
    
    // Only stopped jobs can be resumed with bg
    if (job->state != PROCESS_STOPPED) {
        printf("Job is not stopped\n");
        return -1;
    }
    
    // Send SIGCONT to resume the job
    if (kill(job->pid, SIGCONT) == -1) {
        if (errno == ESRCH) {
            // Process no longer exists, remove from job list
            job->is_active = 0;
            printf("No such job\n");
        } else {
            perror("bg: failed to send SIGCONT");
        }
        return -1;
    }
    
    // Update job state to running
    job->state = PROCESS_RUNNING;
    
    // Print resume message
    printf("[%d] %s &\n", job->job_id, job->command);
    fflush(stdout);
    
    return 0;
}
