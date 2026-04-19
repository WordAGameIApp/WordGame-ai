#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

char* utils_read_file(const char *filename);
int utils_write_file(const char *filename, const char *content);
char* utils_read_prompt_file(const char *filename);
int utils_ensure_dir(const char *path);
void utils_mask_api_key(const char *key, char *buf, size_t bufSize);
size_t utils_strlcpy(char *dst, const char *src, size_t size);
char* utils_strdup_concat(const char *a, const char *b);
int utils_file_exists(const char *filename);
unsigned long utils_string_hash(const char *str);
int utils_safe_atoi(const char *str, int default_value);

#endif
