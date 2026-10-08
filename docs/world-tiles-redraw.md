# Environment tile redraw — 2026-10-07

[Open the tile comparison](world-tiles-redraw.html) or
[the tile asset page](world-tile-assets.html).

Reference: `art/grayscale/images/tiles_16x16.png`, a 256×768 atlas with
16 columns × 48 rows of 16×16 tiles. Its four indexed shades are black,
85-gray, 170-gray and white.

The previous v3 preview was 724×2172. Its nominal tile width was 45.25 pixels:
it did not respect the native grid or guarantee dithering at game-pixel scale.

The new environment-clarity pass is
`assets/world-tiles/tiles-bw-redraw-v4-native.png`: 256×768, opaque black/white,
with 16×16 review cells. The comparison displays exact 1×, 2× or 4× pixels and
an optional tile-boundary overlay. The generated concept is preserved separately
as `tiles-bw-redraw-v4-concept.png`. Native export uses centered nearest-neighbor
sampling and a strict binary threshold; it does not add ordered dithering.

Visual targets: sparse tufts for grass, quiet light paths, dark water with
horizontal ripples, bold connected tree canopies, angular rock faces, clear
brick seams and readable wooden rails. Shape and texture distinguish materials
instead of applying the same noisy gray pattern everywhere. Prompt and export
details are in `assets/world-tiles/prompts.json`.

Native dimensions are verified; exact source tile-ID correspondence, multi-tile
seams and obstacle edges are not certified. The generator can shift shapes;
resizing cannot repair that. This remains review artwork pending per-tile
alignment and seam checks. No installed game atlas, firmware or FX cart changed.

## Second variant: creature style

`assets/world-tiles/tiles-bw-redraw-v5-native.png` is a separate 256×768 binary
review export. v4 concept/native files are preserved unchanged. Built-in imagegen
used the grayscale layout, v4, and actual original/expansion creature contact
sheets (`docs/assets/battle-sprites48/contact-{1,5}-4x.png`). The style target is
broad white shapes, bold black cutouts, rounded organic forms and sparse detail.
Raw output is preserved as `tiles-bw-redraw-v5-concept.png`; export uses the same
centered nearest-neighbor sampling and binary threshold as v4. Compare both
variants in the HTML viewer. Native size is checked; tile IDs and seams remain
uncertified. No installed game assets were replaced.
