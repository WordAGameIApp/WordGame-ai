#include "book_storage.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static BookStorageConfig g_storage_config = { NULL, NULL };

void bookStorageInit(const char *book_dir, const char *book_file) {
    free(g_storage_config.book_dir);
    free(g_storage_config.book_file);
    g_storage_config.book_dir = NULL;
    g_storage_config.book_file = NULL;

    if (book_dir) {
        g_storage_config.book_dir = strdup(book_dir);
    } else {
        g_storage_config.book_dir = strdup("book");
    }

    if (!g_storage_config.book_dir) return;

    if (book_file) {
        g_storage_config.book_file = strdup(book_file);
    } else {
        size_t len = strlen(g_storage_config.book_dir) + 32;
        g_storage_config.book_file = (char *)malloc(len);
        if (g_storage_config.book_file) {
            snprintf(g_storage_config.book_file, len, "%s/worldbook.book", g_storage_config.book_dir);
        }
    }
}

void bookStorageCleanup(void) {
    free(g_storage_config.book_dir);
    free(g_storage_config.book_file);
    g_storage_config.book_dir = NULL;
    g_storage_config.book_file = NULL;
}

static int ensureBookDir(void) {
    if (!g_storage_config.book_dir) {
        bookStorageInit(NULL, NULL);
    }
    return utils_ensure_dir(g_storage_config.book_dir);
}

int bookStorageSave(const char *content) {
    if (!content) return 0;
    if (!ensureBookDir()) return 0;

    if (!g_storage_config.book_file) return 0;
    return utils_write_file(g_storage_config.book_file, content);
}

int bookStorageAppend(const char *label, const char *content) {
    if (!content) return 0;
    if (!ensureBookDir()) return 0;

    if (!g_storage_config.book_file) return 0;
    FILE *fp = fopen(g_storage_config.book_file, "a");
    if (!fp) return 0;

    if (label) {
        fprintf(fp, "=== %s ===\n", label);
    }
    fprintf(fp, "%s\n\n", content);
    fclose(fp);
    return 1;
}

long bookStorageGetSize(void) {
    if (!g_storage_config.book_file) return -1;
    FILE *f = fopen(g_storage_config.book_file, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fclose(f);
    return size;
}

char* bookStorageRead(void) {
    if (!g_storage_config.book_file) return NULL;
    return utils_read_file(g_storage_config.book_file);
}

int bookStorageAppendWithTimestamp(const char *label, const char *content) {
    if (!content) return 0;
    if (!ensureBookDir()) return 0;

    if (!g_storage_config.book_file) return 0;
    FILE *fp = fopen(g_storage_config.book_file, "a");
    if (!fp) return 0;

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    if (label) {
        fprintf(fp, "=== %s [%s] ===\n", label, time_buf);
    } else {
        fprintf(fp, "=== [%s] ===\n", time_buf);
    }
    fprintf(fp, "%s\n\n", content);
    fclose(fp);
    return 1;
}
