#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "context.h"

typedef struct {
    char *world_name;
    char *book_file;
    char *context_file;
    int turn_count;
    Context context;
} GameState;

void game_state_init(GameState *state, const char *world_name);
void game_state_free(GameState *state);
int game_state_save(GameState *state);
int game_state_load(GameState *state, const char *world_name);

int game_state_increment_turn(GameState *state);
int game_state_should_review(GameState *state);
void game_state_add_to_book(GameState *state, const char *title, const char *content);

#endif
