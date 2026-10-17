# 6. Battle user interface

| Element | Design |
|---|---|
| Pick row | One cell per mask bit (type icon, up/down matchup marker) + SWAP cell (living bench) + WAIT cell when the kit is empty; cell width = 120/count; white selection box, black strip |
| Centre strip | Round counter "R#", selected type icon (or SWAP/WAIT label) drawn left of the sprites |
| Action page | GATHER and ESCAPE rows, each present only when legal (wild only; trainers none) |
| Party page | Up to 2 living non-active members; names cached on the transition; B cancels back to the SWAP cell |
| Sprites | 48×48 front/back per species (frame = id×2 + side), drawn with plus-mask |
| HUD | HP bars both sides; duel shows numeric HP for both sides (3×5 digits); YOU label on the player row; foe row is numerals-only above the bar |
| Team pips | 3 slots per side: 2×3 alive cell, 2×1 underline for the active member |
| Type badges | White 12×12 frames with 8×8 type icons on each sprite (foe bottom-left, you bottom-right), folded dual types deduped |
| Feedback footer | Both revealed move icons + "vs" caption, then the outcome caption and outgoing damage figures |
| Captions | BONUS WIN/LOSE, WEAK WIN/LOSE, RPS WIN/LOSE, TRADE, DEFAULT WIN, SWAP, WAIT, DRAW, GATHER progress, GATHERED, FLED, ESCAPED |
| Colors | Black strips with white text; the selected cell inverts (white fill, dark glyphs) |

Controls: directions navigate, A confirms/accelerates, B backs out of optional submenus. Trainer battles offer no GATHER or ESCAPE rows.
