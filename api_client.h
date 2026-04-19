#ifndef API_CLIENT_H
#define API_CLIENT_H

#include "api_types.h"
#include "config.h"
#include "context.h"
#include "cJSON.h"
#include <curl/curl.h>

typedef void (*ApiLogCallback)(const char *message);

char* build_request_body(ApiProvider provider, const char *model, Context *ctx,
                         cJSON *tools, const char *tool_choice, GenerationParams *params);

struct curl_slist* build_headers(ApiProvider provider, const char *api_key);

char* send_api_request_raw(ApiProvider provider, const char *url, const char *api_key,
                           const char *model, Context *ctx,
                           cJSON *tools, const char *tool_choice, GenerationParams *params);

int send_api_request(ApiProvider provider, const char *url, const char *api_key,
                     const char *model, Context *ctx,
                     cJSON *tools, const char *tool_choice, GenerationParams *params);

void api_set_log_callback(ApiLogCallback cb);
void api_set_book_storage_enabled(int enabled);
int api_global_init(void);
void api_global_cleanup(void);
int api_set_retry_count(int count);
int api_set_retry_delay_ms(int ms);
void api_set_timeout(long timeout_seconds);
void api_set_connect_timeout(long timeout_seconds);

void print_tool_calls(const ToolCall *tool_calls, int count);

#endif
