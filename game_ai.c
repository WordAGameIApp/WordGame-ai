#include "game_ai.h"
#include "api_client.h"
#include "api_types.h"
#include "response_parser.h"
#include "model_list.h"
#include "spawnlore.h"
#include "ai_settings.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODEL_LIST_FILE "modelList.json"

int ai_get_model_config(const char *model_list_file, const char *model_name,
                        char **api_key, char **api_type, char **base_url, char **model) {
    if (!model_name || !api_key || !api_type || !base_url || !model) return 0;
    
    ModelList list;
    model_list_init(&list);
    
    if (!model_list_load(&list, model_list_file)) {
        model_list_free(&list);
        return 0;
    }
    
    ModelInfo *info = model_list_get(&list, model_name);
    if (!info) {
        model_list_free(&list);
        return 0;
    }
    
    *api_key = strdup(info->api_key);
    *api_type = strdup(info->api_type);
    *base_url = strdup(info->base_url);
    *model = strdup(info->model);
    
    model_list_free(&list);
    return 1;
}

int ai_generate_world(const GameSettings *settings, const char *world_name, 
                      const char *description, char **world_content) {
    if (!settings || !world_name || !description || !world_content) return 0;
    
    char *api_key = NULL, *api_type = NULL, *base_url = NULL, *model = NULL;
    if (!ai_get_model_config(MODEL_LIST_FILE, settings->world_gen_model, 
                             &api_key, &api_type, &base_url, &model)) {
        return 0;
    }
    
    // 构建提示词
    char prompt[4096];
    snprintf(prompt, sizeof(prompt),
             "请创建一个名为'%s'的游戏世界。\n\n"
             "基本设定：%s\n\n"
             "请详细描述这个世界，包括：\n"
             "1. 世界名称和背景故事\n"
             "2. 地理环境和主要地点\n"
             "3. 主要种族、势力或阵营\n"
             "4. 魔法系统或科技水平\n"
             "5. 当前时代的主要冲突或挑战\n"
             "6. 玩家可能扮演的角色类型\n\n"
             "请用中文回答，内容要详细、富有创意，适合作为RPG游戏的背景设定。",
             world_name, description);
    
    AiSettings aiSettings;
    aiSettingsInit(&aiSettings);
    aiSettings.modelToken.provider = api_type;
    aiSettings.modelToken.url = base_url;
    aiSettings.modelToken.apiKey = api_key;
    aiSettings.modelToken.model = model;
    aiSettings.params.temperature = settings->temperature;
    aiSettings.params.top_p = settings->top_p;
    aiSettings.params.max_tokens = 4096;
    aiSettings.params.use_default = 0;
    
    Context ctx;
    context_init(&ctx, 10);
    context_add_message(&ctx, "user", prompt, NULL);
    
    ApiProvider provider = api_provider_from_string(api_type);
    char *raw_response = send_api_request_raw(provider, base_url, api_key, model, 
                                               &ctx, NULL, NULL, &aiSettings.params);
    
    int result = 0;
    if (raw_response) {
        ApiResponse *resp = apiResponseParse(raw_response);
        if (resp && resp->content) {
            *world_content = strdup(resp->content);
            result = 1;
        }
        if (resp) apiResponseFree(resp);
        free(raw_response);
    }
    
    context_free(&ctx);
    // 注意：aiSettings 中的字符串是借用，不需要 free
    
    return result;
}

int ai_generate_plot(const GameSettings *settings, GameState *state, 
                     const char *user_input, char **plot_content) {
    if (!settings || !state || !user_input || !plot_content) return 0;
    
    char *api_key = NULL, *api_type = NULL, *base_url = NULL, *model = NULL;
    if (!ai_get_model_config(MODEL_LIST_FILE, settings->main_plot_model, 
                             &api_key, &api_type, &base_url, &model)) {
        return 0;
    }
    
    // 添加上下文
    context_add_message(&state->context, "user", user_input, NULL);
    
    AiSettings aiSettings;
    aiSettingsInit(&aiSettings);
    aiSettings.modelToken.provider = api_type;
    aiSettings.modelToken.url = base_url;
    aiSettings.modelToken.apiKey = api_key;
    aiSettings.modelToken.model = model;
    aiSettings.params.temperature = settings->temperature;
    aiSettings.params.top_p = settings->top_p;
    aiSettings.params.max_tokens = 2048;
    aiSettings.params.use_default = 0;
    
    ApiProvider provider = api_provider_from_string(api_type);
    char *raw_response = send_api_request_raw(provider, base_url, api_key, model, 
                                               &state->context, NULL, NULL, &aiSettings.params);
    
    int result = 0;
    if (raw_response) {
        ApiResponse *resp = apiResponseParse(raw_response);
        if (resp && resp->content) {
            *plot_content = strdup(resp->content);
            context_add_message(&state->context, "assistant", resp->content, NULL);
            result = 1;
        }
        if (resp) apiResponseFree(resp);
        free(raw_response);
    }
    
    return result;
}

int ai_review_plot(const GameSettings *settings, GameState *state, char **review_feedback) {
    if (!settings || !state || !review_feedback) return 0;
    
    char *api_key = NULL, *api_type = NULL, *base_url = NULL, *model = NULL;
    if (!ai_get_model_config(MODEL_LIST_FILE, settings->review_model, 
                             &api_key, &api_type, &base_url, &model)) {
        return 0;
    }
    
    // 构建审查提示词
    char *context_json = context_to_json(&state->context);
    if (!context_json) return 0;
    
    char prompt[8192];
    snprintf(prompt, sizeof(prompt),
             "请审查以下游戏剧情，检查是否有以下问题：\n"
             "1. 剧情逻辑是否连贯\n"
             "2. 角色行为是否符合设定\n"
             "3. 是否有矛盾或漏洞\n"
             "4. 剧情发展是否有趣\n\n"
             "如果发现问题，请提供具体的修改建议。如果没有问题，请回复\"剧情良好，无需修改\"。\n\n"
             "当前剧情上下文：\n%s",
             context_json);
    free(context_json);
    
    Context review_ctx;
    context_init(&review_ctx, 10);
    context_add_message(&review_ctx, "user", prompt, NULL);
    
    AiSettings aiSettings;
    aiSettingsInit(&aiSettings);
    aiSettings.modelToken.provider = api_type;
    aiSettings.modelToken.url = base_url;
    aiSettings.modelToken.apiKey = api_key;
    aiSettings.modelToken.model = model;
    aiSettings.params.temperature = 0.3;  // 审查用较低温度
    aiSettings.params.top_p = 0.9;
    aiSettings.params.max_tokens = 2048;
    aiSettings.params.use_default = 0;
    
    ApiProvider provider = api_provider_from_string(api_type);
    char *raw_response = send_api_request_raw(provider, base_url, api_key, model, 
                                               &review_ctx, NULL, NULL, &aiSettings.params);
    
    int result = 0;
    if (raw_response) {
        ApiResponse *resp = apiResponseParse(raw_response);
        if (resp && resp->content) {
            *review_feedback = strdup(resp->content);
            result = 1;
        }
        if (resp) apiResponseFree(resp);
        free(raw_response);
    }
    
    context_free(&review_ctx);
    return result;
}
