#include "config.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static double clamp_double(double value, double min, double max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

static int clamp_int(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

void config_init_default_params(GenerationParams *params) {
    if (!params) return;
    params->temperature = 0.7;
    params->top_p = 1.0;
    params->max_tokens = 1024;
    params->presence_penalty = 0.0;
    params->frequency_penalty = 0.0;
    params->use_default = 1;
    params->enable_thinking = 0;
    params->thinking_budget = 0;
}

GenerationParams config_validate_params(const GenerationParams *params) {
    GenerationParams validated = *params;
    validated.temperature = clamp_double(validated.temperature, 0.0, 2.0);
    validated.top_p = clamp_double(validated.top_p, 0.0, 1.0);
    validated.max_tokens = clamp_int(validated.max_tokens, 1, 8192);
    validated.presence_penalty = clamp_double(validated.presence_penalty, -2.0, 2.0);
    validated.frequency_penalty = clamp_double(validated.frequency_penalty, -2.0, 2.0);
    if (validated.thinking_budget > 0) {
        validated.thinking_budget = clamp_int(validated.thinking_budget, 1, 32000);
    }
    return validated;
}

int config_load(const char *filename, ApiConfig *config) {
    if (!filename || !config) return 0;

    config_init_default_params(&config->params);

    char *json_str = utils_read_file(filename);
    if (!json_str) return 0;

    cJSON *root = cJSON_Parse(json_str);
    free(json_str);

    if (!root) {
        fprintf(stderr, "JSON 解析错误: %s\n", cJSON_GetErrorPtr());
        return 0;
    }

    cJSON *provider = cJSON_GetObjectItemCaseSensitive(root, "provider");
    cJSON *url = cJSON_GetObjectItemCaseSensitive(root, "url");
    cJSON *api_key = cJSON_GetObjectItemCaseSensitive(root, "api_key");
    cJSON *model = cJSON_GetObjectItemCaseSensitive(root, "model");
    cJSON *tools = cJSON_GetObjectItemCaseSensitive(root, "tools");
    cJSON *tool_choice = cJSON_GetObjectItemCaseSensitive(root, "tool_choice");
    cJSON *context_file = cJSON_GetObjectItemCaseSensitive(root, "context_file");

    cJSON *params = cJSON_GetObjectItemCaseSensitive(root, "params");
    if (params) {
        cJSON *temperature = cJSON_GetObjectItemCaseSensitive(params, "temperature");
        cJSON *top_p = cJSON_GetObjectItemCaseSensitive(params, "top_p");
        cJSON *max_tokens = cJSON_GetObjectItemCaseSensitive(params, "max_tokens");
        cJSON *presence_penalty = cJSON_GetObjectItemCaseSensitive(params, "presence_penalty");
        cJSON *frequency_penalty = cJSON_GetObjectItemCaseSensitive(params, "frequency_penalty");
        cJSON *enable_thinking = cJSON_GetObjectItemCaseSensitive(params, "enable_thinking");
        cJSON *thinking_budget = cJSON_GetObjectItemCaseSensitive(params, "thinking_budget");

        if (cJSON_IsNumber(temperature)) config->params.temperature = temperature->valuedouble;
        if (cJSON_IsNumber(top_p)) config->params.top_p = top_p->valuedouble;
        if (cJSON_IsNumber(max_tokens)) config->params.max_tokens = max_tokens->valueint;
        if (cJSON_IsNumber(presence_penalty)) config->params.presence_penalty = presence_penalty->valuedouble;
        if (cJSON_IsNumber(frequency_penalty)) config->params.frequency_penalty = frequency_penalty->valuedouble;
        if (cJSON_IsBool(enable_thinking)) config->params.enable_thinking = cJSON_IsTrue(enable_thinking);
        if (cJSON_IsNumber(thinking_budget)) config->params.thinking_budget = thinking_budget->valueint;
        config->params.use_default = 0;
    }

    if (cJSON_IsString(provider) && provider->valuestring)
        config->provider = strdup(provider->valuestring);
    if (cJSON_IsString(url) && url->valuestring)
        config->url = strdup(url->valuestring);
    if (cJSON_IsString(api_key) && api_key->valuestring)
        config->api_key = strdup(api_key->valuestring);
    if (cJSON_IsString(model) && model->valuestring)
        config->model = strdup(model->valuestring);
    if (cJSON_IsArray(tools))
        config->tools = cJSON_Duplicate(tools, 1);
    if (cJSON_IsString(tool_choice) && tool_choice->valuestring)
        config->tool_choice = strdup(tool_choice->valuestring);
    if (cJSON_IsString(context_file) && context_file->valuestring)
        config->context_file = strdup(context_file->valuestring);

    cJSON_Delete(root);
    return 1;
}

int config_load_with_env(const char *filename, ApiConfig *config) {
    if (!config_load(filename, config)) return 0;

    const char *env_key = getenv("WORDGAME_API_KEY");
    if (env_key && strlen(env_key) > 0) {
        free(config->api_key);
        config->api_key = strdup(env_key);
    }

    const char *env_url = getenv("WORDGAME_API_URL");
    if (env_url && strlen(env_url) > 0) {
        free(config->url);
        config->url = strdup(env_url);
    }

    const char *env_model = getenv("WORDGAME_MODEL");
    if (env_model && strlen(env_model) > 0) {
        free(config->model);
        config->model = strdup(env_model);
    }

    return 1;
}

int config_save(const char *filename, const ApiConfig *config) {
    if (!filename || !config) return 0;

    cJSON *root = cJSON_CreateObject();
    if (!root) return 0;

    if (config->provider) cJSON_AddStringToObject(root, "provider", config->provider);
    if (config->url) cJSON_AddStringToObject(root, "url", config->url);
    if (config->api_key) cJSON_AddStringToObject(root, "api_key", config->api_key);
    if (config->model) cJSON_AddStringToObject(root, "model", config->model);
    if (config->tool_choice) cJSON_AddStringToObject(root, "tool_choice", config->tool_choice);
    if (config->context_file) cJSON_AddStringToObject(root, "context_file", config->context_file);
    if (config->tools) cJSON_AddItemToObject(root, "tools", cJSON_Duplicate(config->tools, 1));

    if (!config->params.use_default) {
        cJSON *params = cJSON_CreateObject();
        cJSON_AddNumberToObject(params, "temperature", config->params.temperature);
        cJSON_AddNumberToObject(params, "top_p", config->params.top_p);
        cJSON_AddNumberToObject(params, "max_tokens", config->params.max_tokens);
        if (config->params.presence_penalty != 0.0)
            cJSON_AddNumberToObject(params, "presence_penalty", config->params.presence_penalty);
        if (config->params.frequency_penalty != 0.0)
            cJSON_AddNumberToObject(params, "frequency_penalty", config->params.frequency_penalty);
        if (config->params.enable_thinking)
            cJSON_AddBoolToObject(params, "enable_thinking", 1);
        if (config->params.thinking_budget > 0)
            cJSON_AddNumberToObject(params, "thinking_budget", config->params.thinking_budget);
        cJSON_AddItemToObject(root, "params", params);
    }

    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);

    int result = utils_write_file(filename, json_str);
    free(json_str);
    return result;
}

