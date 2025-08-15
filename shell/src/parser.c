#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shell.h"
#include "parser.h"

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

// Parse shell_cmd: cmd_group ((& | &&) cmd_group)* &?
int parse_command(const char *input) {
    if (!input) return -1;
    
    const char *str = input;
    
    // Must start with cmd_group
    str = parse_cmd_group(str);
    if (!str) return -1;
    
    // Then any number of ((& | &&) cmd_group)
    while (1) {
        const char *next;
        str = skip_whitespace(str);
        
        if (*str == '&') {
            if (*(str + 1) == '&') {
                // &&
                str += 2;
                next = parse_cmd_group(str);
                if (!next) return -1; // && must be followed by cmd_group
                str = next;
            } else {
                // Single &
                str++;
                str = skip_whitespace(str);
                if (*str == '\0') {
                    // & at end is valid
                    return 0;
                }
                // & followed by more content, try to parse as cmd_group
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