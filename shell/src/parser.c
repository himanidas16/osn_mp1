#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shell.h"
#include "parser.h"
#include <sys/wait.h>
#include <signal.h>
// Skip whitespace characters
static const char *skip_whitespace(const char *str) {
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') {
        str++;
    }
    return str;
}

// Check if character is valid for a name token
static int is_name_char(char c) {
    return c != '\0' && c != '|' && c != '&' && c != '>' && c != '<' && 
           c != ';' && c != ' ' && c != '\t' && c != '\n' && c != '\r';
}

// Parse a name token
static const char *parse_name(const char *str) {
    str = skip_whitespace(str);
    if (!is_name_char(*str)) return NULL;
    
    while (is_name_char(*str)) {
        str++;
    }
    return str;
}

// Parse input redirection (< name)
static const char *parse_input(const char *str) {
    str = skip_whitespace(str);
    if (*str != '<') return NULL;
    
    str++; // consume '<'
    str = skip_whitespace(str); // Handle space after <
    return parse_name(str);
}

// Parse output redirection (> name or >> name)
static const char *parse_output(const char *str) {
    str = skip_whitespace(str);
    if (*str != '>') return NULL;
    
    str++; // consume first '>'
    if (*str == '>') {
        str++; // consume second '>' for >>
    }
    str = skip_whitespace(str); // Handle optional whitespace after > or >>
    return parse_name(str);
}

// Parse atomic: name (name | input | output)*
static const char *parse_atomic(const char *str) {
    // Must start with a name
    str = parse_name(str);
    if (!str) return NULL;
    
    // Then any number of (name | input | output)
    while (1) {
        const char *next;
        str = skip_whitespace(str);
        
        // Try name
        if ((next = parse_name(str))) {
            str = next;
            continue;
        }
        
        // Try input
        if ((next = parse_input(str))) {
            str = next;
            continue;
        }
        
        // Try output
        if ((next = parse_output(str))) {
            str = next;
            continue;
        }
        
        // No more matches
        break;
    }
    
    return str;
}

// Parse cmd_group: atomic (| atomic)*
static const char *parse_cmd_group(const char *str) {
    // Must start with atomic
    str = parse_atomic(str);
    if (!str) return NULL;
    
    // Then any number of (| atomic)
    while (1) {
        const char *next;
        str = skip_whitespace(str);
        
        if (*str == '|' && *(str + 1) != '|') { // Single | not ||
            str++; // consume |
            next = parse_atomic(str);
            if (!next) return NULL; // | must be followed by atomic
            str = next;
        } else {
            break;
        }
    }
    
    return str;
}

// Replace your parse_command function in src/parser.c with this:

// Parse shell_cmd: cmd_group ((; | & | &&) cmd_group)* &?
// Update parse_command to handle leading semicolons properly

int parse_command(const char *input) {
    if (!input) return -1;
    
    const char *str = input;
    str = skip_whitespace(str);
    
    // Handle empty input
    if (*str == '\0') return -1;
    
    // Check for leading semicolon (invalid according to grammar)
    if (*str == ';') {
        return -1; // Invalid: cannot start with semicolon
    }
    
    // Must start with cmd_group
    str = parse_cmd_group(str);
    if (!str) return -1;
    
    // Then any number of ((; | & | &&) cmd_group)
    while (1) {
        const char *next;
        str = skip_whitespace(str);
        
        if (*str == ';') {
            // Semicolon - sequential execution
            str++; // consume ';'
            str = skip_whitespace(str);
            
            // Allow trailing semicolon (command; at end is valid)
            if (*str == '\0') {
                return 0; // Valid: command ending with semicolon
            }
            
            // Handle consecutive semicolons (treat as empty commands)
            if (*str == ';') {
                continue; // Skip empty commands
            }
            
            next = parse_cmd_group(str);
            if (!next) return -1;
            str = next;
        } else if (*str == '&') {
            if (*(str + 1) == '&') {
                // &&
                str += 2;
                str = skip_whitespace(str);
                if (*str == '\0') {
                    return -1; // Invalid: && at end without following command
                }
                next = parse_cmd_group(str);
                if (!next) return -1;
                str = next;
            } else {
                // Single &
                str++;
                str = skip_whitespace(str);
                if (*str == '\0') {
                    return 0; // Valid: & at end (background)
                }
                if (*str == ';') {
                    continue; // Handle & followed by ;
                }
                next = parse_cmd_group(str);
                if (!next) return -1;
                str = next;
            }
        } else {
            break;
        }
    }
    
    // Check if we consumed all input
    str = skip_whitespace(str);
    return (*str == '\0') ? 0 : -1;
}

// Helper function to extract a name token and advance pointer
static char *extract_name_token(const char **str) {
    *str = skip_whitespace(*str);
    const char *start = *str;
    
    if (!is_name_char(**str)) return NULL;
    
    while (is_name_char(**str)) {
        (*str)++;
    }
    
    int len = *str - start;
    char *name = malloc(len + 1);
    if (name) {
        strncpy(name, start, len);
        name[len] = '\0';
    }
    return name;
}

