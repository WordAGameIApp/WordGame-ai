#ifndef GAME_SETTINGS_H
#define GAME_SETTINGS_H

typedef struct {
    double temperature;
    double top_p;
    char *world_gen_model;    // 从 modelList.json 中选择的模型名称
    char *main_plot_model;    // 从 modelList.json 中选择的模型名称
    char *review_model;       // 从 modelList.json 中选择的模型名称
} GameSettings;

void game_settings_init(GameSettings *settings);
int game_settings_load(GameSettings *settings, const char *filename);
int game_settings_save(const GameSettings *settings, const char *filename);
void game_settings_free(GameSettings *settings);

void game_settings_print(const GameSettings *settings);
int game_settings_edit(GameSettings *settings);

// 模型选择功能
int game_settings_select_model(const char *model_list_file, char **selected_model);

#endif
