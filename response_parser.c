#include "response_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void apiResponseInit(ApiResponse *resp) {
    if (!resp) return;
    memset(resp, 0, sizeof(ApiResponse));
}

void apiResponseFree(ApiResponse *resp) {
    if (!resp) return;
    free(resp->content);
    free(resp->reasoningContent);
    free(resp->errorMessage);
    free(resp);
}

ApiResponse* apiResponseParse(const char *rawResponse) {
    ApiResponse *resp = (ApiResponse *)malloc(sizeof(ApiResponse));
    if (!resp) return NULL;
    apiResponseInit(resp);

    if (!rawResponse) return resp;

    cJSON *root = cJSON_Parse(rawResponse);
    if (!root) return resp;

    cJSON *error = cJSON_GetObjectItemCaseSensitive(root, "error");
    if (error) {
        resp->hasError = 1;
        cJSON *errMsg = cJSON_GetObjectItemCaseSensitive(error, "message");
        if (cJSON_IsString(errMsg) && errMsg->valuestring) {
            resp->errorMessage = strdup(errMsg->valuestring);
        }
        cJSON_Delete(root);
        return resp;
    }

    cJSON *candidates = cJSON_GetObjectItemCaseSensitive(root, "candidates");
    if (cJSON_IsArray(candidates) && cJSON_GetArraySize(candidates) > 0) {
        cJSON *first = cJSON_GetArrayItem(candidates, 0);
        cJSON *content_obj = cJSON_GetObjectItemCaseSensitive(first, "content");
        if (content_obj) {
            cJSON *parts = cJSON_GetObjectItemCaseSensitive(content_obj, "parts");
            if (cJSON_IsArray(parts) && cJSON_GetArraySize(parts) > 0) {
                cJSON *part = cJSON_GetArrayItem(parts, 0);
                cJSON *text = cJSON_GetObjectItemCaseSensitive(part, "text");
                if (cJSON_IsString(text) && text->valuestring) {
                    resp->content = strdup(text->valuestring);
                }
            }
        }
        cJSON_Delete(root);
        return resp;
    }

    cJSON *choices = cJSON_GetObjectItemCaseSensitive(root, "choices");
    if (!cJSON_IsArray(choices) || cJSON_GetArraySize(choices) == 0) {
        cJSON *content_field = cJSON_GetObjectItemCaseSensitive(root, "content");
        if (cJSON_IsArray(content_field) && cJSON_GetArraySize(content_field) > 0) {
            cJSON *block = cJSON_GetArrayItem(content_field, 0);
            cJSON *text = cJSON_GetObjectItemCaseSensitive(block, "text");
            if (cJSON_IsString(text) && text->valuestring) {
                resp->content = strdup(text->valuestring);
            } else {
                cJSON *type = cJSON_GetObjectItemCaseSensitive(block, "type");
                if (cJSON_IsString(type) && strcmp(type->valuestring, "text") == 0) {
                    cJSON *t = cJSON_GetObjectItemCaseSensitive(block, "text");
                    if (cJSON_IsString(t) && t->valuestring) {
                        resp->content = strdup(t->valuestring);
                    }
                }
            }
        } else if (cJSON_IsString(content_field) && content_field->valuestring) {
            resp->content = strdup(content_field->valuestring);
        }
        cJSON_Delete(root);
        return resp;
    }

    cJSON *first = cJSON_GetArrayItem(choices, 0);
    cJSON *message = cJSON_GetObjectItemCaseSensitive(first, "message");

    if (message) {
        cJSON *content = cJSON_GetObjectItemCaseSensitive(message, "content");
        if (cJSON_IsString(content) && content->valuestring) {
            resp->content = strdup(content->valuestring);
        }

        cJSON *reasoning = cJSON_GetObjectItemCaseSensitive(message, "reasoning_content");
        if (cJSON_IsString(reasoning) && reasoning->valuestring) {
            resp->reasoningContent = strdup(reasoning->valuestring);
        }

        cJSON *tool_calls = cJSON_GetObjectItemCaseSensitive(message, "tool_calls");
        if (cJSON_IsArray(tool_calls) && cJSON_GetArraySize(tool_calls) > 0) {
            resp->hasToolCalls = 1;
        }
    } else {
        cJSON *content = cJSON_GetObjectItemCaseSensitive(first, "content");
        if (cJSON_IsString(content) && content->valuestring) {
            resp->content = strdup(content->valuestring);
        }
    }

    cJSON *usage = cJSON_GetObjectItemCaseSensitive(root, "usage");
    if (usage) {
        cJSON *pt = cJSON_GetObjectItemCaseSensitive(usage, "prompt_tokens");
        cJSON *ct = cJSON_GetObjectItemCaseSensitive(usage, "completion_tokens");
        cJSON *tt = cJSON_GetObjectItemCaseSensitive(usage, "total_tokens");
        if (cJSON_IsNumber(pt)) resp->promptTokens = pt->valueint;
        if (cJSON_IsNumber(ct)) resp->completionTokens = ct->valueint;
        if (cJSON_IsNumber(tt)) resp->totalTokens = tt->valueint;
    }

    cJSON *usage_metadata = cJSON_GetObjectItemCaseSensitive(root, "usageMetadata");
    if (usage_metadata) {
        cJSON *pt = cJSON_GetObjectItemCaseSensitive(usage_metadata, "promptTokenCount");
        cJSON *ct = cJSON_GetObjectItemCaseSensitive(usage_metadata, "candidatesTokenCount");
        cJSON *tt = cJSON_GetObjectItemCaseSensitive(usage_metadata, "totalTokenCount");
        if (cJSON_IsNumber(pt)) resp->promptTokens = pt->valueint;
        if (cJSON_IsNumber(ct)) resp->completionTokens = ct->valueint;
        if (cJSON_IsNumber(tt)) resp->totalTokens = tt->valueint;
    }

    cJSON_Delete(root);
    return resp;
}

