#include "context_manager.h"
#include "utils.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void context_manager_init(ContextManager *mgr, int capacity) {
    if (!mgr || capacity <= 0) return;
    mgr->contexts = (NamedContext *)malloc(sizeof(NamedContext) * (size_t)capacity);
    if (!mgr->contexts) {
        mgr->count = 0;
        mgr->capacity = 0;
        mgr->default_context = NULL;
        return;
    }
    mgr->count = 0;
    mgr->capacity = capacity;
    mgr->default_context = NULL;
}

void context_manager_free(ContextManager *mgr) {
    if (!mgr) return;
    for (int i = 0; i < mgr->count; i++) {
        free(mgr->contexts[i].name);
        free(mgr->contexts[i].filename);
        context_free(&mgr->contexts[i].ctx);
    }
    free(mgr->contexts);
    free(mgr->default_context);
    mgr->contexts = NULL;
    mgr->count = 0;
    mgr->capacity = 0;
    mgr->default_context = NULL;
}

int context_manager_create(ContextManager *mgr, const char *name,
                           const char *filename, int message_limit,
                           int auto_compress, int compress_threshold) {
    if (!mgr || !name) return 0;

    NamedContext *existing = context_manager_get(mgr, name);
    if (existing) return 0;

    if (mgr->count >= mgr->capacity) {
        int new_capacity = mgr->capacity * 2;
        if (new_capacity <= 0) new_capacity = 10;
        NamedContext *new_ctxs = (NamedContext *)realloc(mgr->contexts, sizeof(NamedContext) * (size_t)new_capacity);
        if (!new_ctxs) return 0;
        mgr->contexts = new_ctxs;
        mgr->capacity = new_capacity;
    }

    NamedContext *nctx = &mgr->contexts[mgr->count];
    nctx->name = strdup(name);
    nctx->filename = filename ? strdup(filename) : NULL;
    nctx->message_count = message_limit > 0 ? message_limit : 100;
    nctx->auto_compress = auto_compress;
    nctx->compress_threshold = compress_threshold > 0 ? compress_threshold : 50;

    if (!nctx->name) {
        free(nctx->filename);
        nctx->filename = NULL;
        return 0;
    }

    context_init(&nctx->ctx, nctx->message_count);

    if (nctx->filename) {
        context_load(&nctx->ctx, nctx->filename);
    }

    mgr->count++;

    if (mgr->count == 1 && !mgr->default_context) {
        mgr->default_context = strdup(name);
    }

    return 1;
}

NamedContext* context_manager_get(const ContextManager *mgr, const char *name) {
    if (!mgr || !name) return NULL;
    for (int i = 0; i < mgr->count; i++) {
        if (strcmp(mgr->contexts[i].name, name) == 0) {
            return &mgr->contexts[i];
        }
    }
    return NULL;
}

int context_manager_exists(const ContextManager *mgr, const char *name) {
    return context_manager_get(mgr, name) != NULL;
}

int context_manager_remove(ContextManager *mgr, const char *name) {
    if (!mgr || !name) return 0;
    for (int i = 0; i < mgr->count; i++) {
        if (strcmp(mgr->contexts[i].name, name) == 0) {
            free(mgr->contexts[i].name);
            free(mgr->contexts[i].filename);
            context_free(&mgr->contexts[i].ctx);

            for (int j = i; j < mgr->count - 1; j++) {
                mgr->contexts[j] = mgr->contexts[j + 1];
            }
            mgr->count--;

            if (mgr->default_context && strcmp(mgr->default_context, name) == 0) {
                free(mgr->default_context);
                mgr->default_context = mgr->count > 0 ? strdup(mgr->contexts[0].name) : NULL;
            }

            return 1;
        }
    }
    return 0;
}

int context_manager_rename(ContextManager *mgr, const char *old_name, const char *new_name) {
    if (!mgr || !old_name || !new_name) return 0;
    if (context_manager_exists(mgr, new_name)) return 0;

    NamedContext *nctx = context_manager_get(mgr, old_name);
    if (!nctx) return 0;

    char *new_name_copy = strdup(new_name);
    if (!new_name_copy) return 0;

    free(nctx->name);
    nctx->name = new_name_copy;

    if (mgr->default_context && strcmp(mgr->default_context, old_name) == 0) {
        free(mgr->default_context);
        mgr->default_context = strdup(new_name);
    }

    return 1;
}

