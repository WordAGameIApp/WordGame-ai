#ifndef GAME_AI_H
#define GAME_AI_H

#include "game_settings.h"
#include "game_state.h"
#include "context.h"

// 生成世界设定
int ai_generate_world(const GameSettings *settings, const char *world_name, 
                      const char *description, char **world_content);

// 生成剧情
int ai_generate_plot(const GameSettings *settings, GameState *state, 
                     const char *user_input, char **plot_content);

// 审查剧情
int ai_review_plot(const GameSettings *settings, GameState *state, char **review_feedback);

// 获取模型配置
int ai_get_model_config(const char *model_list_file, const char *model_name,
                        char **api_key, char **api_type, char **base_url, char **model);

#endif
