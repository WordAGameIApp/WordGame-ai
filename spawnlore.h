#ifndef SPAWNLORE_H
#define SPAWNLORE_H

#include "ai_settings.h"
#include "context.h"

typedef struct {
    char *content;
    char *reasoningContent;
    int success;
} SpawnLoreResult;

void spawnLoreResultInit(SpawnLoreResult *result);
void spawnLoreResultFree(SpawnLoreResult *result);

SpawnLoreResult* spawnLore(const char *promptContent, const char *additional, AiSettings *settings);

#endif
