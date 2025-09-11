#ifndef PARSER_H
#define PARSER_H
/* ############## LLM Generated Code Begins ############## */

// Structure to hold parsed command information
typedef struct {
    char *command;           // The main command
    char **args;            // Command arguments  
    int arg_count;          // Number of arguments
    char *input_file;       // Input redirection file (< file)
    char *output_file;      // Output redirection file (> file or >> file)
    int append_mode;        // 1 if >>, 0 if >
} parsed_command_t;

// Structure for pipe handling
typedef struct {
    parsed_command_t *commands;  // Array of commands in pipeline
    int cmd_count;              // Number of commands in pipeline
    int is_background;          // 1 if pipeline should run in background
} command_pipeline_t;

// Structure for sequential command execution
typedef struct {
    command_pipeline_t *pipelines;  // Array of command pipelines
    int pipeline_count;             // Number of pipelines to execute sequentially
} sequential_commands_t;

// Parser function declarations
int parse_command(const char *input);
int parse_command_with_redirection(const char *input, parsed_command_t *cmd);
int parse_pipeline(const char *input, command_pipeline_t *pipeline);
int parse_sequential_commands(const char *input, sequential_commands_t *seq_cmds);
void cleanup_parsed_command(parsed_command_t *cmd);
void cleanup_pipeline(command_pipeline_t *pipeline);
void cleanup_sequential_commands(sequential_commands_t *seq_cmds);
/* ############## LLM Generated Code Ends ################ */
#endif