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
// Add this line to commands.h
// void handle_pending_signals(void);


// fg and bg command functions
int execute_fg(char *args);
int execute_bg(char *args);

// Helper functions for job management
background_job_t* find_job_by_id(int job_id);
background_job_t* find_most_recent_job(void);


#endif