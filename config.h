#ifndef CONFIG_H
#define CONFIG_H

#include "cJSON.h"

typedef struct {
    double temperature;
    double top_p;
    int max_tokens;
    double presence_penalty;
    double frequency_penalty;
    int use_default;
    int enable_thinking;
    int thinking_budget;
} GenerationParams;

typedef struct {
    char *provider;
    char *url;
    char *api_key;
    char *model;
    cJSON *tools;
    char *tool_choice;
    GenerationParams params;
    char *context_file;
} ApiConfig;

void config_init_default_params(GenerationParams *params);
int config_load(const char *filename, ApiConfig *config);
int config_load_with_env(const char *filename, ApiConfig *config);
int config_save(const char *filename, const ApiConfig *config);
void config_free(ApiConfig *config);
GenerationParams config_validate_params(const GenerationParams *params);
int config_validate(const ApiConfig *config);

#endif