// Parse command with redirection information
int parse_command_with_redirection(const char *input, parsed_command_t *cmd) {
    if (!input || !cmd) return -1;
    
    // Initialize the command structure
    memset(cmd, 0, sizeof(parsed_command_t));
    
    const char *str = input;
    str = skip_whitespace(str);
    
    // First token must be the command name
    cmd->command = extract_name_token(&str);
    if (!cmd->command) return -1;
    
    // Count maximum possible arguments first
    const char *temp = str;
    int max_args = 0;
    
    while (1) {
        temp = skip_whitespace(temp);
        if (*temp == '\0' || *temp == '|' || *temp == '&' || *temp == ';') break;
        
        if (*temp == '<') {
            temp++;
            temp = skip_whitespace(temp);
            char *dummy = extract_name_token(&temp);
            if (dummy) free(dummy);
            else return -1;
        } else if (*temp == '>') {
            temp++;
            if (*temp == '>') temp++;
            temp = skip_whitespace(temp);
            char *dummy = extract_name_token(&temp);
            if (dummy) free(dummy);
            else return -1;
        } else {
            char *arg = extract_name_token(&temp);
            if (arg) {
                max_args++;
                free(arg);
            } else {
                break;
            }
        }
    }
    
    // Allocate argument array
    if (max_args > 0) {
        cmd->args = malloc(max_args * sizeof(char*));
        if (!cmd->args) {
            free(cmd->command);
            return -1;
        }
    }
    
    // Parse arguments and redirections
    while (1) {
        str = skip_whitespace(str);
        if (*str == '\0' || *str == '|' || *str == '&' || *str == ';') break;
        
        if (*str == '<') {
            str++;
            str = skip_whitespace(str); // Handle space after <
            char *filename = extract_name_token(&str);
            if (!filename) {
                cleanup_parsed_command(cmd);
                return -1;
            }
            // If multiple input redirections, use only the last one
            free(cmd->input_file);
            cmd->input_file = filename;
        } else if (*str == '>') {
            str++;
            int append = 0;
            if (*str == '>') {
                str++;
                append = 1;
            }
            str = skip_whitespace(str); // Handle space after > or >>
            char *filename = extract_name_token(&str);
            if (!filename) {
                cleanup_parsed_command(cmd);
                return -1;
            }
            // If multiple output redirections, use only the last one
            free(cmd->output_file);
            cmd->output_file = filename;
            cmd->append_mode = append;
        } else {
            char *arg = extract_name_token(&str);
            if (arg) {
                cmd->args[cmd->arg_count++] = arg;
            } else {
                break;
            }
        }
    }
    
    return 0;
}

// Parse pipeline from input
// Replace your parse_pipeline function in src/parser.c

int parse_pipeline(const char *input, command_pipeline_t *pipeline) {
    if (!input || !pipeline) return -1;
    
    memset(pipeline, 0, sizeof(command_pipeline_t));
    
    // Check if command ends with & (background execution)
    const char *temp = input + strlen(input) - 1;
    while (temp >= input && (*temp == ' ' || *temp == '\t' || *temp == '\n' || *temp == '\r')) {
        temp--;
    }
    
    if (temp >= input && *temp == '&') {
        pipeline->is_background = 1;
        
        // Create a copy of input without the trailing &
        int len = temp - input;
        char *input_copy = malloc(len + 1);
        if (!input_copy) return -1;
        strncpy(input_copy, input, len);
        input_copy[len] = '\0';
        
        // Parse the command without the &
        int result = parse_pipeline(input_copy, pipeline);
        pipeline->is_background = 1; // Ensure this is set
        free(input_copy);
        return result;
    }
    
    // Count pipes to determine number of commands
    temp = input;
    int pipe_count = 0;
    
    while (*temp) {
        if (*temp == '|' && *(temp + 1) != '|') {
            pipe_count++;
        }
        temp++;
    }
    
    int cmd_count = pipe_count + 1;
    
    // Allocate command array
    pipeline->commands = malloc(cmd_count * sizeof(parsed_command_t));
    if (!pipeline->commands) return -1;
    
    pipeline->cmd_count = cmd_count;
    
    // Parse each command in the pipeline
    const char *str = input;
    const char *cmd_start = str;
    int cmd_index = 0;
    
    while (*str && cmd_index < cmd_count) {
        // Find the end of current command (next pipe or end of string)
        const char *cmd_end = str;
        while (*cmd_end && !(*cmd_end == '|' && *(cmd_end + 1) != '|')) {
            cmd_end++;
        }
        
        // Extract the current command string
        int cmd_len = cmd_end - cmd_start;
        char *cmd_str = malloc(cmd_len + 1);
        if (!cmd_str) {
            cleanup_pipeline(pipeline);
            return -1;
        }
        strncpy(cmd_str, cmd_start, cmd_len);
        cmd_str[cmd_len] = '\0';
        
        // Parse this command
        if (parse_command_with_redirection(cmd_str, &pipeline->commands[cmd_index]) != 0) {
            free(cmd_str);
            cleanup_pipeline(pipeline);
            return -1;
        }
        
        free(cmd_str);
        cmd_index++;
        
        // Move to next command
        if (*cmd_end == '|') {
            str = cmd_end + 1;
            cmd_start = str;
        } else {
            break;
        }
    }
    
    return 0;
}
// Cleanup function for parsed command
void cleanup_parsed_command(parsed_command_t *cmd) {
    if (!cmd) return;
    
    free(cmd->command);
    for (int i = 0; i < cmd->arg_count; i++) {
        free(cmd->args[i]);
    }
    free(cmd->args);
    free(cmd->input_file);
    free(cmd->output_file);
    
    memset(cmd, 0, sizeof(parsed_command_t));
}

