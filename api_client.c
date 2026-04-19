#include "api_client.h"
#include "api_types.h"
#include "response_parser.h"
#include "book_storage.h"
#include "utils.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

static ApiLogCallback g_log_callback = NULL;
static int g_book_storage_enabled = 1;
static int g_curl_initialized = 0;
static int g_retry_count = 0;
static int g_retry_delay_ms = 1000;
static long g_timeout = 120L;
static long g_connect_timeout = 30L;

int api_set_retry_count(int count) {
    if (count < 0 || count > 10) return 0;
    g_retry_count = count;
    return 1;
}

int api_set_retry_delay_ms(int ms) {
    if (ms < 0 || ms > 30000) return 0;
    g_retry_delay_ms = ms;
    return 1;
}

void api_set_timeout(long timeout_seconds) {
    if (timeout_seconds > 0) g_timeout = timeout_seconds;
}

void api_set_connect_timeout(long timeout_seconds) {
    if (timeout_seconds > 0) g_connect_timeout = timeout_seconds;
}

int api_global_init(void) {
    if (g_curl_initialized) return 1;
    CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (res != CURLE_OK) {
        fprintf(stderr, "CURL 全局初始化失败: %s\n", curl_easy_strerror(res));
        return 0;
    }
    g_curl_initialized = 1;
    return 1;
}

void api_global_cleanup(void) {
    if (g_curl_initialized) {
        curl_global_cleanup();
        g_curl_initialized = 0;
    }
}

void api_set_log_callback(ApiLogCallback cb) {
    g_log_callback = cb;
}

void api_set_book_storage_enabled(int enabled) {
    g_book_storage_enabled = enabled;
}

static void api_log(const char *message) {
    if (g_log_callback) {
        g_log_callback(message);
    } else {
        printf("%s\n", message);
    }
}

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    char *ptr = (char *)realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) return 0;
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

static void add_openai_body(cJSON *root, const char *model, Context *ctx,
                            cJSON *tools, const char *tool_choice, GenerationParams *validated) {
    cJSON_AddStringToObject(root, "model", model);
    if (!validated->use_default) {
        cJSON_AddNumberToObject(root, "temperature", validated->temperature);
        cJSON_AddNumberToObject(root, "top_p", validated->top_p);
        cJSON_AddNumberToObject(root, "max_tokens", validated->max_tokens);
        if (validated->presence_penalty != 0.0)
            cJSON_AddNumberToObject(root, "presence_penalty", validated->presence_penalty);
        if (validated->frequency_penalty != 0.0)
            cJSON_AddNumberToObject(root, "frequency_penalty", validated->frequency_penalty);
        if (validated->enable_thinking) {
            cJSON *thinking = cJSON_CreateObject();
            cJSON_AddStringToObject(thinking, "type", "enabled");
            if (validated->thinking_budget > 0)
                cJSON_AddNumberToObject(thinking, "budget_tokens", validated->thinking_budget);
            cJSON_AddItemToObject(root, "thinking", thinking);
        }
    } else {
        cJSON_AddNumberToObject(root, "temperature", 0.7);
        cJSON_AddNumberToObject(root, "max_tokens", 1024);
    }
    cJSON *messages = cJSON_CreateArray();
    for (int i = 0; i < ctx->count; i++) {
        cJSON *msg = cJSON_CreateObject();
        cJSON_AddStringToObject(msg, "role", ctx->messages[i].role);
        cJSON_AddStringToObject(msg, "content", ctx->messages[i].content);
        if (ctx->messages[i].tool_call_id)
            cJSON_AddStringToObject(msg, "tool_call_id", ctx->messages[i].tool_call_id);
        cJSON_AddItemToArray(messages, msg);
    }
    cJSON_AddItemToObject(root, "messages", messages);
    if (tools) {
        cJSON_AddItemToObject(root, "tools", cJSON_Duplicate(tools, 1));
        if (tool_choice) {
            if (strcmp(tool_choice, "auto") == 0 || strcmp(tool_choice, "none") == 0) {
                cJSON_AddStringToObject(root, "tool_choice", tool_choice);
            } else {
                cJSON *tc = cJSON_CreateObject();
                cJSON_AddStringToObject(tc, "type", "function");
                cJSON *func = cJSON_CreateObject();
                cJSON_AddStringToObject(func, "name", tool_choice);
                cJSON_AddItemToObject(tc, "function", func);
                cJSON_AddItemToObject(root, "tool_choice", tc);
            }
        }
    }
}

