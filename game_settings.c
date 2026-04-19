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
    settings->words_per_turn = 500;  // 默认每回合500字
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
    cJSON *words = cJSON_GetObjectItemCaseSensitive(root, "words_per_turn");

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
    if (cJSON_IsNumber(words)) settings->words_per_turn = words->valueint;

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
    cJSON_AddNumberToObject(root, "words_per_turn", settings->words_per_turn);

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
    printf("\n========== Game Settings ==========\n");
    printf("1. Temperature:      %.2f\n", settings->temperature);
    printf("2. Top_p:            %.2f\n", settings->top_p);
    printf("3. World Gen Model:  %s\n", settings->world_gen_model ? settings->world_gen_model : "(not set)");
    printf("4. Main Plot Model:  %s\n", settings->main_plot_model ? settings->main_plot_model : "(not set)");
    printf("5. Review Model:     %s\n", settings->review_model ? settings->review_model : "(not set)");
    printf("6. Words Per Turn:   %d\n", settings->words_per_turn);
    printf("====================================\n");
}

int game_settings_select_model(const char *model_list_file, char **selected_model) {
    if (!selected_model) return 0;

    ModelList list;
    model_list_init(&list);

    if (!model_list_load(&list, model_list_file)) {
        printf("Error: Cannot load model list from %s\n", model_list_file);
        model_list_free(&list);
        return 0;
    }

    if (model_list_count(&list) == 0) {
        printf("Error: Model list is empty\n");
        model_list_free(&list);
        return 0;
    }

    model_list_print(&list);

    printf("\nSelect model (1-%d), 0 to cancel: ", model_list_count(&list));
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
        printf("Selected model: %s\n", info->name);
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
        printf("\nSelect setting to modify (1-6), 0 to save and return: ");

        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            continue;
        }
        while (getchar() != '\n');

        switch (choice) {
            case 0:
                return 1;

            case 1:
                printf("Enter Temperature (0.0-2.0): ");
                if (scanf("%lf", &settings->temperature) == 1) {
                    if (settings->temperature < 0.0) settings->temperature = 0.0;
                    if (settings->temperature > 2.0) settings->temperature = 2.0;
                }
                while (getchar() != '\n');
                break;

            case 2:
                printf("Enter Top_p (0.0-1.0): ");
                if (scanf("%lf", &settings->top_p) == 1) {
                    if (settings->top_p < 0.0) settings->top_p = 0.0;
                    if (settings->top_p > 1.0) settings->top_p = 1.0;
                }
                while (getchar() != '\n');
                break;

            case 3:
                printf("\n=== Select World Generation Model ===\n");
                game_settings_select_model(MODEL_LIST_FILE, &settings->world_gen_model);
                break;

            case 4:
                printf("\n=== Select Main Plot Model ===\n");
                game_settings_select_model(MODEL_LIST_FILE, &settings->main_plot_model);
                break;

            case 5:
                printf("\n=== Select Review Model ===\n");
                game_settings_select_model(MODEL_LIST_FILE, &settings->review_model);
                break;

            case 6:
                printf("Enter Words Per Turn (100-2000): ");
                if (scanf("%d", &settings->words_per_turn) == 1) {
                    if (settings->words_per_turn < 100) settings->words_per_turn = 100;
                    if (settings->words_per_turn > 2000) settings->words_per_turn = 2000;
                    printf("Words per turn set to: %d\n", settings->words_per_turn);
                }
                while (getchar() != '\n');
                break;

            default:
                printf("Invalid selection\n");
                break;
        }
    }
}
