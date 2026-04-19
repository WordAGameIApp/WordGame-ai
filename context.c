#include "context.h"
#include "utils.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void context_init(Context *ctx, int capacity) {
    if (!ctx || capacity <= 0) return;
    ctx->messages = (Message *)malloc(sizeof(Message) * (size_t)capacity);
    if (!ctx->messages) {
        ctx->count = 0;
        ctx->capacity = 0;
        return;
    }
    ctx->count = 0;
    ctx->capacity = capacity;
}

void context_free(Context *ctx) {
    if (!ctx) return;
    for (int i = 0; i < ctx->count; i++) {
        free(ctx->messages[i].role);
        free(ctx->messages[i].content);
        free(ctx->messages[i].tool_call_id);
    }
    free(ctx->messages);
    ctx->messages = NULL;
    ctx->count = 0;
    ctx->capacity = 0;
}

int context_copy(const Context *src, Context *dst) {
    if (!src || !dst) return 0;
    context_free(dst);
    if (src->count <= 0) return 1;

    context_init(dst, src->count > 0 ? src->count : 10);
    if (!dst->messages) return 0;

    for (int i = 0; i < src->count; i++) {
        context_add_message(dst, src->messages[i].role, src->messages[i].content,
                           src->messages[i].tool_call_id);
    }
    return 1;
}

void context_clear(Context *ctx) {
    if (!ctx) return;
    for (int i = 0; i < ctx->count; i++) {
        free(ctx->messages[i].role);
        free(ctx->messages[i].content);
        free(ctx->messages[i].tool_call_id);
    }
    ctx->count = 0;
}

static int is_valid_role(const char *role) {
    if (!role) return 0;
    return strcmp(role, "user") == 0 ||
           strcmp(role, "assistant") == 0 ||
           strcmp(role, "system") == 0 ||
           strcmp(role, "tool") == 0;
}

void context_add_message(Context *ctx, const char *role, const char *content, const char *tool_call_id) {
    if (!ctx || !role || !content) return;
    if (!is_valid_role(role)) return;

    if (ctx->count >= ctx->capacity) {
        int new_capacity = ctx->capacity * 2;
        if (new_capacity <= 0) new_capacity = 10;
        Message *new_messages = (Message *)realloc(ctx->messages, sizeof(Message) * (size_t)new_capacity);
        if (!new_messages) return;
        ctx->messages = new_messages;
        ctx->capacity = new_capacity;
    }

    char *role_copy = strdup(role);
    char *content_copy = strdup(content);
    char *tool_id_copy = tool_call_id ? strdup(tool_call_id) : NULL;

    if (!role_copy || !content_copy) {
        free(role_copy);
        free(content_copy);
        free(tool_id_copy);
        return;
    }

    ctx->messages[ctx->count].role = role_copy;
    ctx->messages[ctx->count].content = content_copy;
    ctx->messages[ctx->count].tool_call_id = tool_id_copy;
    ctx->count++;
}

int context_load(Context *ctx, const char *filename) {
    if (!ctx || !filename) return 0;

    char *json_str = utils_read_file(filename);
    if (!json_str) return 0;

    cJSON *root = cJSON_Parse(json_str);
    free(json_str);

    if (!root || !cJSON_IsArray(root)) {
        if (root) cJSON_Delete(root);
        return 0;
    }

    context_clear(ctx);

    int size = cJSON_GetArraySize(root);
    for (int i = 0; i < size; i++) {
        cJSON *msg = cJSON_GetArrayItem(root, i);
        cJSON *role = cJSON_GetObjectItemCaseSensitive(msg, "role");
        cJSON *content = cJSON_GetObjectItemCaseSensitive(msg, "content");
        cJSON *tool_call_id = cJSON_GetObjectItemCaseSensitive(msg, "tool_call_id");

        if (cJSON_IsString(role) && cJSON_IsString(content)) {
            context_add_message(ctx, role->valuestring, content->valuestring,
                               tool_call_id ? tool_call_id->valuestring : NULL);
        }
    }

    cJSON_Delete(root);
    return 1;
}

