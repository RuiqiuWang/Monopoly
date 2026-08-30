#ifndef MONOPOLY_PLAYER_H
#define MONOPOLY_PLAYER_H

#define NAME_LEN 5
#define ITEM_COUNT 8

enum {
    ITEM_BARRIER = 0,
    ITEM_BOMB = 1,
    ITEM_ROBOT = 2
};

typedef enum {
    COLOR_GREEN,
    COLOR_RED,
    COLOR_BLUE,
    COLOR_YELLOW
} PlayerColor;

typedef enum {
    PLAYER_NORMAL,
    PLAYER_HOSPITAL,
    PLAYER_JAIL,
    PLAYER_GOD
} PlayerStatus;

typedef struct {
    int id;
    char name[NAME_LEN];
    PlayerColor color;
    int position;
    int money;
    int points;
    int items[ITEM_COUNT];
    PlayerStatus status;
    int status_rounds;
    int god_of_wealth_rounds;
    int active;
    int is_winner;
} Player;

const char *status_to_string(PlayerStatus status);

#endif
