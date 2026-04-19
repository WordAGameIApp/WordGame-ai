#ifndef API_TYPES_H
#define API_TYPES_H

typedef enum {
    API_OPENAI,
    API_GOOGLE,
    API_CLAUDE
} ApiProvider;

typedef enum {
    API_SUCCESS = 0,
    API_ERR_INVALID_PARAM = 1,
    API_ERR_NETWORK = 2,
    API_ERR_HTTP_ERROR = 3,
    API_ERR_PARSE = 4,
    API_ERR_NO_CONTENT = 5,
    API_ERR_TIMEOUT = 6,
    API_ERR_MEMORY = 7
} ApiErrorCode;

typedef struct {
    char *id;
    char *type;
    char *function_name;
    char *function_arguments;
} ToolCall;

ApiProvider api_provider_from_string(const char *provider_str);
const char* api_provider_to_string(ApiProvider provider);
const char* api_error_code_to_string(ApiErrorCode code);

void tool_call_free(ToolCall *tc);
void tool_calls_free(ToolCall *tcs, int count);

#endif
