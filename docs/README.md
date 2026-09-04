# Monopoly Developer Notes

主程序启动流程：设置初始资金、选择 2～4 名角色、按需显示首次教程，然后进入地图和命令循环。

当前命令包括 `roll`、`step`、`query`、`sell`、`block`、`robot`、`reset`、`help` 和 `quit`。空输入不会自动掷骰，`query` 只查询当前玩家，`q` 和 `bomb` 已删除。

当前地图将原魔法屋、医院和监狱统一为无事件公园 `P`。财神由 `fortune.c` 管理，在第 10 回合后出现，地图寿命为 5 回合，领取后立即提供 5 个玩家行动回合的免租效果。

道具屋会在玩家进入时及每次购买后检查点数；低于最便宜道具的 30 点门槛时自动退出并说明原因。

## 模块文档

- [移动](MOVE_PLAYER.md)
- [`step` 命令](COMMAND_STEP.md)
- [道具使用](ITEM_USAGE.md)
- [移动道具效果](ITEM_EFFECT.md)
- [财神](FORTUNE.md)
- [地产](PROPERTY.md)
- [教程](TUTORIAL.md)
