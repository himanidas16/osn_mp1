#ifndef REDIRECTION_H
#define REDIRECTION_H
/* ############## LLM Generated Code Begins ############## */

#include "parser.h"  // Include parser.h to get the type definitions

// Function to execute command with redirection
int execute_command_with_redirection(parsed_command_t *cmd);

// Function to execute pipeline of commands
int execute_pipeline(command_pipeline_t *pipeline);

// Function to execute sequential commands
int execute_sequential_commands(sequential_commands_t *seq_cmds);

// Function to execute command in background
int execute_command_background(parsed_command_t *cmd);

// Helper function to handle input redirection
int handle_input_redirection(const char *filename);

// Helper function to handle output redirection  
int handle_output_redirection(const char *filename, int append_mode);

#endif
/* ############## LLM Generated Code Ends ################ */