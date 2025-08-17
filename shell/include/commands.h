#ifndef COMMANDS_H
#define COMMANDS_H

#include "shell.h"

// Command execution function
int execute_command(const char *input);
int execute_reveal(char *args);
int execute_log(char *args);

// Direct execution functions for redirection
int execute_hop_direct(char *args);

// Activities command
int execute_activities(void);

// Ping command
int execute_ping(char *args);

#endif