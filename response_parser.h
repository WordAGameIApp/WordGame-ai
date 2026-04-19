#ifndef RESPONSE_PARSER_H
#define RESPONSE_PARSER_H

#include "api_types.h"
#include "cJSON.h"

typedef struct {
    char *content;
    char *reasoningContent;
    char *errorMessage;
    int hasError;
    int hasToolCalls;
    int promptTokens;
    int completionTokens;
    int totalTokens;
} ApiResponse;

void apiResponseInit(ApiResponse *resp);
void apiResponseFree(ApiResponse *resp);

ApiResponse* apiResponseParse(const char *rawResponse);

int responseParseToolCalls(const char *rawResponse, char ***ids, char ***types,
                           char ***funcNames, char ***funcArgs, int *count);
int responseParseToolCallsSimple(const char *rawResponse, ToolCall **tool_calls, int *count);
void freeParsedToolCalls(char **ids, char **types, char **funcNames, char **funcArgs, int count);

char* responseExtractContent(const char *rawResponse);
char* responseExtractReasoningContent(const char *rawResponse);
char* responseExtractErrorMessage(const char *rawResponse);
int responseHasError(const char *rawResponse);
int responseExtractHttpStatusCode(const char *rawResponse);

#endif
