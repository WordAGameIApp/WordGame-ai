#ifndef MODEL_LIST_H
#define MODEL_LIST_H

typedef struct {
    char *name;
    char *api_key;
    char *api_type;
    char *base_url;
    char *model;
} ModelInfo;

typedef struct {
    ModelInfo *models;
    int count;
    int capacity;
} ModelList;

void model_list_init(ModelList *list);
void model_list_free(ModelList *list);
int model_list_load(ModelList *list, const char *filename);
int model_list_save(const ModelList *list, const char *filename);

ModelInfo* model_list_get(ModelList *list, const char *name);
ModelInfo* model_list_get_by_index(ModelList *list, int index);
int model_list_add(ModelList *list, const char *name, const char *api_key, const char *api_type, const char *base_url, const char *model);
void model_list_print(const ModelList *list);
int model_list_count(const ModelList *list);

#endif
