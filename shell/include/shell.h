#ifndef SHELL_H
#define SHELL_H

#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

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

// Background job management
#define MAX_BACKGROUND_JOBS 100

typedef enum {
    PROCESS_RUNNING,
    PROCESS_STOPPED,
    PROCESS_TERMINATED
} process_state_t;

typedef struct background_job {
    int job_id;
    pid_t pid;
    char command[256];
    int is_active;
    process_state_t state;
} background_job_t;

// Global background job storage
extern background_job_t g_background_jobs[MAX_BACKGROUND_JOBS];
extern int g_next_job_id;

// Signal handling globals
extern pid_t g_foreground_pid;
extern pid_t g_foreground_pgid;
extern char g_foreground_command[256];

// Log functions
int log_init(void);
void log_add_command(const char *command);
int log_contains_log_command(const char *command);

// Background job functions
void init_background_jobs(void);
int add_background_job(pid_t pid, const char *command);
void check_background_jobs(void);
void cleanup_background_job(int index);

// Activities command
int execute_activities(void);

// Ping command
int execute_ping(char *args);

// Signal handling functions
void setup_signal_handlers(void);
void sigint_handler(int sig);
void sigtstp_handler(int sig);
void cleanup_and_exit(void);
///part ctrol

// Add these lines to the existing declarations in shell.h
extern volatile sig_atomic_t sigint_received;
extern volatile sig_atomic_t sigtstp_received;

// Add this function declaration
// void handle_pending_signals(void);

int add_background_job_running(pid_t pid, const char *command);
int add_background_job_stopped(pid_t pid, const char *command);


#endif