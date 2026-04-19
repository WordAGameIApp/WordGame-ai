#include "game_state.h"
#include "utils.h"
#include "book_storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void game_state_init(GameState *state, const char *world_name) {
    if (!state || !world_name) return;
    
    state->world_name = strdup(world_name);
    
    char book_path[256];
    snprintf(book_path, sizeof(book_path), "book/%s.book", world_name);
    state->book_file = strdup(book_path);
    
    char context_path[256];
    snprintf(context_path, sizeof(context_path), "context/%s.json", world_name);
    state->context_file = strdup(context_path);
    
    state->turn_count = 0;
    context_init(&state->context, 100);
}

void game_state_free(GameState *state) {
    if (!state) return;
    free(state->world_name);
    free(state->book_file);
    free(state->context_file);
    context_free(&state->context);
    state->world_name = NULL;
    state->book_file = NULL;
    state->context_file = NULL;
}

int game_state_save(GameState *state) {
    if (!state) return 0;
    
    // 确保目录存在
    utils_ensure_dir("book");
    utils_ensure_dir("context");
    
    // 保存上下文
    return context_save(&state->context, state->context_file);
}

int game_state_load(GameState *state, const char *world_name) {
    if (!state || !world_name) return 0;
    
    game_state_free(state);
    game_state_init(state, world_name);
    
    // 尝试加载上下文
    if (!context_load(&state->context, state->context_file)) {
        // 如果加载失败，重新初始化上下文
        context_free(&state->context);
        context_init(&state->context, 100);
    }
    
    return 1;
}

int game_state_increment_turn(GameState *state) {
    if (!state) return 0;
    state->turn_count++;
    return state->turn_count;
}

int game_state_should_review(GameState *state) {
    if (!state) return 0;
    return state->turn_count > 0 && state->turn_count % 5 == 0;
}

void game_state_add_to_book(GameState *state, const char *title, const char *content) {
    if (!state || !content) return;
    
    utils_ensure_dir("book");
    
    FILE *fp = fopen(state->book_file, "a");
    if (!fp) return;
    
    if (title) {
        fprintf(fp, "\n========== %s ==========\n", title);
    }
    fprintf(fp, "%s\n", content);
    fclose(fp);
}
