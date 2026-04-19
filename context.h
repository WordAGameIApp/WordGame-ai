#ifndef CONTEXT_H
#define CONTEXT_H

typedef struct {
    char *role;
    char *content;
    char *tool_call_id;
} Message;

typedef struct {
    Message *messages;
    int count;
    int capacity;
} Context;

void context_init(Context *ctx, int capacity);
void context_free(Context *ctx);
int context_copy(const Context *src, Context *dst);
void context_add_message(Context *ctx, const char *role, const char *content, const char *tool_call_id);
int context_load(Context *ctx, const char *filename);
int context_save(const Context *ctx, const char *filename);
void context_clear(Context *ctx);

int context_count(const Context *ctx);
const Message* context_get_message(const Context *ctx, int index);
int context_total_chars(const Context *ctx);
int context_find_by_role(const Context *ctx, const char *role, int start_index);
int context_remove_last(Context *ctx);
int context_trim_to_size(Context *ctx, int max_messages);
char* context_to_json(const Context *ctx);
int context_replace_message(Context *ctx, int index, const char *role, const char *content);

#endif
