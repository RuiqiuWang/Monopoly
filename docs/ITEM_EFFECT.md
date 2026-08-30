# Item Effects

The item effect module applies barriers and bombs while a player moves.

## Public API

```c
ItemEffectMoveResult Move_Player_With_Item_Effects(
    Player *player, Map *map, int step, ItemEffectReport *report);

ItemEffectTurnResult Process_Hospital_Turn(Player *player);
```

## Movement Rules

- Movement checks each entered block, starting with the first step.
- Only the first item encountered during one move triggers.
- An item on the starting block does not trigger immediately.
- The starting block is checked when a complete 70-step lap returns to it.
- An item at the requested endpoint triggers normally.
- Triggered items are removed without changing the block's terrain flags.

A barrier stops the player on its block. The block remains a normal landing
position, so the game engine continues its mine, room, and property handling.

A bomb stops movement, removes the bomb, and moves the player to the hospital.
The bomb block's landing event is skipped, and an item at the hospital does not
chain-trigger during the transfer.

## Hospital Turns

`Process_Hospital_Turn` is called at the start of the affected player's own
turn. Three calls return `ITEM_EFFECT_TURN_SKIPPED`; the fourth returns
`ITEM_EFFECT_TURN_READY`. Other players' turns do not change this counter.

If a bomb is found but the map has no hospital, movement fails atomically: the
player and item remain unchanged.
