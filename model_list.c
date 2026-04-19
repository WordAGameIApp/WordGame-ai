#include "model_list.h"
#include "utils.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void model_list_init(ModelList *list) {
    if (!list) return;
    list->models = NULL;
    list->count = 0;
    list->capacity = 0;
}

void model_list_free(ModelList *list) {
    if (!list) return;
    for (int i = 0; i < list->count; i++) {
        free(list->models[i].name);
        free(list->models[i].api_key);
        free(list->models[i].api_type);
        free(list->models[i].base_url);
        free(list->models[i].model);
    }
    free(list->models);
    list->models = NULL;
    list->count = 0;
    list->capacity = 0;
}

int model_list_load(ModelList *list, const char *filename) {
    if (!list || !filename) return 0;

    char *json_str = utils_read_file(filename);
    if (!json_str) {
        fprintf(stderr, "无法读取模型列表文件: %s\n", filename);
        return 0;
    }

    cJSON *root = cJSON_Parse(json_str);
    free(json_str);
    if (!root) {
        fprintf(stderr, "解析模型列表文件失败\n");
        return 0;
    }

    cJSON *model_list = cJSON_GetObjectItemCaseSensitive(root, "modelList");
    if (!cJSON_IsObject(model_list)) {
        cJSON_Delete(root);
        fprintf(stderr, "模型列表格式错误: 缺少 modelList 对象\n");
        return 0;
    }

    model_list_free(list);

    cJSON *model_entry = NULL;
    cJSON_ArrayForEach(model_entry, model_list) {
        if (!cJSON_IsObject(model_entry)) continue;

        const char *name = model_entry->string;
        cJSON *api_key = cJSON_GetObjectItemCaseSensitive(model_entry, "api_key");
        cJSON *api_type = cJSON_GetObjectItemCaseSensitive(model_entry, "api_type");
        cJSON *base_url = cJSON_GetObjectItemCaseSensitive(model_entry, "base_url");
        cJSON *model = cJSON_GetObjectItemCaseSensitive(model_entry, "model");

        if (name && cJSON_IsString(api_key) && cJSON_IsString(api_type) &&
            cJSON_IsString(base_url) && cJSON_IsString(model)) {
            model_list_add(list, name, api_key->valuestring, api_type->valuestring,
                          base_url->valuestring, model->valuestring);
        }
    }

    cJSON_Delete(root);
    return list->count > 0;
}

int model_list_save(const ModelList *list, const char *filename) {
    if (!list || !filename) return 0;

    cJSON *root = cJSON_CreateObject();
    cJSON *model_list = cJSON_CreateObject();

    for (int i = 0; i < list->count; i++) {
        cJSON *model_entry = cJSON_CreateObject();
        cJSON_AddStringToObject(model_entry, "api_key", list->models[i].api_key ? list->models[i].api_key : "");
        cJSON_AddStringToObject(model_entry, "api_type", list->models[i].api_type ? list->models[i].api_type : "openai");
        cJSON_AddStringToObject(model_entry, "base_url", list->models[i].base_url ? list->models[i].base_url : "");
        cJSON_AddStringToObject(model_entry, "model", list->models[i].model ? list->models[i].model : "");
        cJSON_AddItemToObject(model_list, list->models[i].name, model_entry);
    }

    cJSON_AddItemToObject(root, "modelList", model_list);

    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);

    int result = utils_write_file(filename, json_str);
    free(json_str);
    return result;
}

ModelInfo* model_list_get(ModelList *list, const char *name) {
    if (!list || !name) return NULL;
    for (int i = 0; i < list->count; i++) {
        if (strcmp(list->models[i].name, name) == 0) {
            return &list->models[i];
        }
    }
    return NULL;
}

ModelInfo* model_list_get_by_index(ModelList *list, int index) {
    if (!list || index < 0 || index >= list->count) return NULL;
    return &list->models[index];
}

int model_list_add(ModelList *list, const char *name, const char *api_key, const char *api_type, const char *base_url, const char *model) {
    if (!list || !name) return 0;

    if (list->count >= list->capacity) {
        int new_capacity = list->capacity == 0 ? 10 : list->capacity * 2;
        ModelInfo *new_models = (ModelInfo *)realloc(list->models, sizeof(ModelInfo) * new_capacity);
        if (!new_models) return 0;
        list->models = new_models;
        list->capacity = new_capacity;
    }

    ModelInfo *info = &list->models[list->count];
    info->name = strdup(name);
    info->api_key = api_key ? strdup(api_key) : NULL;
    info->api_type = api_type ? strdup(api_type) : strdup("openai");
    info->base_url = base_url ? strdup(base_url) : NULL;
    info->model = model ? strdup(model) : strdup(name);

    list->count++;
    return 1;
}

void model_list_print(const ModelList *list) {
    if (!list) return;
    printf("\n========== 可用模型列表 ==========\n");
    for (int i = 0; i < list->count; i++) {
        printf("%d. %s\n", i + 1, list->models[i].name);
        printf("   模型: %s\n", list->models[i].model);
        printf("   API类型: %s\n", list->models[i].api_type);
        printf("   URL: %s\n", list->models[i].base_url ? list->models[i].base_url : "(未设置)");
    }
    printf("==================================\n");
}

int model_list_count(const ModelList *list) {
    if (!list) return 0;
    return list->count;
}
