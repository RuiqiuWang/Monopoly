#ifndef MONOPOLY_PLAYER_H
#define MONOPOLY_PLAYER_H

#define NAME_LEN 5
#define ITEM_COUNT 8

enum {
    ITEM_BARRIER = 0,
    ITEM_ROBOT = 1,
    ITEM_BOMB = 2
};

// 颜色
typedef enum {
    COLOR_GREEN,
    COLOR_RED,
    COLOR_BLUE,
    COLOR_YELLOW
} PlayerColor;

// 状态
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

    int position;          // 当前地图位置
    int money;             // 资金
    int points;            // 点数

    int items[ITEM_COUNT]; // 道具数量, ITEM_COUNT 为道具种类总数

    PlayerStatus status;   // 当前状态
    int status_rounds;     // 状态剩余轮数

    int active;            // 是否仍在游戏中
    int is_winner;         // 是否获胜
} Player;

const char *status_to_string(PlayerStatus status);

#endif
