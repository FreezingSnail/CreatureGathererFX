# Creature Gatherer music streams

Eleven compact FX streams, one per theme, generated from the authored scores in
the sibling ATMlib2FX repository at commit `c3d5e7c`.

Each stream is a `0xC1` streaming-loop compact stream (`--ring 64
--streaming-loops`). It is packaged verbatim as a raw FX data entry
(`fxlayout.toml`, appended before `[save]`) and addressed by the generated
`music_*` symbols in `src/fxdata.h`. `MusicCatalog.hpp` and `catalog.tsv` carry
the device table and host-test metadata.

Generator command (run once per theme under the ATMlib2FX checkout):

```sh
tools/atm-music build examples/music_authoring/creature_gatherer/<theme>/song.json \
  --backend fx --ring 64 --streaming-loops
# output: build/audio-plan/<theme>/stream.bin
```

| theme | source JSON | bytes | sha256 |
| --- | --- | ---: | --- |
| overworld | `examples/music_authoring/creature_gatherer/overworld/song.json` | 6328 | `5221434212febffc5a2505ded047a50d710e7827ea7da2c0155666be31a47c39` |
| combat | `examples/music_authoring/creature_gatherer/combat/song.json` | 5772 | `b5cf74f5c0e45f6920dcb638310d1f60626fa69c8105feae60809decea83b804` |
| victory | `examples/music_authoring/creature_gatherer/victory/song.json` | 1338 | `80c2f598ba07510a88ccaa683d3f735537597a8064731921b04c86df21719587` |
| defeat | `examples/music_authoring/creature_gatherer/defeat/song.json` | 674 | `28322d6237c0c0510969fa60f12d9ecc2472ab94dea37f92c3d384679323b2ee` |
| opening | `examples/music_authoring/creature_gatherer/opening/song.json` | 7208 | `a1b520059a97a8071330db4851f9d0035432807545701bbe8ce404485c0a7f9f` |
| village | `examples/music_authoring/creature_gatherer/village/song.json` | 1660 | `4ce6bd04bbd6e25d4e2e9938a3581b8297122db80a280cd1af437892e35fe636` |
| forest | `examples/music_authoring/creature_gatherer/forest/song.json` | 6204 | `6dd4e6c9a906c8491fcabf3df2025fd57be45b6f8c9da897b581c3aab5f7bb3a` |
| coast | `examples/music_authoring/creature_gatherer/coast/song.json` | 2964 | `32c31d931fa90016fede0675fc2c671c563fbb757001309c72812c92113f3390` |
| marsh | `examples/music_authoring/creature_gatherer/marsh/song.json` | 1452 | `834ecf302a17f40861ebf41433dd8774a4c26a6d7712fafd0413ee237f5e0b76` |
| ruins | `examples/music_authoring/creature_gatherer/ruins/song.json` | 7056 | `7843fb9adc8850d44523141fe130c3cf3ecd524270095fb6896184d1c1189a15` |
| exploration | `examples/music_authoring/creature_gatherer/exploration/song.json` | 1928 | `199f137cbc75c86484431f3e881cccbb014fba7e07c44595ca8b6db135c66a2a` |

Total: 42584 B. This directory contains no runtime audio code; playback lands
with the ATMlib2FX runtime bead (`CreatureGathererFX-9ha9.2`).
