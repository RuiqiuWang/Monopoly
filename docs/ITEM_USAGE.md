# Item Usage

The item usage module implements the active use of the three purchasable items.

## Public API

```c
ItemUseResult Use_Block(Player *player, Map *map, int relative_distance);
ItemUseResult Use_Bomb(Player *player, Map *map, int relative_distance);
ItemUseResult Use_Robot(Player *player, Map *map);
```

The shared inventory indexes are defined in `include/player.h`:

```text
ITEM_BARRIER = 0
ITEM_BOMB    = 1
ITEM_ROBOT   = 2
```

Do not introduce a second item-slot mapping in a caller or test adapter.

## Placement Rules

- Barrier and bomb offsets are inclusive from `-10` through `10`.
- The board wraps across all 70 positions.
- Offset zero and special map blocks are valid targets.
- A target that already contains a barrier or bomb is rejected.
- Failed operations do not consume inventory or change the map.
- A placed item does not trigger immediately when a player already occupies its block.

## Robot Rules

- The robot clears positions one through ten ahead of the player.
- The player's current position is not cleared.
- The scan wraps around the board and clears barriers and bombs.
- A robot is consumed even when no item is found.

Command parsing and user messages remain in `command.c` and `game_engine.c`.
Movement-triggered behavior is implemented by the item effect module.
