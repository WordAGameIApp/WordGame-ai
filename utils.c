#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/types.h>
#endif

char* utils_read_file(const char *filename) {
    if (!filename) return NULL;

    FILE *file = fopen(filename, "rb");
    if (!file) return NULL;

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (length < 0) {
        fclose(file);
        return NULL;
    }

    char *content = (char *)malloc((size_t)length + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }

    size_t read_len = fread(content, 1, (size_t)length, file);
    content[read_len] = '\0';
    fclose(file);

    return content;
}

int utils_write_file(const char *filename, const char *content) {
    if (!filename || !content) return 0;

    FILE *file = fopen(filename, "wb");
    if (!file) return 0;

    size_t len = strlen(content);
    size_t written = fwrite(content, 1, len, file);
    fclose(file);

    return written == len;
}

char* utils_read_prompt_file(const char *filename) {
    return utils_read_file(filename);
}

int utils_ensure_dir(const char *path) {
    if (!path) return 0;
#ifdef _WIN32
    if (_mkdir(path) != 0 && errno != EEXIST) return 0;
#else
    if (mkdir(path, 0755) != 0 && errno != EEXIST) return 0;
#endif
    return 1;
}

void utils_mask_api_key(const char *key, char *buf, size_t bufSize) {
    if (!buf || bufSize == 0) return;
    buf[0] = '\0';
    if (!key) return;

    size_t len = strlen(key);
    if (len <= 8) {
        snprintf(buf, bufSize, "***");
        return;
    }
    snprintf(buf, bufSize, "%.4s...%.4s", key, key + len - 4);
}

size_t utils_strlcpy(char *dst, const char *src, size_t size) {
    if (!dst || !src || size == 0) return 0;
    size_t srclen = strlen(src);
    if (srclen < size) {
        memcpy(dst, src, srclen + 1);
    } else {
        memcpy(dst, src, size - 1);
        dst[size - 1] = '\0';
    }
    return srclen;
}

char* utils_strdup_concat(const char *a, const char *b) {
    if (!a && !b) return NULL;
    if (!a) return strdup(b);
    if (!b) return strdup(a);
    size_t la = strlen(a);
    size_t lb = strlen(b);
    char *result = (char *)malloc(la + lb + 1);
    if (!result) return NULL;
    memcpy(result, a, la);
    memcpy(result + la, b, lb + 1);
    return result;
}

int utils_file_exists(const char *filename) {
    if (!filename) return 0;
    FILE *f = fopen(filename, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

unsigned long utils_string_hash(const char *str) {
    if (!str) return 0;
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

int utils_safe_atoi(const char *str, int default_value) {
    if (!str || strlen(str) == 0) return default_value;
    char *end;
    long val = strtol(str, &end, 10);
    if (end == str || *end != '\0') return default_value;
    if (val < INT_MIN || val > INT_MAX) return default_value;
    return (int)val;
}
