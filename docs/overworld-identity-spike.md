# Overworld visual identity spike

[Open the comparison viewer](overworld-identity-spike.html). It shows cave,
forest, rural town and city scenes at the exact 128×64 viewport size, with the
current 16×16 player sprite composited at its runtime position. An optional
overlay marks the 16×16 cell grid.

The viewer has three independent art directions. Each covers the same four
areas and is converted with the same three monochrome treatments:

| Art direction | Visual approach |
| --- | --- |
| Layered storybook | Clustered natural forms and broad grayscale shading |
| Chunky silhouettes | Larger shapes, calm ground and simplified landmarks |
| Hand-inked field guide | Bold contours and grouped ink-like shadows |

Within a direction, the same grayscale source and scene crop feed every
treatment, so texture and contrast stay easy to compare:

| Treatment | Conversion | Intent |
| --- | --- | --- |
| Clean | Threshold at 128, no dithering | Strong silhouettes and open ground |
| Light | 2×2 Bayer threshold, amplitude 72 | A little texture in midtones |
| Heavy | 4×4 Bayer threshold, amplitude 104 | Richer tone, with more visible pattern |

All generated concept sheets and their exact prompts are preserved in
`assets/overworld-identity-spike/`. The reproducible exporter reduces each
four-panel sheet to 128×64 with area-weighted sampling, applies each monochrome
treatment, then composites the original down-facing player sprite at `(56,24)`.
The player center sits at `(64,32)`, matching the current renderer. All 36
viewports and the three 2× contact sheets can be rebuilt with:

```sh
node tools/generate-overworld-identity-spike.mjs
```

This is a review-only exploration. It does not replace installed tiles, alter
maps, or change firmware. The existing environment/tile exploration remains
available in [the 32×32 environment spike](world-environment-32-spike.md) and
[its viewer](world-environment-32-spike.html).

The follow-up exploration assembles four viewport studies from one indexed
16×16 tile atlas. See [Overworld reusable tileset · round 2](overworld-tileset-round2.html).