static void add_google_body(cJSON *root, Context *ctx, cJSON *tools, GenerationParams *validated) {
    cJSON *contents = cJSON_CreateArray();
    for (int i = 0; i < ctx->count; i++) {
        cJSON *content = cJSON_CreateObject();
        cJSON *parts = cJSON_CreateArray();
        cJSON *part = cJSON_CreateObject();
        cJSON_AddStringToObject(part, "text", ctx->messages[i].content);
        cJSON_AddItemToArray(parts, part);
        cJSON_AddItemToObject(content, "parts", parts);
        const char *role = ctx->messages[i].role;
        if (strcmp(role, "user") == 0) cJSON_AddStringToObject(content, "role", "user");
        else if (strcmp(role, "assistant") == 0) cJSON_AddStringToObject(content, "role", "model");
        cJSON_AddItemToArray(contents, content);
    }
    cJSON_AddItemToObject(root, "contents", contents);
    cJSON *gc = cJSON_CreateObject();
    if (!validated->use_default) {
        cJSON_AddNumberToObject(gc, "temperature", validated->temperature);
        cJSON_AddNumberToObject(gc, "topP", validated->top_p);
        cJSON_AddNumberToObject(gc, "maxOutputTokens", validated->max_tokens);
    } else {
        cJSON_AddNumberToObject(gc, "temperature", 0.7);
        cJSON_AddNumberToObject(gc, "maxOutputTokens", 1024);
    }
    cJSON_AddItemToObject(root, "generationConfig", gc);
    if (tools) {
        cJSON *tools_decl = cJSON_CreateArray();
        int tc = cJSON_GetArraySize(tools);
        for (int i = 0; i < tc; i++) {
            cJSON *tool = cJSON_GetArrayItem(tools, i);
            cJSON *func = cJSON_GetObjectItemCaseSensitive(tool, "function");
            if (func) {
                cJSON *decl = cJSON_CreateObject();
                cJSON *name = cJSON_GetObjectItemCaseSensitive(func, "name");
                cJSON *desc = cJSON_GetObjectItemCaseSensitive(func, "description");
                cJSON *p = cJSON_GetObjectItemCaseSensitive(func, "parameters");
                if (name) cJSON_AddStringToObject(decl, "name", name->valuestring);
                if (desc) cJSON_AddStringToObject(decl, "description", desc->valuestring);
                if (p) cJSON_AddItemToObject(decl, "parameters", cJSON_Duplicate(p, 1));
                cJSON_AddItemToArray(tools_decl, decl);
            }
        }
        cJSON *tools_root = cJSON_CreateObject();
        cJSON_AddItemToObject(tools_root, "function_declarations", tools_decl);
        cJSON_AddItemToObject(root, "tools", tools_root);
    }
}

static void add_claude_body(cJSON *root, const char *model, Context *ctx,
                            cJSON *tools, GenerationParams *validated) {
    cJSON_AddStringToObject(root, "model", model);
    cJSON_AddNumberToObject(root, "max_tokens", validated->use_default ? 1024 : validated->max_tokens);
    if (!validated->use_default) {
        cJSON_AddNumberToObject(root, "temperature", validated->temperature);
        cJSON_AddNumberToObject(root, "top_p", validated->top_p);
    } else {
        cJSON_AddNumberToObject(root, "temperature", 0.7);
    }

    char *system_content = NULL;
    int system_idx = context_find_by_role(ctx, "system", 0);
    if (system_idx >= 0) {
        system_content = ctx->messages[system_idx].content;
    }

    if (system_content) {
        cJSON_AddStringToObject(root, "system", system_content);
    }

    cJSON *messages = cJSON_CreateArray();
    for (int i = 0; i < ctx->count; i++) {
        if (strcmp(ctx->messages[i].role, "system") == 0) continue;
        cJSON *msg = cJSON_CreateObject();
        cJSON_AddStringToObject(msg, "role", ctx->messages[i].role);
        cJSON_AddStringToObject(msg, "content", ctx->messages[i].content);
        cJSON_AddItemToArray(messages, msg);
    }
    cJSON_AddItemToObject(root, "messages", messages);
    if (tools) {
        cJSON *ta = cJSON_CreateArray();
        int tc = cJSON_GetArraySize(tools);
        for (int i = 0; i < tc; i++) {
            cJSON *tool = cJSON_GetArrayItem(tools, i);
            cJSON *func = cJSON_GetObjectItemCaseSensitive(tool, "function");
            if (func) cJSON_AddItemToArray(ta, cJSON_Duplicate(func, 1));
        }
        cJSON_AddItemToObject(root, "tools", ta);
    }
}