int context_count(const Context *ctx) {
    if (!ctx) return 0;
    return ctx->count;
}

const Message* context_get_message(const Context *ctx, int index) {
    if (!ctx || index < 0 || index >= ctx->count) return NULL;
    return &ctx->messages[index];
}

int context_total_chars(const Context *ctx) {
    if (!ctx) return 0;
    int total = 0;
    for (int i = 0; i < ctx->count; i++) {
        if (ctx->messages[i].content) {
            total += (int)strlen(ctx->messages[i].content);
        }
    }
    return total;
}

int context_find_by_role(const Context *ctx, const char *role, int start_index) {
    if (!ctx || !role) return -1;
    for (int i = start_index; i < ctx->count; i++) {
        if (ctx->messages[i].role && strcmp(ctx->messages[i].role, role) == 0) {
            return i;
        }
    }
    return -1;
}

int context_remove_last(Context *ctx) {
    if (!ctx || ctx->count <= 0) return 0;
    int last = ctx->count - 1;
    free(ctx->messages[last].role);
    free(ctx->messages[last].content);
    free(ctx->messages[last].tool_call_id);
    ctx->messages[last].role = NULL;
    ctx->messages[last].content = NULL;
    ctx->messages[last].tool_call_id = NULL;
    ctx->count--;
    return 1;
}

int context_trim_to_size(Context *ctx, int max_messages) {
    if (!ctx || max_messages < 0) return 0;
    if (ctx->count <= max_messages) return 0;

    int removed = 0;
    while (ctx->count > max_messages) {
        int has_system = (ctx->count > 0 && ctx->messages[0].role &&
                         strcmp(ctx->messages[0].role, "system") == 0);
        int start = has_system ? 1 : 0;
        if (start >= ctx->count) break;

        free(ctx->messages[start].role);
        free(ctx->messages[start].content);
        free(ctx->messages[start].tool_call_id);

        for (int i = start; i < ctx->count - 1; i++) {
            ctx->messages[i] = ctx->messages[i + 1];
        }
        ctx->count--;
        removed++;
    }
    return removed;
}

int context_save(const Context *ctx, const char *filename) {
    if (!ctx || !filename) return 0;

    cJSON *root = cJSON_CreateArray();

    for (int i = 0; i < ctx->count; i++) {
        cJSON *msg = cJSON_CreateObject();
        cJSON_AddStringToObject(msg, "role", ctx->messages[i].role);
        cJSON_AddStringToObject(msg, "content", ctx->messages[i].content);
        if (ctx->messages[i].tool_call_id) {
            cJSON_AddStringToObject(msg, "tool_call_id", ctx->messages[i].tool_call_id);
        }
        cJSON_AddItemToArray(root, msg);
    }

    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);

    int result = utils_write_file(filename, json_str);
    free(json_str);
    return result;
}

char* context_to_json(const Context *ctx) {
    if (!ctx) return NULL;

    cJSON *root = cJSON_CreateArray();
    for (int i = 0; i < ctx->count; i++) {
        cJSON *msg = cJSON_CreateObject();
        cJSON_AddStringToObject(msg, "role", ctx->messages[i].role);
        cJSON_AddStringToObject(msg, "content", ctx->messages[i].content);
        if (ctx->messages[i].tool_call_id) {
            cJSON_AddStringToObject(msg, "tool_call_id", ctx->messages[i].tool_call_id);
        }
        cJSON_AddItemToArray(root, msg);
    }

    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);
    return json_str;
}

int context_replace_message(Context *ctx, int index, const char *role, const char *content) {
    if (!ctx || !role || !content) return 0;
    if (index < 0 || index >= ctx->count) return 0;

    char *new_role = strdup(role);
    char *new_content = strdup(content);
    if (!new_role || !new_content) {
        free(new_role);
        free(new_content);
        return 0;
    }

    free(ctx->messages[index].role);
    free(ctx->messages[index].content);
    ctx->messages[index].role = new_role;
    ctx->messages[index].content = new_content;
    return 1;
}
