#ifndef BOOK_STORAGE_H
#define BOOK_STORAGE_H

typedef struct {
    char *book_dir;
    char *book_file;
} BookStorageConfig;

void bookStorageInit(const char *book_dir, const char *book_file);
void bookStorageCleanup(void);

int bookStorageSave(const char *content);
int bookStorageAppend(const char *label, const char *content);
char* bookStorageRead(void);
int bookStorageAppendWithTimestamp(const char *label, const char *content);
long bookStorageGetSize(void);

#endif
