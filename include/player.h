#ifndef MONOPOLY_PLAYER_H
#define MONOPOLY_PLAYER_H

#define NAME_LEN 5
#define ITEM_COUNT 2

enum {
    ITEM_BARRIER = 0,
    ITEM_ROBOT = 1
};

typedef enum {
    COLOR_GREEN,
    COLOR_RED,
    COLOR_BLUE,
    COLOR_YELLOW
} PlayerColor;

typedef enum {
    PLAYER_NORMAL
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
    int god_of_wealth_rounds;
    int active;
    int is_winner;
} Player;

const char *status_to_string(PlayerStatus status);

#endif
