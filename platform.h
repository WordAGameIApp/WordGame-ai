#ifndef PLATFORM_H
#define PLATFORM_H

#define WORDGAME_VERSION "1.0.0"

void platform_setup_utf8(void);
const char* platform_get_version(void);

#ifdef _WIN32
char** platform_get_utf8_argv(int *argCount);
void platform_free_utf8_argv(int argCount, char **argv);
#endif

#endif
