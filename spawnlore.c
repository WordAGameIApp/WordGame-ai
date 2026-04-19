#include "spawnlore.h"
#include "api_client.h"
#include "api_types.h"
#include "response_parser.h"
#include "utils.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void spawnLoreResultInit(SpawnLoreResult *result) {
    if (!result) return;
    memset(result, 0, sizeof(SpawnLoreResult));
    result->success = 0;
}

void spawnLoreResultFree(SpawnLoreResult *result) {
    if (!result) return;
    free(result->content);
    free(result->reasoningContent);
    result->content = NULL;
    result->reasoningContent = NULL;
    free(result);
}

SpawnLoreResult* spawnLore(const char *promptContent, const char *additional, AiSettings *settings) {
    SpawnLoreResult *result = (SpawnLoreResult *)malloc(sizeof(SpawnLoreResult));
    if (!result) {
        fprintf(stderr, "内存分配失败\n");
        return NULL;
    }
    spawnLoreResultInit(result);

    if (!promptContent || !additional || !settings) {
        fprintf(stderr, "错误: 无效参数\n");
        result->content = strdup("错误: 无效参数");
        return result;
    }

    if (!aiSettingsValidate(settings)) {
        result->content = strdup("错误: AI设置无效");
        return result;
    }

    if (settings->modelToken.provider) {
        printf("提供商: %s\n", settings->modelToken.provider);
    }

    ApiProvider provider = api_provider_from_string(settings->modelToken.provider);

    char *part1 = utils_strdup_concat(promptContent, "\n\n用户请求：");
    char *combinedPrompt = part1 ? utils_strdup_concat(part1, additional) : NULL;
    free(part1);
    if (!combinedPrompt) {
        fprintf(stderr, "内存分配失败\n");
        result->content = strdup("错误: 内存分配失败");
        return result;
    }

    printf("=== SpawnLore 模式 ===\n");
    printf("用户请求: %s\n", additional);
    printf("提供商: %s\n", api_provider_to_string(provider));
    printf("模型: %s\n", settings->modelToken.model ? settings->modelToken.model : "unknown");
    printf("================\n\n");

    Context ctx;
    context_init(&ctx, 10);
    context_add_message(&ctx, "user", combinedPrompt, NULL);

    char *rawResponse = send_api_request_raw(
        provider,
        settings->modelToken.url,
        settings->modelToken.apiKey,
        settings->modelToken.model,
        &ctx,
        NULL,
        NULL,
        &settings->params
    );

    free(combinedPrompt);
    context_free(&ctx);

    if (rawResponse) {
        ApiResponse *resp = apiResponseParse(rawResponse);
        if (resp) {
            if (resp->hasError) {
                result->content = resp->errorMessage ? strdup(resp->errorMessage) : strdup("错误: API返回错误");
                result->success = 0;
            } else {
                if (resp->content) result->content = strdup(resp->content);
                if (resp->reasoningContent) result->reasoningContent = strdup(resp->reasoningContent);
                result->success = (resp->content != NULL);
            }
            apiResponseFree(resp);
        } else {
            result->content = strdup("错误: 响应解析失败");
            result->success = 0;
        }
        free(rawResponse);
    } else {
        result->content = strdup("错误: API请求失败");
        result->success = 0;
    }

    return result;
}
