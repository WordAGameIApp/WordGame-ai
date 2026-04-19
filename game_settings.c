#include "game_settings.h"
#include "model_list.h"
#include "utils.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODEL_LIST_FILE "modelList.json"

void game_settings_init(GameSettings *settings) {
    if (!settings) return;
    settings->temperature = 0.7;
    settings->top_p = 1.0;
    settings->world_gen_model = strdup("deepseek-chat");
    settings->main_plot_model = strdup("deepseek-chat");
    settings->review_model = strdup("deepseek-chat");
}

int game_settings_load(GameSettings *settings, const char *filename) {
    if (!settings || !filename) return 0;

    char *json_str = utils_read_file(filename);
    if (!json_str) {
        game_settings_init(settings);
        return 0;
    }

    cJSON *root = cJSON_Parse(json_str);
    free(json_str);
    if (!root) {
        game_settings_init(settings);
        return 0;
    }

    cJSON *temp = cJSON_GetObjectItemCaseSensitive(root, "temperature");
    cJSON *topp = cJSON_GetObjectItemCaseSensitive(root, "top_p");
    cJSON *world = cJSON_GetObjectItemCaseSensitive(root, "world_gen_model");
    cJSON *plot = cJSON_GetObjectItemCaseSensitive(root, "main_plot_model");
    cJSON *review = cJSON_GetObjectItemCaseSensitive(root, "review_model");

    if (cJSON_IsNumber(temp)) settings->temperature = temp->valuedouble;
    if (cJSON_IsNumber(topp)) settings->top_p = topp->valuedouble;
    if (cJSON_IsString(world) && world->valuestring) {
        free(settings->world_gen_model);
        settings->world_gen_model = strdup(world->valuestring);
    }
    if (cJSON_IsString(plot) && plot->valuestring) {
        free(settings->main_plot_model);
        settings->main_plot_model = strdup(plot->valuestring);
    }
    if (cJSON_IsString(review) && review->valuestring) {
        free(settings->review_model);
        settings->review_model = strdup(review->valuestring);
    }

    cJSON_Delete(root);
    return 1;
}

int game_settings_save(const GameSettings *settings, const char *filename) {
    if (!settings || !filename) return 0;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "temperature", settings->temperature);
    cJSON_AddNumberToObject(root, "top_p", settings->top_p);
    cJSON_AddStringToObject(root, "world_gen_model", settings->world_gen_model ? settings->world_gen_model : "deepseek-chat");
    cJSON_AddStringToObject(root, "main_plot_model", settings->main_plot_model ? settings->main_plot_model : "deepseek-chat");
    cJSON_AddStringToObject(root, "review_model", settings->review_model ? settings->review_model : "deepseek-chat");

    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);

    int result = utils_write_file(filename, json_str);
    free(json_str);
    return result;
}

void game_settings_free(GameSettings *settings) {
    if (!settings) return;
    free(settings->world_gen_model);
    free(settings->main_plot_model);
    free(settings->review_model);
    settings->world_gen_model = NULL;
    settings->main_plot_model = NULL;
    settings->review_model = NULL;
}

void game_settings_print(const GameSettings *settings) {
    if (!settings) return;
    printf("\n========== 游戏设置 ==========\n");
    printf("1. Temperature:     %.2f\n", settings->temperature);
    printf("2. Top_p:           %.2f\n", settings->top_p);
    printf("3. 世界生成模型:    %s\n", settings->world_gen_model ? settings->world_gen_model : "(未设置)");
    printf("4. 主剧情生成模型:  %s\n", settings->main_plot_model ? settings->main_plot_model : "(未设置)");
    printf("5. 审查模型:        %s\n", settings->review_model ? settings->review_model : "(未设置)");
    printf("==============================\n");
}

int game_settings_select_model(const char *model_list_file, char **selected_model) {
    if (!selected_model) return 0;

    ModelList list;
    model_list_init(&list);

    if (!model_list_load(&list, model_list_file)) {
        printf("错误: 无法加载模型列表文件 %s\n", model_list_file);
        model_list_free(&list);
        return 0;
    }

    if (model_list_count(&list) == 0) {
        printf("错误: 模型列表为空\n");
        model_list_free(&list);
        return 0;
    }

    model_list_print(&list);

    printf("\n请选择模型 (1-%d), 0 取消: ", model_list_count(&list));
    int choice;
    if (scanf("%d", &choice) != 1) {
        while (getchar() != '\n');
        model_list_free(&list);
        return 0;
    }
    while (getchar() != '\n');

    if (choice < 1 || choice > model_list_count(&list)) {
        model_list_free(&list);
        return 0;
    }

    ModelInfo *info = model_list_get_by_index(&list, choice - 1);
    if (info && info->name) {
        free(*selected_model);
        *selected_model = strdup(info->name);
        printf("已选择模型: %s\n", info->name);
        model_list_free(&list);
        return 1;
    }

    model_list_free(&list);
    return 0;
}

int game_settings_edit(GameSettings *settings) {
    if (!settings) return 0;

    int choice;

    while (1) {
        game_settings_print(settings);
        printf("\n选择要修改的设置 (1-5), 0 保存并返回: ");

        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            continue;
        }
        while (getchar() != '\n');

        switch (choice) {
            case 0:
                return 1;

            case 1:
                printf("输入 Temperature (0.0-2.0): ");
                if (scanf("%lf", &settings->temperature) == 1) {
                    if (settings->temperature < 0.0) settings->temperature = 0.0;
                    if (settings->temperature > 2.0) settings->temperature = 2.0;
                }
                while (getchar() != '\n');
                break;

            case 2:
                printf("输入 Top_p (0.0-1.0): ");
                if (scanf("%lf", &settings->top_p) == 1) {
                    if (settings->top_p < 0.0) settings->top_p = 0.0;
                    if (settings->top_p > 1.0) settings->top_p = 1.0;
                }
                while (getchar() != '\n');
                break;

            case 3:
                printf("\n=== 选择世界生成模型 ===\n");
                game_settings_select_model(MODEL_LIST_FILE, &settings->world_gen_model);
                break;

            case 4:
                printf("\n=== 选择主剧情生成模型 ===\n");
                game_settings_select_model(MODEL_LIST_FILE, &settings->main_plot_model);
                break;

            case 5:
                printf("\n=== 选择审查模型 ===\n");
                game_settings_select_model(MODEL_LIST_FILE, &settings->review_model);
                break;

            default:
                printf("无效选择\n");
                break;
        }
    }
}
