#ifndef PROMPT_H
#define PROMPT_H
#include "shell.h"
/* ############## LLM Generated Code Begins ############## */
int prompt_init(void); // set shell "home" + cache ids
int prompt_build(char *buf, size_t buflen); // builds "<user@host:path> "
/* ############## LLM Generated Code Ends ################ */
#endif