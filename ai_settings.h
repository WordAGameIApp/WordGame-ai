#ifndef AI_SETTINGS_H
#define AI_SETTINGS_H

#include "config.h"

typedef struct {
    char *provider;
    char *url;
    char *apiKey;
    char *model;
} ModelToken;

typedef struct {
    ModelToken modelToken;
    GenerationParams params;
    int enableTools;
} AiSettings;

void modelTokenInit(ModelToken *token);
void modelTokenFree(ModelToken *token);
void aiSettingsInit(AiSettings *settings);
int aiSettingsLoad(AiSettings *settings, const char *configFile);
int aiSettingsLoadWithEnv(AiSettings *settings, const char *configFile);
void aiSettingsFree(AiSettings *settings);
int aiSettingsValidate(const AiSettings *settings);

#endif
