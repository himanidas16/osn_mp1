#ifndef REDIRECTION_H
#define REDIRECTION_H

#include "parser.h"  // Include parser.h to get the type definitions

// Function to execute command with redirection
int execute_command_with_redirection(parsed_command_t *cmd);

// Function to execute pipeline of commands
int execute_pipeline(command_pipeline_t *pipeline);

// Helper function to handle input redirection
int handle_input_redirection(const char *filename);

// Helper function to handle output redirection  
int handle_output_redirection(const char *filename, int append_mode);

#endif