// Cleanup function for pipeline
void cleanup_pipeline(command_pipeline_t *pipeline) {
    if (!pipeline) return;
    
    for (int i = 0; i < pipeline->cmd_count; i++) {
        cleanup_parsed_command(&pipeline->commands[i]);
    }
    free(pipeline->commands);
    
    memset(pipeline, 0, sizeof(command_pipeline_t));
}



//part d

// Add these functions to the end of src/parser.c

// Parse sequential commands separated by semicolons
// Replace your parse_sequential_commands function in src/parser.c

int parse_sequential_commands(const char *input, sequential_commands_t *seq_cmds) {
    if (!input || !seq_cmds) return -1;
    
    memset(seq_cmds, 0, sizeof(sequential_commands_t));
    
    // Count semicolons to determine number of command groups
    const char *temp = input;
    int semicolon_count = 0;
    
    while (*temp) {
        if (*temp == ';') {
            semicolon_count++;
        }
        temp++;
    }
    
    int max_pipeline_count = semicolon_count + 1;
    
    // Allocate pipeline array (may be larger than needed due to empty commands)
    seq_cmds->pipelines = malloc(max_pipeline_count * sizeof(command_pipeline_t));
    if (!seq_cmds->pipelines) return -1;
    
    // Parse each command group separated by semicolons
    const char *str = input;
    const char *cmd_start = str;
    int pipeline_index = 0;
    
    while (*str) {
        // Find the end of current command group (next semicolon or end of string)
        const char *cmd_end = str;
        while (*cmd_end && *cmd_end != ';') {
            cmd_end++;
        }
        
        // Extract the current command group string
        int cmd_len = cmd_end - cmd_start;
        char *cmd_str = malloc(cmd_len + 1);
        if (!cmd_str) {
            // Cleanup what we've allocated so far
            seq_cmds->pipeline_count = pipeline_index;
            cleanup_sequential_commands(seq_cmds);
            return -1;
        }
        strncpy(cmd_str, cmd_start, cmd_len);
        cmd_str[cmd_len] = '\0';
        
        // Trim whitespace from command string
        char *trimmed_cmd = cmd_str;
        while (*trimmed_cmd == ' ' || *trimmed_cmd == '\t' || *trimmed_cmd == '\n' || *trimmed_cmd == '\r') {
            trimmed_cmd++;
        }
        char *end = trimmed_cmd + strlen(trimmed_cmd) - 1;
        while (end > trimmed_cmd && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
            *end = '\0';
            end--;
        }
        
        // Handle empty commands - just skip them
        if (strlen(trimmed_cmd) == 0) {
            free(cmd_str);
            // Move to next command group
            if (*cmd_end == ';') {
                str = cmd_end + 1;
                cmd_start = str;
            } else {
                break;
            }
            continue;
        }
        
        // Parse this command group as a pipeline
        if (parse_pipeline(trimmed_cmd, &seq_cmds->pipelines[pipeline_index]) != 0) {
            free(cmd_str);
            seq_cmds->pipeline_count = pipeline_index;
            cleanup_sequential_commands(seq_cmds);
            return -1;
        }
        
        free(cmd_str);
        pipeline_index++;
        
        // Move to next command group
        if (*cmd_end == ';') {
            str = cmd_end + 1;
            cmd_start = str;
        } else {
            break;
        }
    }
    
    // Update the actual count
    seq_cmds->pipeline_count = pipeline_index;
    
    return 0;
}

// Cleanup function for sequential commands
void cleanup_sequential_commands(sequential_commands_t *seq_cmds) {
    if (!seq_cmds) return;
    
    for (int i = 0; i < seq_cmds->pipeline_count; i++) {
        cleanup_pipeline(&seq_cmds->pipelines[i]);
    }
    free(seq_cmds->pipelines);
    
    memset(seq_cmds, 0, sizeof(sequential_commands_t));
}


//part d2 background 

