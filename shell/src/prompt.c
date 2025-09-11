#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <limits.h>
#include "shell.h"
#include "prompt.h"
#include <sys/stat.h>
/* ############## LLM Generated Code Begins ############## */

static char s_user[LOGIN_NAME_MAX] = "user";
static char s_host[256] = "host";

static void cache_identity(void) {
    struct passwd *pw = getpwuid(geteuid());
    if (pw && pw->pw_name && pw->pw_name[0]) {
        strncpy(s_user, pw->pw_name, sizeof s_user - 1);
        s_user[sizeof s_user - 1] = '\0';
    } else {
        // Q4: Don't use bash environment variables
        strncpy(s_user, "user", sizeof(s_user) - 1);
        s_user[sizeof(s_user) - 1] = '\0';
    }
    
    if (gethostname(s_host, sizeof s_host) != 0 || !s_host[0]) {
        strncpy(s_host, "host", sizeof s_host - 1);
        s_host[sizeof s_host - 1] = '\0';
    }
}

int prompt_init(void) {
    cache_identity();
    if (!getcwd(g_shell_home, sizeof g_shell_home)) {
        strncpy(g_shell_home, "/", sizeof g_shell_home - 1);
        g_shell_home[sizeof g_shell_home - 1] = '\0';
        return -1;
    }
    return 0;
}

static void tilde_path(const char *cwd, const char *home, char *out, size_t outlen) {
    if (!cwd || !home || !out || outlen == 0) {
        if (out && outlen > 0) out[0] = '\0';
        return;
    }
    
    size_t hl = strlen(home);
    size_t cl = strlen(cwd);
    
    // Check if current directory is exactly the home directory
    if (strcmp(cwd, home) == 0) {
        snprintf(out, outlen, "~");
        return;
    }
    
    // Check if current directory is under home directory (subdirectory only)
    if (cl > hl && strncmp(cwd, home, hl) == 0 && cwd[hl] == '/') {
        snprintf(out, outlen, "~%s", cwd + hl);
        return;
    }
    
    // REMOVE the ancestor check completely - don't show ~ for parent directories
    // Just show the full path for everything else
    snprintf(out, outlen, "%s", cwd);
}

int prompt_build(char *buf, size_t buflen) {
    if (!buf || buflen < 8) return -1;
    
    char cwd[PATH_MAX] = {0};
    char shown[PATH_MAX] = {0};
    
    if (!getcwd(cwd, sizeof cwd)) {
        strncpy(cwd, "?", sizeof cwd - 1);
        cwd[sizeof cwd - 1] = '\0';
    }
    
    tilde_path(cwd, g_shell_home, shown, sizeof shown);
    int n = snprintf(buf, buflen, "<%s@%s:%s> ", s_user, s_host, shown);
    return (n < 0 || (size_t)n >= buflen) ? -1 : 0;
}
/* ############## LLM Generated Code Ends ################ */