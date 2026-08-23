# Move_Player 接口说明

## 1. 功能概述

`Move_Player` 用于让玩家在 70 个区块组成的环形地图上向前移动。地图位置采用从 0 开始的下标：

```text
有效位置：0 ~ 69
位置 69 再前进 1 步：回到位置 0
前进 70 步：位置不变
```

函数只修改 `Player.position`，不会修改玩家的资金、道具、状态或其他字段。

## 2. 头文件和接口

使用接口前包含：

```c
#include "movement.h"
```

移动函数：

```c
PlayerMoveResult Move_Player(Player *player, int step);
```

参数：

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `player` | `Player *` | 要移动的玩家，不能为空 |
| `step` | `int` | 向前移动的步数，必须大于 0 |

返回值：

| 返回值 | 含义 |
| --- | --- |
| `PLAYER_MOVE_OK` | 移动成功 |
| `PLAYER_MOVE_INVALID_ARGUMENT` | `player` 为空 |
| `PLAYER_MOVE_INVALID_POSITION` | 移动前的位置不在 0 到 69 之间 |
| `PLAYER_MOVE_INVALID_STEP` | 步数小于等于 0 |

示例：

```c
Player player = {0};
player.id = 1;
player.position = 68;

if (Move_Player(&player, 3) == PLAYER_MOVE_OK) {
    /* player.position == 1 */
}
```

移动计算等价于：

```c
position = (position + step % MAP_BLOCK_COUNT) % MAP_BLOCK_COUNT;
```

先对步数取模，可以避免大步数参与位置计算时造成整数溢出。

## 3. 输入检查接口

由于 C 函数接收到 `int` 后无法判断原始输入是不是浮点数、字母或其他字符，文本输入必须先经过 `Parse_Step`：

```c
StepParseResult Parse_Step(const char *input, int *step);
```

`StepParseResult` 的分类如下：

| 返回值 | 含义 |
| --- | --- |
| `STEP_PARSE_OK` | 输入是合法的正整数 |
| `STEP_PARSE_INVALID_ARGUMENT` | 输入指针或输出指针为空 |
| `STEP_PARSE_INVALID_CHARACTER` | 包含字母、负号、小数点、管道符或其他非数字字符 |
| `STEP_PARSE_INVALID_STEP` | 空字符串或数值 0 |
| `STEP_PARSE_OVERFLOW` | 数值超出 `int` 的可表示范围 |

该函数只接受由十进制数字组成的正整数，例如：

```text
合法：1、5、70、2147483647
非法：0、-1、1.5、125abd、wada、90|、2147483648
```

函数拒绝空字符串、空格、正负号、小数点、字母、管道符和超出 `int` 范围的数字。CLI 会将非法字符明确提示为 `ERROR invalid character: digits only`，不再只显示笼统的错误信息。

解析成功后，再把得到的 `step` 传给 `Move_Player`。

## 4. Player 数据结构

当前版本按照项目统一定义使用 `int` 字段和 C `enum` 类型，不提前进行字段压缩：

```c
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
    int active;
    int is_winner;
} Player;
```

`Move_Player` 只依赖 `id` 所在的 `Player` 实例和其中的 `position`，实际移动时只更新 `position`。当前阶段暂不进行 `int` 到定宽整数类型的内存优化，后续可以在明确业务范围后统一评估。

## 5. 测试内容

### 单元测试

`test_movement.c` 覆盖以下内容：

1. 正整数步数可以正常移动。
2. 位置 69 前进 1 步回到 0。
3. 步数超过一圈时正确取模。
4. 负数和 0 被拒绝，位置保持不变。
5. 非法位置被拒绝。
6. 空玩家指针被拒绝。
7. 合法数字字符串可以解析。
8. 空字符串、负数、小数、字母组合、管道符和超大整数被拒绝。
9. 空输入指针和空输出指针被拒绝。

编译并运行：

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic -Iinclude movement.c tests/test_movement.c -o test_movement.exe
.\test_movement.exe
```

程序使用 `assert` 检查，退出码为 0 表示全部断言通过。

### 脚本测试

`run_movement_tests.ps1` 会自动编译 `tests/movement_cli.c`，逐条输入测试数据，并比较实际输出和预期输出。脚本覆盖 10 条输入：

```text
5
-1
1.5
125abd
wada
90|
0
69
70
2147483648
```

运行脚本：

```powershell
powershell -ExecutionPolicy Bypass -File .\run_movement_tests.ps1
```

成功时输出：

```text
PASS: 10 movement input cases
```

也可以直接运行交互式测试程序。它从标准输入逐行读取步数，成功时输出新位置，失败时输出 `ERROR invalid input`：

```powershell
gcc -std=c11 -Wall -Wextra -Wpedantic -Iinclude movement.c tests/movement_cli.c -o movement_cli.exe
@("5", "-1", "70") | .\movement_cli.exe
```

## 6. 相关文件

- `include/player.h`：玩家结构体和状态常量
- `include/movement.h`：地图大小、返回码和函数声明
- `movement.c`：移动和输入解析实现
- `tests/movement_cli.c`：标准输入测试程序
- `tests/test_movement.c`：C 单元测试
- `run_movement_tests.ps1`：自动化输入测试脚本
