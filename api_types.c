#include "api_types.h"
#include <stdlib.h>
#include <string.h>

ApiProvider api_provider_from_string(const char *provider_str) {
    if (!provider_str) return API_OPENAI;
    if (strcmp(provider_str, "openai") == 0) return API_OPENAI;
    if (strcmp(provider_str, "google") == 0) return API_GOOGLE;
    if (strcmp(provider_str, "claude") == 0) return API_CLAUDE;
    return API_OPENAI;
}

const char* api_provider_to_string(ApiProvider provider) {
    switch (provider) {
        case API_OPENAI: return "openai";
        case API_GOOGLE: return "google";
        case API_CLAUDE: return "claude";
        default: return "openai";
    }
}

const char* api_error_code_to_string(ApiErrorCode code) {
    switch (code) {
        case API_SUCCESS: return "成功";
        case API_ERR_INVALID_PARAM: return "无效参数";
        case API_ERR_NETWORK: return "网络错误";
        case API_ERR_HTTP_ERROR: return "HTTP错误";
        case API_ERR_PARSE: return "解析错误";
        case API_ERR_NO_CONTENT: return "无内容";
        case API_ERR_TIMEOUT: return "请求超时";
        case API_ERR_MEMORY: return "内存不足";
        default: return "未知错误";
    }
}

void tool_call_free(ToolCall *tc) {
    if (!tc) return;
    free(tc->id);
    free(tc->type);
    free(tc->function_name);
    free(tc->function_arguments);
    memset(tc, 0, sizeof(ToolCall));
}

void tool_calls_free(ToolCall *tcs, int count) {
    if (!tcs) return;
    for (int i = 0; i < count; i++) {
        free(tcs[i].id);
        free(tcs[i].type);
        free(tcs[i].function_name);
        free(tcs[i].function_arguments);
    }
    free(tcs);
}
