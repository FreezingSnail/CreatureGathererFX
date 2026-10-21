# Vendored ATMlib2FX compact music runtime

Pinned upstream: `arduboy/ATMlib2FX` commit
`c3d5e7c15fa570d829af860231dbf2b5c8b1bcee`.

Only the files below are copied. `atm_fx_stream.*`, `ATMlib.cpp` and
`ATMlib.h` are deliberately absent: the selected engine is the compact
directly-consumed FX backend, and including the legacy ABI would add unused
code and SRAM.

| file | upstream sha256 |
| --- | --- |
| `atm_synth.c` | `bf8b06372529f6014b3720e8ba7c316abe22ab9d00cf0574ea0552097628d1aa` |
| `atm_synth.h` | `cabfaae4a8ed008f51974115c29ac5606209037cea67158eb1c5292346b3bae6` |
| `osc.c` | `ff9609e49125225d5ebddb7a39a7014bfae0dbfd10d6627b8f165092f40ebc34` |
| `osc.h` | `aabb338655bd64ef94d1478cb540a25cb5faa25478f3921031ca58715e2a09a9` |
| `cmd_parse.c` | `e8271ec37bdb5fca07521028463a3761e67752f809d78a186e3ec55f05e9e0b4` |
| `atm_cmd_constants.h` | `79321d8aee14d3f78082dc8e3f4d7c4074cf10aa47087f15f9fd265c44845f6f` |
| `atm_fx_compact.c` | `807e4165e3b4833c1bd9ddf69f2f7db15074da624a47df7e4ef9c5052130c9ba` |
| `atm_fx_compact.h` | `c540f00688eba7c59fc9874043087b9f5fc645bf9233c6b36bb77426b0da227f` |
| `LICENSE` (BSD-3-Clause) | `1dd0bcd2d05368d3182cff7b3f1c5e81768ee0db6c15ae3eead3539a65c48a64` |

## Local modifications

`atm_synth.c` and `osc.c` gained an outer `#if CGFX_AUDIO` guard so the AUDIO=0
build compiles them to empty objects. `cmd_parse.c` and `atm_fx_compact.c`
already self-guard (`ATM_PATTERN_STACK_DEPTH` / `ATM_FX_COMPACT_IMPL`, defined
only while `atm_synth.c` includes them), so they stay upstream-identical and
compile empty standalone. No engine logic was changed.

## Build flags

`AUDIO=1` defines `CGFX_AUDIO=1`, `ATM_HAS_FX_COMPACT=1`,
`ATM_FX_COMPACT_LOOP=1`, `ATM_FX_COMPACT_LOOP_STREAMING=1` and
`ATM_FX_COMPACT_RING=64` for both C and C++ translation units. The ring and
loop flags must match `data/music/README.md` (`--ring 64 --streaming-loops`).
