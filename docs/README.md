# Monopoly
# 开发人员请以/function_name/details开分支，后续统一merge
# 测试人员请以issue的形式提交测试的结果

## Integrated startup flow

The main executable now uses this sequence:

1. Set initial money from 1000 to 50000 (press Enter for 10000).
2. Select 2-4 characters (`1=Q`, `2=A`, `3=S`, `4=J`).
3. On the first run, optionally open the tutorial.
4. Enter the game TUI and command loop.

An empty command line rolls for the current player. Existing commands such as
`step`, `query`, `sell`, `block`, `bomb`, `robot`, `reset`, `help`, and `quit` remain
available.

`step <steps>` moves only the current player. `reset` clears the persisted play
record so the tutorial is offered again on the next launch; it does not reset
the current match.

All interactive runtime modules use the shared interface declared in
`include/input.h`. The TUI remains responsible for rendering the board,
current player, focused property, bankruptcy, and winner state.
