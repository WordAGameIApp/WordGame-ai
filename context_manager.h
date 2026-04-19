#ifndef CONTEXT_MANAGER_H
#define CONTEXT_MANAGER_H

#include "context.h"
#include "config.h"

typedef struct {
    char *name;
    Context ctx;
    char *filename;
    int message_count;
    int auto_compress;
    int compress_threshold;
} NamedContext;

typedef struct {
    NamedContext *contexts;
    int count;
    int capacity;
    char *default_context;
} ContextManager;

void context_manager_init(ContextManager *mgr, int capacity);
void context_manager_free(ContextManager *mgr);
int context_manager_create(ContextManager *mgr, const char *name,
                           const char *filename, int message_limit,
                           int auto_compress, int compress_threshold);
NamedContext* context_manager_get(const ContextManager *mgr, const char *name);
int context_manager_exists(const ContextManager *mgr, const char *name);
int context_manager_remove(ContextManager *mgr, const char *name);
int context_manager_rename(ContextManager *mgr, const char *old_name, const char *new_name);
int context_manager_switch(ContextManager *mgr, const char *name);
NamedContext* context_manager_get_current(const ContextManager *mgr);
int context_manager_load_all(ContextManager *mgr);
int context_manager_save_all(const ContextManager *mgr);
int context_manager_save_state(const ContextManager *mgr, const char *stateFile);
int context_manager_load_state(ContextManager *mgr, const char *stateFile);
int context_compress_manual(NamedContext *nctx, const char *summary);
int context_auto_compress_if_needed(NamedContext *nctx);
void context_get_stats(const NamedContext *nctx, int *total_messages, int *total_chars);
void context_manager_list(const ContextManager *mgr);
int context_manager_count(const ContextManager *mgr);
int context_manager_export_context(const ContextManager *mgr, const char *name, const char *output_file);

#endif
