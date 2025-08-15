#ifndef SHELL_H
#define SHELL_H

#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Ensure PATH_MAX is defined */
#ifndef PATH_MAX
#ifdef _POSIX_PATH_MAX
#define PATH_MAX _POSIX_PATH_MAX
#else
#define PATH_MAX 4096
#endif
#endif

/* Ensure LOGIN_NAME_MAX is defined */
#ifndef LOGIN_NAME_MAX
#ifdef _POSIX_LOGIN_NAME_MAX
#define LOGIN_NAME_MAX _POSIX_LOGIN_NAME_MAX
#else
#define LOGIN_NAME_MAX 256
#endif
#endif

#define SHELL_PROMPT_MAX 1024
extern char g_shell_home[PATH_MAX];


// Global variable for previous directory
extern char g_shell_prev[PATH_MAX];


// Log functionality
#define MAX_LOG_COMMANDS 15
#define LOG_FILENAME ".shell_history"

// Global log storage
extern char g_log_commands[MAX_LOG_COMMANDS][1024];
extern int g_log_count;
extern int g_log_start;

// Log functions
int log_init(void);
void log_add_command(const char *command);
int log_contains_log_command(const char *command);


#endif