char* build_request_body(ApiProvider provider, const char *model, Context *ctx,
                         cJSON *tools, const char *tool_choice, GenerationParams *params) {
    if (!model || !ctx) return NULL;
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;

    GenerationParams defaultParams;
    config_init_default_params(&defaultParams);
    GenerationParams validated = params ? config_validate_params(params) : defaultParams;

    switch (provider) {
        case API_OPENAI: add_openai_body(root, model, ctx, tools, tool_choice, &validated); break;
        case API_GOOGLE: add_google_body(root, ctx, tools, &validated); break;
        case API_CLAUDE: add_claude_body(root, model, ctx, tools, &validated); break;
    }

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return body;
}

struct curl_slist* build_headers(ApiProvider provider, const char *api_key) {
    struct curl_slist *headers = NULL;
    char header_buf[1024];
    headers = curl_slist_append(headers, "Content-Type: application/json");
    switch (provider) {
        case API_OPENAI:
            snprintf(header_buf, sizeof(header_buf), "Authorization: Bearer %s", api_key ? api_key : "");
            headers = curl_slist_append(headers, header_buf);
            break;
        case API_GOOGLE:
            break;
        case API_CLAUDE:
            snprintf(header_buf, sizeof(header_buf), "x-api-key: %s", api_key ? api_key : "");
            headers = curl_slist_append(headers, header_buf);
            headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");
            break;
    }
    return headers;
}

char* send_api_request_raw(ApiProvider provider, const char *url, const char *api_key,
                           const char *model, Context *ctx,
                           cJSON *tools, const char *tool_choice, GenerationParams *params) {
    if (!url || !api_key || !model || !ctx) {
        fprintf(stderr, "错误: 无效的参数\n");
        return NULL;
    }

    CURL *curl;
    CURLcode res;
    struct MemoryStruct response = {0};
    char *full_url = NULL;

    response.memory = (char *)malloc(1);
    if (!response.memory) { fprintf(stderr, "内存分配失败\n"); return NULL; }
    response.size = 0;

    curl = curl_easy_init();
    if (!curl) { fprintf(stderr, "CURL 初始化失败\n"); free(response.memory); return NULL; }

    if (provider == API_GOOGLE && api_key) {
        size_t url_len = strlen(url) + strlen(api_key) + 16;
        full_url = (char *)malloc(url_len);
        if (!full_url) {
            curl_easy_cleanup(curl);
            free(response.memory);
            return NULL;
        }
        snprintf(full_url, url_len, "%s?key=%s", url, api_key);
    } else {
        full_url = strdup(url);
    }

    char *post_data = build_request_body(provider, model, ctx, tools, tool_choice, params);
    if (!post_data) {
        fprintf(stderr, "构建请求体失败\n");
        curl_easy_cleanup(curl);
        free(response.memory);
        free(full_url);
        return NULL;
    }

    char masked_key[64];
    utils_mask_api_key(api_key, masked_key, sizeof(masked_key));

    char log_buf[256];
    snprintf(log_buf, sizeof(log_buf), "请求 URL: %s", full_url);
    api_log(log_buf);
    snprintf(log_buf, sizeof(log_buf), "API Key: %s", masked_key);
    api_log(log_buf);

    curl_easy_setopt(curl, CURLOPT_URL, full_url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, g_timeout);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, g_connect_timeout);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    struct curl_slist *headers = build_headers(provider, api_key);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    int attempt;
    for (attempt = 0; attempt <= g_retry_count; attempt++) {
        if (attempt > 0) {
            snprintf(log_buf, sizeof(log_buf), "重试第 %d 次...", attempt);
            api_log(log_buf);
#ifdef _WIN32
            Sleep((DWORD)g_retry_delay_ms);
#else
            usleep((useconds_t)g_retry_delay_ms * 1000);
#endif
        }

        free(response.memory);
        response.memory = (char *)malloc(1);
        if (!response.memory) break;
        response.size = 0;

        res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            long http_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            snprintf(log_buf, sizeof(log_buf), "HTTP 状态码: %ld", http_code);
            api_log(log_buf);

            if (http_code < 500) break;

            snprintf(log_buf, sizeof(log_buf), "服务器错误 %ld，将重试", http_code);
            api_log(log_buf);
        } else {
            fprintf(stderr, "请求失败: %s\n", curl_easy_strerror(res));
        }
    }

    char *result = NULL;
    if (res == CURLE_OK && response.memory && response.size > 0) {
        result = strdup(response.memory);
    } else if (res == CURLE_OK) {
        fprintf(stderr, "错误: 空响应\n");
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(post_data);
    free(response.memory);
    free(full_url);
    return result;
}

