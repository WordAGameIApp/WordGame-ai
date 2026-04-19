#include "platform.h"
#include <locale.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#endif

void platform_setup_utf8(void) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    setlocale(LC_ALL, ".UTF8");
#endif
}

const char* platform_get_version(void) {
    return WORDGAME_VERSION;
}

#ifdef _WIN32
char** platform_get_utf8_argv(int *argCount) {
    if (!argCount) return NULL;

    wchar_t **wargv = CommandLineToArgvW(GetCommandLineW(), argCount);
    if (!wargv) return NULL;

    char **argv = (char **)malloc((size_t)(*argCount + 1) * sizeof(char*));
    if (!argv) {
        LocalFree(wargv);
        return NULL;
    }

    for (int i = 0; i < *argCount; i++) {
        int reqLen = WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, NULL, 0, NULL, NULL);
        if (reqLen <= 0) {
            for (int j = 0; j < i; j++) free(argv[j]);
            free(argv);
            LocalFree(wargv);
            return NULL;
        }
        argv[i] = (char *)malloc((size_t)reqLen);
        if (!argv[i]) {
            for (int j = 0; j < i; j++) free(argv[j]);
            free(argv);
            LocalFree(wargv);
            return NULL;
        }
        WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, argv[i], reqLen, NULL, NULL);
    }
    argv[*argCount] = NULL;
    LocalFree(wargv);
    return argv;
}

void platform_free_utf8_argv(int argCount, char **argv) {
    if (!argv) return;
    for (int i = 0; i < argCount; i++) {
        free(argv[i]);
    }
    free(argv);
}
#endif