char* responseExtractContent(const char *rawResponse) {
    ApiResponse *resp = apiResponseParse(rawResponse);
    if (!resp) return NULL;
    char *result = resp->content;
    resp->content = NULL;
    apiResponseFree(resp);
    return result;
}

char* responseExtractReasoningContent(const char *rawResponse) {
    ApiResponse *resp = apiResponseParse(rawResponse);
    if (!resp) return NULL;
    char *result = resp->reasoningContent;
    resp->reasoningContent = NULL;
    apiResponseFree(resp);
    return result;
}

char* responseExtractErrorMessage(const char *rawResponse) {
    if (!rawResponse) return NULL;
    cJSON *root = cJSON_Parse(rawResponse);
    if (!root) return NULL;
    cJSON *error = cJSON_GetObjectItemCaseSensitive(root, "error");
    char *result = NULL;
    if (error) {
        cJSON *msg = cJSON_GetObjectItemCaseSensitive(error, "message");
        if (cJSON_IsString(msg) && msg->valuestring) {
            result = strdup(msg->valuestring);
        }
    }
    cJSON_Delete(root);
    return result;
}

int responseHasError(const char *rawResponse) {
    if (!rawResponse) return 0;
    cJSON *root = cJSON_Parse(rawResponse);
    if (!root) return 0;
    cJSON *error = cJSON_GetObjectItemCaseSensitive(root, "error");
    int hasError = error ? 1 : 0;
    cJSON_Delete(root);
    return hasError;
}

int responseExtractHttpStatusCode(const char *rawResponse) {
    if (!rawResponse) return 0;
    cJSON *root = cJSON_Parse(rawResponse);
    if (!root) return 0;
    cJSON *error = cJSON_GetObjectItemCaseSensitive(root, "error");
    int code = 0;
    if (error) {
        cJSON *status = cJSON_GetObjectItemCaseSensitive(error, "status");
        if (cJSON_IsNumber(status)) code = status->valueint;
        else {
            cJSON *code_item = cJSON_GetObjectItemCaseSensitive(error, "code");
            if (cJSON_IsNumber(code_item)) code = code_item->valueint;
        }
    }
    cJSON_Delete(root);
    return code;
}