int context_manager_switch(ContextManager *mgr, const char *name) {
    if (!mgr || !name) return 0;
    NamedContext *nctx = context_manager_get(mgr, name);
    if (nctx) {
        free(mgr->default_context);
        mgr->default_context = strdup(name);
        return mgr->default_context ? 1 : 0;
    }
    return 0;
}

NamedContext* context_manager_get_current(const ContextManager *mgr) {
    if (!mgr || !mgr->default_context) return NULL;
    return context_manager_get(mgr, mgr->default_context);
}

int context_manager_load_all(ContextManager *mgr) {
    if (!mgr) return 0;
    int loaded = 0;
    for (int i = 0; i < mgr->count; i++) {
        if (mgr->contexts[i].filename) {
            if (context_load(&mgr->contexts[i].ctx, mgr->contexts[i].filename)) {
                loaded++;
            }
        }
    }
    return loaded;
}

int context_manager_save_all(const ContextManager *mgr) {
    if (!mgr) return 0;
    int saved = 0;
    for (int i = 0; i < mgr->count; i++) {
        if (mgr->contexts[i].filename) {
            if (context_save(&mgr->contexts[i].ctx, mgr->contexts[i].filename)) {
                saved++;
            }
        }
    }
    return saved;
}

int context_compress_manual(NamedContext *nctx, const char *summary) {
    if (!nctx || !summary) return 0;
    if (nctx->ctx.count < 2) return 0;

    int original_count = nctx->ctx.count;

    char *system_msg = NULL;
    if (nctx->ctx.count > 0 && nctx->ctx.messages[0].role &&
        strcmp(nctx->ctx.messages[0].role, "system") == 0) {
        system_msg = strdup(nctx->ctx.messages[0].content);
    }

    context_clear(&nctx->ctx);

    if (system_msg) {
        context_add_message(&nctx->ctx, "system", system_msg, NULL);
        free(system_msg);
    }

    size_t msg_len = strlen(summary) + 128;
    char *compressed_msg = (char *)malloc(msg_len);
    if (!compressed_msg) return 0;

    snprintf(compressed_msg, msg_len,
             "[上下文已压缩] 原对话包含 %d 条消息。\n摘要：%s",
             original_count, summary);
    context_add_message(&nctx->ctx, "system", compressed_msg, NULL);
    free(compressed_msg);

    printf("上下文已手动压缩，原 %d 条消息 -> 摘要\n", original_count);
    return 1;
}

int context_auto_compress_if_needed(NamedContext *nctx) {
    if (!nctx || !nctx->auto_compress) return 0;
    if (nctx->ctx.count < nctx->compress_threshold) return 0;

    int target = nctx->compress_threshold / 2;
    if (target < 2) target = 2;

    int removed = context_trim_to_size(&nctx->ctx, target);
    if (removed > 0) {
        char msg[256];
        snprintf(msg, sizeof(msg), "[自动压缩] 移除了 %d 条旧消息，保留 %d 条", removed, nctx->ctx.count);
        context_add_message(&nctx->ctx, "system", msg, NULL);
        printf("上下文自动压缩: 移除 %d 条消息\n", removed);
    }
    return removed;
}

void context_get_stats(const NamedContext *nctx, int *total_messages, int *total_chars) {
    if (total_messages) *total_messages = 0;
    if (total_chars) *total_chars = 0;

    if (!nctx) return;

    if (total_messages) *total_messages = nctx->ctx.count;
    if (total_chars) {
        *total_chars = context_total_chars(&nctx->ctx);
    }
}

