#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include "shell.h"
#include "commands.h"





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