int responseParseToolCalls(const char *rawResponse, char ***ids, char ***types,
                           char ***funcNames, char ***funcArgs, int *count) {
    if (!rawResponse || !ids || !types || !funcNames || !funcArgs || !count) return 0;

    *ids = NULL;
    *types = NULL;
    *funcNames = NULL;
    *funcArgs = NULL;
    *count = 0;

    cJSON *root = cJSON_Parse(rawResponse);
    if (!root) return 0;

    cJSON *choices = cJSON_GetObjectItemCaseSensitive(root, "choices");
    if (!cJSON_IsArray(choices) || cJSON_GetArraySize(choices) == 0) {
        cJSON_Delete(root);
        return 0;
    }

    cJSON *first = cJSON_GetArrayItem(choices, 0);
    cJSON *message = cJSON_GetObjectItemCaseSensitive(first, "message");
    if (!message) {
        cJSON_Delete(root);
        return 0;
    }

    cJSON *tool_calls = cJSON_GetObjectItemCaseSensitive(message, "tool_calls");
    if (!cJSON_IsArray(tool_calls)) {
        cJSON_Delete(root);
        return 0;
    }

    int tc = cJSON_GetArraySize(tool_calls);
    if (tc == 0) {
        cJSON_Delete(root);
        return 0;
    }

    *ids = (char **)malloc(sizeof(char*) * (size_t)tc);
    *types = (char **)malloc(sizeof(char*) * (size_t)tc);
    *funcNames = (char **)malloc(sizeof(char*) * (size_t)tc);
    *funcArgs = (char **)malloc(sizeof(char*) * (size_t)tc);

    if (!*ids || !*types || !*funcNames || !*funcArgs) {
        free(*ids); free(*types); free(*funcNames); free(*funcArgs);
        *ids = *types = *funcNames = *funcArgs = NULL;
        cJSON_Delete(root);
        return 0;
    }

    int valid = 0;
    for (int i = 0; i < tc; i++) {
        cJSON *tc_item = cJSON_GetArrayItem(tool_calls, i);
        cJSON *id = cJSON_GetObjectItemCaseSensitive(tc_item, "id");
        cJSON *type = cJSON_GetObjectItemCaseSensitive(tc_item, "type");
        cJSON *function = cJSON_GetObjectItemCaseSensitive(tc_item, "function");
        cJSON *fname = function ? cJSON_GetObjectItemCaseSensitive(function, "name") : NULL;
        cJSON *fargs = function ? cJSON_GetObjectItemCaseSensitive(function, "arguments") : NULL;

        if (!cJSON_IsString(id) || !cJSON_IsString(type) || !cJSON_IsString(fname) || !cJSON_IsString(fargs)) {
            for (int j = 0; j < valid; j++) {
                free((*ids)[j]); free((*types)[j]); free((*funcNames)[j]); free((*funcArgs)[j]);
            }
            free(*ids); free(*types); free(*funcNames); free(*funcArgs);
            *ids = *types = *funcNames = *funcArgs = NULL;
            cJSON_Delete(root);
            return 0;
        }

        (*ids)[valid] = strdup(id->valuestring);
        (*types)[valid] = strdup(type->valuestring);
        (*funcNames)[valid] = strdup(fname->valuestring);
        (*funcArgs)[valid] = strdup(fargs->valuestring);
        valid++;
    }

    *count = valid;
    cJSON_Delete(root);
    return 1;
}

int responseParseToolCallsSimple(const char *rawResponse, ToolCall **tool_calls, int *count) {
    if (!rawResponse || !tool_calls || !count) return 0;

    *tool_calls = NULL;
    *count = 0;

    cJSON *root = cJSON_Parse(rawResponse);
    if (!root) return 0;

    cJSON *choices = cJSON_GetObjectItemCaseSensitive(root, "choices");
    if (!cJSON_IsArray(choices) || cJSON_GetArraySize(choices) == 0) {
        cJSON_Delete(root);
        return 0;
    }

    cJSON *first = cJSON_GetArrayItem(choices, 0);
    cJSON *message = cJSON_GetObjectItemCaseSensitive(first, "message");
    if (!message) { cJSON_Delete(root); return 0; }

    cJSON *tc_json = cJSON_GetObjectItemCaseSensitive(message, "tool_calls");
    if (!cJSON_IsArray(tc_json)) { cJSON_Delete(root); return 0; }

    int tc = cJSON_GetArraySize(tc_json);
    if (tc == 0) { cJSON_Delete(root); return 0; }

    *tool_calls = (ToolCall *)malloc(sizeof(ToolCall) * (size_t)tc);
    if (!*tool_calls) { cJSON_Delete(root); return 0; }

    memset(*tool_calls, 0, sizeof(ToolCall) * (size_t)tc);

    for (int i = 0; i < tc; i++) {
        cJSON *item = cJSON_GetArrayItem(tc_json, i);
        cJSON *id = cJSON_GetObjectItemCaseSensitive(item, "id");
        cJSON *type = cJSON_GetObjectItemCaseSensitive(item, "type");
        cJSON *func = cJSON_GetObjectItemCaseSensitive(item, "function");
        cJSON *fn = func ? cJSON_GetObjectItemCaseSensitive(func, "name") : NULL;
        cJSON *fa = func ? cJSON_GetObjectItemCaseSensitive(func, "arguments") : NULL;

        if (!cJSON_IsString(id) || !cJSON_IsString(type) || !cJSON_IsString(fn) || !cJSON_IsString(fa)) {
            for (int j = 0; j < i; j++) {
                free((*tool_calls)[j].id);
                free((*tool_calls)[j].type);
                free((*tool_calls)[j].function_name);
                free((*tool_calls)[j].function_arguments);
            }
            free(*tool_calls);
            *tool_calls = NULL;
            cJSON_Delete(root);
            return 0;
        }

        (*tool_calls)[i].id = strdup(id->valuestring);
        (*tool_calls)[i].type = strdup(type->valuestring);
        (*tool_calls)[i].function_name = strdup(fn->valuestring);
        (*tool_calls)[i].function_arguments = strdup(fa->valuestring);
    }

    *count = tc;
    cJSON_Delete(root);
    return 1;
}

void freeParsedToolCalls(char **ids, char **types, char **funcNames, char **funcArgs, int count) {
    if (!ids) return;
    for (int i = 0; i < count; i++) {
        free(ids[i]);
        free(types[i]);
        free(funcNames[i]);
        free(funcArgs[i]);
    }
    free(ids);
    free(types);
    free(funcNames);
    free(funcArgs);
}
