#ifndef GAME_MENU_H
#define GAME_MENU_H

#include "game_settings.h"

typedef enum {
    MENU_NEW_GAME,
    MENU_LOAD_GAME,
    MENU_SETTINGS,
    MENU_EXIT,
    MENU_INVALID
} MenuChoice;

void game_menu_show(void);
MenuChoice game_menu_get_choice(void);
int game_menu_run(GameSettings *settings);

void game_new_game(const GameSettings *settings);
void game_load_game(const GameSettings *settings);

#endif
