# Property

The property module implements buying land, upgrading a property, collecting
toll, and selling property. Player IDs remain 1-based and the map uses owner ID
`MAP_PROPERTY_UNOWNED` (`0`) for unowned land.

An owned empty lot has level 0. Buildings have levels 1 through 3.

- Buying costs the land's base price.
- Each upgrade costs the same base price.
- Toll is half of total investment: `price * (level + 1) / 2`.
- Sale proceeds are twice total investment: `price * (level + 1) * 2`.

God of Wealth is represented by `god_of_wealth_rounds`, independently of
`PlayerStatus`. Toll is waived while that counter is positive or while the
property owner is in hospital, in jail, or inactive.

The game engine prompts before buying or upgrading. Selling remains an explicit
`sell <position>` command and does not end the current turn.
