# Sharing VM dialog and battle text rendering

Research: CreatureGathererFX-7xq, 2026-10-06, baseline commit df3d670.

## Existing paths

`ScriptVM.cpp` handles Msg/MsgIf opcodes by queuing a `SCRIPT_TEXT` dialog.
It does not rasterize text. `DialogMenu.cpp` reads the indexed, length-prefixed
ASCII FX text into a resident 36-byte buffer when preparing the queue head.
Drawing uses that SRAM buffer, eighteen columns, two rows, six-pixel character
advance and eight-pixel row advance. The current authored map text table is
empty; the existing device dialog suite only exercises an invalid index and
does not verify nonempty glyph rendering.

`BattlePresenter.cpp` reads fixed captions from PROGMEM and renders decimal
damage digits directly. These use the same trimmed 5x6 font with six-pixel
advance. Creature/move names, effectiveness labels and several other fixed
labels remain pre-rendered FX rasters. Battle stage timing, before/after HP,
prepared assets and borrowed action result are independent of the dialog queue.

Both glyph paths use `Blit::draw`. Its input is headerless pixels; the native
`fontTrimmed` image has a four-byte width/height header, so glyph pixels start
at `fontTrimmed + 4`. Battle handles this correctly. Script dialogs still need
the correction tracked by CreatureGathererFX-3ds. The font supports ASCII
'0' through 'z'; spaces are skipped while advancing, and earlier punctuation
such as '!' needs a different font or an explicit future glyph mapping.

## Whole-image measurements

All variants used `make ram BUILD_DIR=build/text-sharing-research/<variant>`
with the same installed tools and shipping LTO flags. Prototypes were restored
after measuring; no production refactor is included in this research commit.

| Variant | Shipping flash | Static RAM | Flash change |
| --- | ---: | ---: | ---: |
| Current baseline | 28,050 B | 1,841 B | 0 |
| Dialog header correction alone | 28,050 B | 1,841 B | 0 |
| Shared glyph primitive | 27,978 B | 1,841 B | -72 B |
| Shared glyph primitive and text loop | 28,038 B | 1,841 B | -12 B |

The shared primitive prototype used this interface in `src/lib/Text.hpp/.cpp`:

```cpp
void drawGlyph(int16_t x, int16_t y, uint8_t character, uint8_t mode);
```

It rejects characters outside '0'..'z', then draws a 5x6 glyph at
`fontTrimmed + 4`, frame `character - '0'`, with the caller's `Blit` polarity.
The dialog SRAM loop and battle PROGMEM loop remain in their owners; damage
digits also call the primitive. It needs no new buffers or persistent state.

The larger prototype unified bounded text iteration with a SRAM/PROGMEM source
flag, optional column wrapping and a NUL stop. Its common loop replaced dialog
division/modulo for wrapping with a column counter. Even so, the full shipping
image saved 60 B less than the smaller primitive. This is measured output under
LTO, not a per-function estimate.

Both prototypes passed the existing presentation suite (210/0) and test_stack
(4/0). The primitive's presenter painted/effective stack reserve was 324/255 B
and caption reserve 371/302 B, versus the baseline's 324/255 B and 369/300 B.
The larger loop measured 320/251 B and 367/298 B. General test_stack headroom
was 335 B for the primitive and 340 B for the larger loop. Effective figures
subtract the existing 69 B USB ISR allowance. These are focused prototype
checks; implementation still needs independent nonempty script-text tests.

## Recommendation and larger RAM opportunity

Extract the validated glyph primitive (CreatureGathererFX-n11, after the
script-font fix CreatureGathererFX-3ds). Preserve caller-specific layout and
all preparation boundaries. This consolidates the address/bounds logic that
caused the recent garbling, with a measured modest flash saving. It does not
reduce the existing dialog queue, script buffer or battle payload. Keep battle
resolution/presentation timing outside VM and dialog FIFO dispatch, as required
by the frozen battle contract. Raster glyph streaming is already allowed;
text metadata and payload preparation must stay on transitions.

For RAM, the world dialog object is 139 B in the shipping symbol report:
six 17-byte `PopUpDialog` entries, one count byte, and 36 cached text bytes.
Production only queues TEXT and SCRIPT_TEXT. The old battle fields
`detailAddress` (3 B), `damage` (2 B), and `animation` (3 B) have no rendering
consumers. Removing them and reducing `DialogType` from a two-byte AVR enum to
`uint8_t` would make an entry eight bytes and structurally save 54 B across six
entries. This is an unmeasured estimate, tracked by CreatureGathererFX-a4b for
a separate whole-image spike. Preserve queue capacity and type IDs in that
slice; it is independent of glyph sharing.

Replacing all pre-rendered names/labels with ASCII, removing the text cache,
or overlaying dialog storage with battle storage requires separate data-format,
SPI and lifetime contracts. This research provides no measured savings for
those larger changes. The two existing font copies occupy FX cart storage;
deduplicating them alone would not save static SRAM or MCU application flash.

## Implementation measurements

The owner requested a gpt-6-luna worker to implement the follow-ups. The script
font correction (CreatureGathererFX-3ds), shared glyph primitive
(CreatureGathererFX-n11), and compact dialog descriptor (CreatureGathererFX-a4b)
are implemented. The larger shared text loop remains a research comparison.

| Checkpoint | Shipping flash | Static RAM |
| --- | ---: | ---: |
| Research baseline | 28,050 B | 1,841 B |
| Corrected script font offset | 28,050 B | 1,841 B |
| Shared glyph primitive | 27,978 B | 1,841 B |
| Compact dialog queue | 27,894 B | 1,787 B |
| Total saving | **156 B** | **54 B** |

The compact queue saves an additional 84 B flash and confirms the 54 B SRAM
estimate. Its descriptor is eight bytes on AVR, enforced at compile time;
DialogMenu falls from 139 to 85 B. Shipping `.data` stays at 90 B while `.bss`
falls by 54 B, so constant initializer storage does not offset the savings.
TEXT and SCRIPT_TEXT keep IDs 0 and 15, queue capacity remains six, and the
cached text buffer remains 36 bytes.

The script device suite now exercises actual SRAM text through
`DialogMenu::drawPopMenu()` and compares against the independent raw font. It
covers nonempty and empty text, full two-row wrapping, supported endpoints,
unsupported characters, spaces, digits and letter case. The existing independent
battle caption/damage regressions pass. General painted stack headroom improves
from 335 to 420 B (266 to 351 B effective after the 69 B ISR allowance).

The full integrated gate passed after each bead. Final host/world/VM counts are
155,550/0, 190/0 and 42/0; every FX suite passes, including presentation 210/0,
dialog 9/0 and arena 1,864/0. Arena callback effective reserve improves from
184 to 238 B. Generation leaves the packed assets unchanged.