void context_manager_list(const ContextManager *mgr) {
    if (!mgr) return;
    printf("\n=== 上下文列表 (%d 个) ===\n", mgr->count);
    for (int i = 0; i < mgr->count; i++) {
        const NamedContext *nctx = &mgr->contexts[i];
        int msgs, chars;
        context_get_stats(nctx, &msgs, &chars);

        printf("\n[%d] %s %s\n", i + 1, nctx->name,
               (mgr->default_context && strcmp(mgr->default_context, nctx->name) == 0) ? "(当前)" : "");
        printf("    文件: %s\n", nctx->filename ? nctx->filename : "无");
        printf("    消息: %d/%d\n", msgs, nctx->message_count);
        printf("    字符: %d\n", chars);
        printf("    自动压缩: %s (阈值: %d)\n",
               nctx->auto_compress ? "开启" : "关闭",
               nctx->compress_threshold);
    }
    printf("\n========================\n");
}

int context_manager_count(const ContextManager *mgr) {
    if (!mgr) return 0;
    return mgr->count;
}

int context_manager_export_context(const ContextManager *mgr, const char *name, const char *output_file) {
    if (!mgr || !name || !output_file) return 0;
    const NamedContext *nctx = context_manager_get(mgr, name);
    if (!nctx) return 0;
    return context_save(&nctx->ctx, output_file);
}

int context_manager_save_state(const ContextManager *mgr, const char *stateFile) {
    if (!mgr || !stateFile) return 0;

    cJSON *root = cJSON_CreateObject();
    if (!root) return 0;

    if (mgr->default_context) {
        cJSON_AddStringToObject(root, "default_context", mgr->default_context);
    }

    cJSON *ctx_list = cJSON_CreateArray();
    for (int i = 0; i < mgr->count; i++) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "name", mgr->contexts[i].name);
        if (mgr->contexts[i].filename)
            cJSON_AddStringToObject(item, "filename", mgr->contexts[i].filename);
        cJSON_AddNumberToObject(item, "message_count", mgr->contexts[i].message_count);
        cJSON_AddBoolToObject(item, "auto_compress", mgr->contexts[i].auto_compress);
        cJSON_AddNumberToObject(item, "compress_threshold", mgr->contexts[i].compress_threshold);
        cJSON_AddItemToArray(ctx_list, item);
    }
    cJSON_AddItemToObject(root, "contexts", ctx_list);

    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);

    int result = utils_write_file(stateFile, json_str);
    free(json_str);
    return result;
}

int context_manager_load_state(ContextManager *mgr, const char *stateFile) {
    if (!mgr || !stateFile) return 0;

    char *json_str = utils_read_file(stateFile);
    if (!json_str) return 0;

    cJSON *root = cJSON_Parse(json_str);
    free(json_str);
    if (!root) return 0;

    cJSON *dc = cJSON_GetObjectItemCaseSensitive(root, "default_context");
    if (cJSON_IsString(dc) && dc->valuestring) {
        free(mgr->default_context);
        mgr->default_context = strdup(dc->valuestring);
    }

    cJSON *ctx_list = cJSON_GetObjectItemCaseSensitive(root, "contexts");
    if (cJSON_IsArray(ctx_list)) {
        int size = cJSON_GetArraySize(ctx_list);
        for (int i = 0; i < size; i++) {
            cJSON *item = cJSON_GetArrayItem(ctx_list, i);
            cJSON *name = cJSON_GetObjectItemCaseSensitive(item, "name");
            cJSON *filename = cJSON_GetObjectItemCaseSensitive(item, "filename");
            cJSON *msg_count = cJSON_GetObjectItemCaseSensitive(item, "message_count");
            cJSON *auto_compress = cJSON_GetObjectItemCaseSensitive(item, "auto_compress");
            cJSON *compress_threshold = cJSON_GetObjectItemCaseSensitive(item, "compress_threshold");

            if (!cJSON_IsString(name)) continue;

            const char *fname = cJSON_IsString(filename) ? filename->valuestring : NULL;
            int mc = cJSON_IsNumber(msg_count) ? msg_count->valueint : 100;
            int ac = cJSON_IsBool(auto_compress) ? cJSON_IsTrue(auto_compress) : 0;
            int ct = cJSON_IsNumber(compress_threshold) ? compress_threshold->valueint : 50;

            context_manager_create(mgr, name->valuestring, fname, mc, ac, ct);
        }
    }

    cJSON_Delete(root);
    return 1;
}
