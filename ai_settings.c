#include "ai_settings.h"
#include "utils.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void modelTokenInit(ModelToken *token) {
    if (!token) return;
    memset(token, 0, sizeof(ModelToken));
}

void modelTokenFree(ModelToken *token) {
    if (!token) return;
    free(token->provider);
    free(token->url);
    free(token->apiKey);
    free(token->model);
    memset(token, 0, sizeof(ModelToken));
}

void aiSettingsInit(AiSettings *settings) {
    if (!settings) return;
    memset(settings, 0, sizeof(AiSettings));
    modelTokenInit(&settings->modelToken);
    config_init_default_params(&settings->params);
    settings->enableTools = 0;
}

int aiSettingsLoad(AiSettings *settings, const char *configFile) {
    if (!settings || !configFile) return 0;

    ApiConfig config;
    memset(&config, 0, sizeof(ApiConfig));

    if (!config_load(configFile, &config)) {
        fprintf(stderr, "无法加载配置文件: %s\n", configFile);
        return 0;
    }

    modelTokenFree(&settings->modelToken);

    settings->modelToken.provider = config.provider ? strdup(config.provider) : NULL;
    settings->modelToken.url = config.url ? strdup(config.url) : NULL;
    settings->modelToken.apiKey = config.api_key ? strdup(config.api_key) : NULL;
    settings->modelToken.model = config.model ? strdup(config.model) : NULL;

    settings->params = config.params;
    settings->enableTools = config.tools ? 1 : 0;

    config_free(&config);
    return 1;
}

int aiSettingsLoadWithEnv(AiSettings *settings, const char *configFile) {
    if (!aiSettingsLoad(settings, configFile)) return 0;

    const char *env_key = getenv("WORDGAME_API_KEY");
    if (env_key && strlen(env_key) > 0) {
        free(settings->modelToken.apiKey);
        settings->modelToken.apiKey = strdup(env_key);
    }

    const char *env_url = getenv("WORDGAME_API_URL");
    if (env_url && strlen(env_url) > 0) {
        free(settings->modelToken.url);
        settings->modelToken.url = strdup(env_url);
    }

    const char *env_model = getenv("WORDGAME_MODEL");
    if (env_model && strlen(env_model) > 0) {
        free(settings->modelToken.model);
        settings->modelToken.model = strdup(env_model);
    }

    return 1;
}

void aiSettingsFree(AiSettings *settings) {
    if (!settings) return;
    modelTokenFree(&settings->modelToken);
    memset(settings, 0, sizeof(AiSettings));
}

int aiSettingsValidate(const AiSettings *settings) {
    if (!settings) return 0;
    if (!settings->modelToken.url || strlen(settings->modelToken.url) == 0) {
        fprintf(stderr, "AiSettings验证失败: 缺少 URL\n");
        return 0;
    }
    if (!settings->modelToken.apiKey || strlen(settings->modelToken.apiKey) == 0) {
        fprintf(stderr, "AiSettings验证失败: 缺少 API Key\n");
        return 0;
    }
    if (!settings->modelToken.model || strlen(settings->modelToken.model) == 0) {
        fprintf(stderr, "AiSettings验证失败: 缺少 Model\n");
        return 0;
    }
    return 1;
}
