# Monopoly
# 开发人员请以/function_name/details开分支，后续统一merge
# 测试人员请以issue的形式提交测试的结果

## Integrated startup flow

The main executable now uses this sequence:

1. Select 2-4 characters (`1=Q`, `2=A`, `3=S`, `4=J`).
2. On the first run, optionally open the tutorial.
3. Enter the game TUI and command loop.

An empty command line rolls for the current player. Existing commands such as
`step`, `query`, `sell`, `block`, `bomb`, `robot`, `help`, and `quit` remain
available.

All interactive runtime modules use the shared interface declared in
`include/input.h`. The TUI remains responsible for rendering the board,
current player, focused property, bankruptcy, and winner state.
