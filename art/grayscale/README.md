# Archived grayscale sprites

This directory preserves the original 4-shade PNGs that were the inputs to the
1bpp placeholder conversion on 2026-10-04. It mirrors the paths in `images/`;
SHA-256 hashes for every archived PNG and its pre-conversion source are recorded
in the `CreatureGathererFX-b2a.3` section of `../../output.md`.

The archived files are the source snapshot for this conversion. A Git commit ID
was not recorded in this session; the per-file hashes identify the exact bytes.

## Restore 4-shade inputs

From the repository root, copy the originals back to their input locations,
change the top-level `shades = 2` in `fxsprites.toml` to `shades = 4`, then run
`make gen` to regenerate the 4-shade sprite arrays and FX image:

```sh
cp -f art/grayscale/images/*.png images/
cp -f art/grayscale/images/battleEffects/*.png images/battleEffects/
# Set the top-level shades value in fxsprites.toml to 4, then:
make gen
```

Keep this archive unchanged. It contains 26 PNGs referenced by `fxsprites.toml`;
unlisted source images are not included.
