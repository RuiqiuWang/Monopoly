# `step` 命令

## 格式

```text
step [id] [number]
```

- `id`：玩家编号，当前游戏为 1 到 4。
- `number`：正整数步数，玩家沿地图顺时针移动。
- 命令名称不区分大小写，例如 `STEP 1 5` 也有效。
- 输入 `quit` 退出游戏。

示例：

```text
step 1 5
玩家 A 移动 5 步，当前位置 5。
```

非法输入会提示原因，例如参数缺失、非数字、数值为 0、超出范围或参数过多。

## 构建和测试

```powershell
mingw32-make CC=gcc command_test game_engine
.\command_test.exe
```

端到端测试：

```powershell
@("N", "step 1 5", "quit") | .\game_engine.exe
```