void config_free(ApiConfig *config) {
    if (!config) return;
    free(config->provider);
    free(config->url);
    free(config->api_key);
    free(config->model);
    free(config->tool_choice);
    free(config->context_file);
    if (config->tools) cJSON_Delete(config->tools);
    memset(config, 0, sizeof(ApiConfig));
}

int config_validate(const ApiConfig *config) {
    if (!config) return 0;
    if (!config->provider || strlen(config->provider) == 0) {
        fprintf(stderr, "配置验证失败: 缺少 provider\n");
        return 0;
    }
    if (strcmp(config->provider, "openai") != 0 &&
        strcmp(config->provider, "google") != 0 &&
        strcmp(config->provider, "claude") != 0) {
        fprintf(stderr, "配置验证失败: 不支持的 provider '%s' (支持: openai, google, claude)\n",
                config->provider);
        return 0;
    }
    if (!config->url || strlen(config->url) == 0) {
        fprintf(stderr, "配置验证失败: 缺少 url\n");
        return 0;
    }
    if (!config->api_key || strlen(config->api_key) == 0) {
        fprintf(stderr, "配置验证失败: 缺少 api_key\n");
        return 0;
    }
    if (!config->model || strlen(config->model) == 0) {
        fprintf(stderr, "配置验证失败: 缺少 model\n");
        return 0;
    }
    return 1;
}
