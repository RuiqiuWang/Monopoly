# Item Usage

主动道具接口：

```c
ItemUseResult Use_Block(Player *player, Map *map, int relative_distance);
ItemUseResult Use_Robot(Player *player, Map *map);
```

库存仅包含 `ITEM_BARRIER` 和 `ITEM_ROBOT`。

- 路障可放置在相对距离 `-10～10`，地图位置自动环绕。
- 已有路障或财神的位置不能再放置路障；失败不会消耗库存。
- 机器娃娃清除玩家前方 1～10 格内的路障，不清除当前位置，也不清除财神。
- 即使没有清除任何路障，机器娃娃仍会被消耗。
