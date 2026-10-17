# 11. Menus, modes and dialogs

| State | Meaning |
|---|---|
| `WORLD` | Overworld update/render; interactions, encounters, rest |
| `BATTLE` | Duel session + compact UI (menu/party/action pages, feedback) |
| `SAVING` | Advance-driven save state machine with on-screen status (no FX reads while flash is busy) |
| `ARENA` | Opt-in arena demo only |

- `ModeState` is a single union (battle/world-transient/arena) so the device keeps one live mode object; entering battle installs the battle RNG and builds a fresh session; exiting re-initializes the world.
- The battle menu is a small page stack (pick row → optional action page → party page) with intents: select move, select party slot, gather, escape, back, wait/pass.
- World menu + explicit SAVE + collection view are the remaining first-beta UI work (`jp8.11.23`, `jp8.11.8`); the frozen root menu is BAG / COLLECTION / SAVE, non-wrapping.
- Popups/dialogs belong to the full world VM profile only; the compact shipping profile queues no messages by design.
- The 193-byte mode state also hosts the compact battle facts (round, feedback kind, damage, revealed types) that the renderer reads directly.
