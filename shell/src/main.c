#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>
#include "shell.h"
#include "prompt.h"
#include "parser.h"
#include "commands.h"
#include "redirection.h"
/* ############## LLM Generated Code Begins ############## */
char g_shell_home[PATH_MAX];
char g_shell_prev[PATH_MAX] = {0};

background_job_t g_background_jobs[MAX_BACKGROUND_JOBS];
int g_next_job_id = 1;

char g_log_commands[MAX_LOG_COMMANDS][1024];
int g_log_count = 0;
int g_log_start = 0;

pid_t g_foreground_pid = 0;
pid_t g_foreground_pgid = 0;
char g_foreground_command[256] = {0};

volatile sig_atomic_t sigint_received = 0;
volatile sig_atomic_t sigtstp_received = 0;

int g_hop_called = 0;

int main(void)
{
    if (prompt_init() != 0)
    {
        fprintf(stderr, "Failed to initialize prompt\n");
        return 1;
    }

    log_init();
    init_background_jobs();
    setup_signal_handlers();

    for (;;)
    {
        check_background_jobs();

        char p[SHELL_PROMPT_MAX];
        if (prompt_build(p, sizeof p) == 0)
        {
            printf("%s", p);
            fflush(stdout);
        }

        char *line = NULL;
        size_t cap = 0;

        ssize_t n = getline(&line, &cap, stdin);

        if (n < 0)
        {
            if (feof(stdin))
            {
                cleanup_and_exit();
            }
            else if (errno == EINTR)
            {
                printf("\n");
                if (line)
                    free(line);
                clearerr(stdin);
                fflush(stdin); // ADD THIS LINE
                errno = 0;
                continue;
            }
            else
            {
                perror("getline");
                if (line)
                    free(line);
                break;
            }
        }

        if (n > 0 && line[n - 1] == '\n')
        {
            line[n - 1] = '\0';
            n--;
        }

        check_background_jobs();
if (strlen(line) > 0)
{
    // Trim leading and trailing whitespace
    char *trimmed = line;
    while (*trimmed == ' ' || *trimmed == '\t' || *trimmed == '\n' || *trimmed == '\r') {
        trimmed++;
    }
    
    // Find end and trim trailing whitespace
    char *end = trimmed + strlen(trimmed) - 1;
    while (end > trimmed && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }
    
    // Skip empty input after trimming
    if (strlen(trimmed) == 0) {
        free(line);
        continue;
    }
    
    // Use trimmed input for all processing
    if (parse_command(trimmed) != 0)
    {
        printf("Invalid Syntax!\n");
        log_add_command(trimmed);  // Use trimmed, not line
    }
    else
    {
        if (!log_contains_log_command(trimmed))
        {
            log_add_command(trimmed);  // Use trimmed, not line
        }

        // Check for sequential commands first (contains semicolon)
        if (strchr(trimmed, ';') != NULL)
        {
            sequential_commands_t seq_cmds;
            if (parse_sequential_commands(trimmed, &seq_cmds) == 0)
            {
                execute_sequential_commands(&seq_cmds);
                cleanup_sequential_commands(&seq_cmds);
            }
            else
            {
                printf("Invalid Syntax!\n");
            }
        }
        // ... rest of your logic using 'trimmed' instead of 'line'
        else if (strchr(trimmed, '|') != NULL || strchr(trimmed, '&') != NULL)
        {
            command_pipeline_t pipeline;
            if (parse_pipeline(trimmed, &pipeline) == 0)
            {
                execute_pipeline(&pipeline);
                cleanup_pipeline(&pipeline);
            }
            else
            {
                printf("Invalid Syntax!\n");
            }
        }
        else if (strchr(trimmed, '<') != NULL || strchr(trimmed, '>') != NULL)
        {
            parsed_command_t cmd;
            if (parse_command_with_redirection(trimmed, &cmd) == 0)
            {
                execute_command_with_redirection(&cmd);
                cleanup_parsed_command(&cmd);
            }
            else
            {
                execute_command(trimmed);
            }
        }
        else
        {
            execute_command(trimmed);
        }
    }
}
        free(line);
    }
    return 0;
}

/* ############## LLM Generated Code Ends ################ */