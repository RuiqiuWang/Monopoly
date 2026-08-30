#include "tutorial.h"
#include "input.h"
#include "tui.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define TUTORIAL_PAGE_COUNT 3

static const char *const kTutorialPages[TUTORIAL_PAGE_COUNT][3] = {
    {
        "规则 1/3：每位玩家从起点 S 出发，轮流掷骰子并按点数前进。",
        "经过或停在起点通常可以获得一笔基础奖励。",
        "玩家的位置会沿地图顺时针方向循环。"
    },
    {
        "规则 2/3：停在可购买地块时，可以支付费用购买房产。",
        "其他玩家停在你的房产上时，需要按地块规则支付费用。",
        "资金为零或触发特殊事件时，玩家可能退出游戏。"
    },
    {
        "规则 3/3：道具屋、礼品屋和魔法屋会提供特殊效果。",
        "矿地可以带来额外收益，医院和监狱可能改变玩家状态。",
        "最终按游戏规则完成目标或成为最后仍在游戏的玩家获胜。"
    }
};

bool tutorial_state_load(TutorialState *state, const char *path)
{
    FILE *file;
    char buffer[128];
    size_t used = 0;
    int value;

    if (state == NULL || path == NULL) {
        return false;
    }

    state->has_run = false;
    file = fopen(path, "r");
    if (file == NULL) {
        return true;
    }

    while (used + 1 < sizeof(buffer) &&
           fgets(buffer + used, (int)(sizeof(buffer) - used), file) != NULL) {
        used = strlen(buffer);
    }
    {
        const char *key = strstr(buffer, "\"has_run\"");
        const char *separator = key != NULL ? strchr(key, ':') : NULL;
        if (separator != NULL && sscanf(separator + 1, " %d", &value) == 1) {
            state->has_run = value != 0;
        }
    }
    fclose(file);
    return true;
}

bool tutorial_state_save(const TutorialState *state, const char *path)
{
    FILE *file;

    if (state == NULL || path == NULL) {
        return false;
    }

    file = fopen(path, "w");
    if (file == NULL) {
        return false;
    }
    fprintf(file, "{\n  \"has_run\": %d\n}\n", state->has_run ? 1 : 0);
    fclose(file);
    return true;
}

TutorialChoice tutorial_parse_choice(const char *input)
{
    if (input == NULL) return TUTORIAL_CHOICE_INVALID;
    if (toupper((unsigned char)input[0]) == 'Y' && (input[1] == '\0' || input[1] == '\r' || input[1] == '\n')) return TUTORIAL_CHOICE_YES;
    if (toupper((unsigned char)input[0]) == 'N' && (input[1] == '\0' || input[1] == '\r' || input[1] == '\n')) return TUTORIAL_CHOICE_NO;
    return TUTORIAL_CHOICE_INVALID;
}

bool tutorial_prompt_first_run(void)
{
    char input[32];

    for (;;) {
        if (!input_read_line("是否进行新手教程？[Y/N]: ", input, sizeof(input))) {
            return false;
        }
        if (tutorial_parse_choice(input) == TUTORIAL_CHOICE_YES) {
            return true;
        }
        if (tutorial_parse_choice(input) == TUTORIAL_CHOICE_NO) {
            return false;
        }
        puts("请输入 Y 或 N。");
    }
}

void tutorial_run(const Map *map)
{
    char input[32];

    for (int page = 0; page < TUTORIAL_PAGE_COUNT; ++page) {
        puts("\033[2J\033[H");
        puts("================ MONOPOLY 新手教程 ================");
        tui_render_map(map);
        puts("");
        for (int line = 0; line < 3; ++line) {
            puts(kTutorialPages[page][line]);
        }
        puts("");
        if (page + 1 < TUTORIAL_PAGE_COUNT) {
            printf("输入区：按空格继续，输入其他字符重试：" );
        } else {
            printf("输入区：按空格结束教程并进入游戏：" );
        }

        if (!input_read_line(NULL, input, sizeof(input))) {
            return;
        }
        if (input[0] != ' ') {
            --page;
        }
    }
}
