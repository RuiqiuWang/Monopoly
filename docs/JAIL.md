# Jail

Landing exactly on the jail block sets `PLAYER_JAIL` with two detention rounds.
Passing the block does not trigger jail. The arrival turn does not consume a
detention round; the player's next two own turns are skipped.

The public API is:

```c
JailCheckResult Check_Player_in_Jail(Player *player, const Map *map);
JailTurnResult Process_Jail_Turn(Player *player);
```

`Check_Player_in_Jail` runs after movement only when normal landing events are
allowed. A bomb therefore sends the player to hospital and skips the jail check.
A barrier that stops a player on jail still permits the jail check.

`Process_Jail_Turn` runs at the beginning of each player's own turn. The first
two calls after entry return `JAIL_TURN_SKIPPED`; the following call returns
`JAIL_TURN_READY`.
