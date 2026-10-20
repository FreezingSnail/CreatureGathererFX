# Overworld reusable tileset spike · round 2

[Open the interactive comparison](overworld-tileset-round2.html). This second
pass assembles the viewport examples from a shared atlas rather than painting
each view as a standalone scene.

The atlas has 64 indexed cells at 16×16 pixels. The same tile IDs build cave,
forest, rural-town and city examples, each at the native 128×64 viewport size
(8×4 cells). Those four locations also join into a 16×8 tile world, which is
256×128 pixels and spans four viewports. Its Tiled map and the four viewport
maps are included for review.

| Art direction | Approach |
| --- | --- |
| Layered storybook | Softly grouped values and clustered natural texture |
| Chunky silhouettes | Larger masses, firm contours and open ground |
| Hand-inked field guide | Defined outlines and graphic shadow marks |

Each art direction is shown with three monochrome conversions from the same
grayscale tiles:

| Treatment | Conversion | Intent |
| --- | --- | --- |
| Clean | Threshold at 128 | Compare silhouettes without added dithering |
| Light | 2×2 Bayer, amplitude 72 | Add some texture in midtones |
| Heavy | 4×4 Bayer, amplitude 104 | Test denser tonal texture |

## Shared atlas layout

| IDs | Tile family |
| --- | --- |
| 0–7 | Base ground and terrain |
| 8–15 | Path segments, corners and junctions |
| 16–23 | Trees, shrubs and forest props |
| 24–31 | Cave walls, floor, entrance and minerals |
| 32–39 | Cottage pieces, fences and gardens |
| 40–47 | Shop fronts, street edges and lighting |
| 48–55 | Water, banks, bridges and stepping stones |
| 56–63 | Shared landmarks and props |

The direction sources are fresh independent tile sheets. Existing artwork
spikes were used to identify the viewport and grid constraints, but were not
provided as visual references. Original source sheets and prompt specs are in
`assets/overworld-tileset-round2/sources/`.

Every treatment exports a 128×128 atlas PNG, matching Tiled TSX, four 8×4 TMJ
maps and one 16×8 world TMJ. Tiled map paths are relative to their matching
tileset. In the viewer, a gray center line on the enlarged world preview marks
the four viewport bounds; the downloadable map does not contain that guide or
the player overlay. The viewport examples place the down-facing 16×16 player
at `(56,24)`, so its center aligns to `(64,32)`.

This is a review-only exploration; it does not replace installed game art or
alter runtime maps. The per-cell crops establish tilemap assembly and stable
IDs, but still need edge cleanup and device testing before any art is adopted.
Rebuild the rasters, Tiled exports and manifest with:

```sh
node tools/generate-overworld-tileset-round2.mjs
```

For the scene-led first round, see [the concept comparison](overworld-identity-spike.html).
The pre-existing [environment and tile spike](world-environment-32-spike.html)
also remains available.
