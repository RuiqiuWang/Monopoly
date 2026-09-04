# Item Effects

```c
ItemEffectMoveResult Move_Player_With_Item_Effects(
    Player *player, Map *map, int step, ItemEffectReport *report);
```

- 有效移动路径长度为 `step % 70`。
- 按路径顺序检查路障，第一个路障使玩家停在该格并随后消失。
- 路障格仍会继续执行矿地、房屋和地产等落地事件。
- `step 0` 与整圈步数的移动路径为空，不触发路径上的路障，但会执行当前位置的落地事件。
- 炸弹和住院回合已删除。