int send_api_request(ApiProvider provider, const char *url, const char *api_key,
                     const char *model, Context *ctx,
                     cJSON *tools, const char *tool_choice, GenerationParams *params) {
    if (!url || !api_key || !model || !ctx) {
        fprintf(stderr, "API请求参数无效: %s\n", api_error_code_to_string(API_ERR_INVALID_PARAM));
        return API_ERR_INVALID_PARAM;
    }

    char *raw = send_api_request_raw(provider, url, api_key, model, ctx, tools, tool_choice, params);
    if (!raw) {
        fprintf(stderr, "API请求失败: %s\n", api_error_code_to_string(API_ERR_NETWORK));
        return API_ERR_NETWORK;
    }

    ApiResponse *resp = apiResponseParse(raw);
    if (!resp) {
        fprintf(stderr, "API响应解析失败: %s\n", api_error_code_to_string(API_ERR_PARSE));
        free(raw);
        return API_ERR_PARSE;
    }

    int result = API_ERR_NO_CONTENT;

    if (resp->hasError) {
        if (resp->errorMessage) fprintf(stderr, "API 错误: %s\n", resp->errorMessage);
        apiResponseFree(resp);
        free(raw);
        return API_ERR_HTTP_ERROR;
    }

    if (resp->hasToolCalls) {
        ToolCall *tool_calls = NULL;
        int tool_count = 0;
        if (responseParseToolCallsSimple(raw, &tool_calls, &tool_count)) {
            print_tool_calls(tool_calls, tool_count);
            tool_calls_free(tool_calls, tool_count);
        }
    }

    if (resp->content) {
        context_add_message(ctx, "assistant", resp->content, NULL);
        if (g_book_storage_enabled) {
            bookStorageAppendWithTimestamp("对话记录", resp->content);
        }
        result = API_SUCCESS;
    } else {
        fprintf(stderr, "API返回空内容: %s\n", api_error_code_to_string(API_ERR_NO_CONTENT));
    }

    if (resp->totalTokens > 0) {
        char log_buf[256];
        snprintf(log_buf, sizeof(log_buf),
                 "Token 使用: prompt=%d, completion=%d, total=%d",
                 resp->promptTokens, resp->completionTokens, resp->totalTokens);
        api_log(log_buf);
    }

    apiResponseFree(resp);
    free(raw);
    return result;
}

void print_tool_calls(const ToolCall *tool_calls, int count) {
    if (!tool_calls || count <= 0) return;
    printf("\n=== 工具调用 (%d 个) ===\n", count);
    for (int i = 0; i < count; i++) {
        printf("\n工具 #%d:\n", i + 1);
        printf("  ID: %s\n", tool_calls[i].id);
        printf("  类型: %s\n", tool_calls[i].type);
        printf("  函数名: %s\n", tool_calls[i].function_name);
        printf("  参数: %s\n", tool_calls[i].function_arguments);
        cJSON *args = cJSON_Parse(tool_calls[i].function_arguments);
        if (args) {
            char *formatted = cJSON_Print(args);
            printf("  格式化参数:\n%s\n", formatted);
            free(formatted); cJSON_Delete(args);
        }
    }
    printf("\n=== 工具调用结束 ===\n");
}
