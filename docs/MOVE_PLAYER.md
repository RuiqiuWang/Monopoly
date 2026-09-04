# `Move_Player` 接口

`Move_Player(Player *player, int step)` 在 70 格环形地图上移动玩家，只修改 `Player.position`。

- 有效位置为 0～69。
- `step` 可以为 0；只有负数返回 `PLAYER_MOVE_INVALID_STEP`。
- 位置计算为 `(position + step % 70) % 70`，支持 `int` 范围内的大步数。
- `Parse_Step` 接受仅由十进制数字组成的非负整数，包括 `0`；拒绝空字符串、符号、小数、字母和溢出值。

`Player` 当前只有正常行动状态；破产通过 `active` 字段表示。医院和监狱状态已删除。
