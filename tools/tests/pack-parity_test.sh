#!/bin/sh
# Verify the hand-expanded migration fixture and expand fixture keep the exact
# cgfx-core field ABI, then preserve the committed FX image baseline.
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd -P)
cd "$root"
fixtures=$root/tools/tests/fixtures
old_layout=$fixtures/fxlayout-old-304.toml
new_layout=$fixtures/fxlayout-expand-20.toml
perturbed_layout=$fixtures/fxlayout-expand-20-perturbed.toml
# Baseline includes the eight-sector save region declared by fxlayout.toml.
# Restored TSX-authored walkability; GIDs, field addresses and save bytes agree
# with the prior baseline. Semantic map checks run in test-generated-libs.
# Utility prototype adds move names, kits and trainer basic attacks.
# Runtime/authored matchup chart reconciliation updates the packed lookup table.
# Arena demo appends 3 player records, 5 opponent previews, and 8 paired labels.
# Arena UI appends three fixed bitmap labels after the existing text symbols.
# Battle labels follow the four authored burst renames (five extra pixels each).
# Expansion roster adds 32 creature rows, names, and paired sprite frames.
# Expansion roster move slots are filled with concept- and type-matched moves.
# Append native48 battle views and compact option frames; existing addresses stay fixed.
# Reviewed left-facing front profiles are baked toward the player at screen right.
# Append twelve masked chibi player idle/walk frames; prior450constants/data stay fixed.
# Slim battle feedback appends eleven authored captions after the existing text assets.
# Overworld pocket floor carries the encounter prop; zone 0 gains five lure-biased
# tables and [1,2,3,4] lure nibbles, changing raw_map and encounters payloads.
# jp8.9 adds the pocket box/plant tiles, chunk 1 and 33 lure zones, and the
# LureText/harness strings, changing raw_map, encounters, and the text block.
# jp8.9 gate fix gives the box tile the spare prop (bit 5) so the World A hook
# reads it from the RAM property window instead of a per-press FX word read.
# jp8.10 appends the six-line lure-demo intro card strings to the text block.
# Append64 opaque lure-world render cells; all745392 prior payload bytes match the control.
# jiu: generator identity carries crate version + built source revision, so the
# generator_version stamp grows from "cgfx-tools 0.2.0" to "cgfx-tools
# 0.3.0+g<short8>" and shifts later packed addresses; intentional rebaseline.
# jp8.11.2 authors the first-beta area corridor in maps/world_map.json (walkable
# pocket floor GID 275 at (12,8) and x12..20,y9), so the raw_map payload and
# therefore fxdata.bin change; intentional rebaseline.
# le5: the native `--project` generator now emits fxdata/generated/map.bin (the
# chunked big-endian map_data field) from the authored map instead of relying on
# the stale 2025-06-22 worktree artifact, so the map_data payload and therefore
# fxdata.bin change. Previous fixture map.bin sha256 de43a603..., regenerated
# a950e357...; previous cart sha256 94f9f6c9..., regenerated 36a3bd23....
# cf7: install the approved simple redraw of the selected first-beta cells into
# images/tiles_16x16.png and images/tiles.png at their existing GIDs (see
# docs/assets/world-environment-32-spike/simple/shipping-map.json). Only the
# `tiles` symbol payload bytes change; no FX address moves and the AVR image is
# byte-identical (flash 24986 B / static SRAM 1760 B, control == candidate).
# d73i.1 spike: appended the authored duel relation chart field (duelChartFixture,
# 25 bytes) at the end of the layout; later save-region addresses shift and the
# selected AVR duel chart source reads it. Previous cart sha256 cb170b6a..., now
# 5c0d4e7d.... Intentional rebaseline for the measured FX-chart duel variant.
# d73i.2 spike: appended the compact duel UI icon sheet (duelIcons, 14 masked
# frames) and the authored DuelText captions to the live layout; existing field
# addresses stay fixed except those after the text block, and the fixtures
# exclude the new DuelText symbols to keep the 304-declaration migration proof.
# Previous cart sha256 5c0d4e7d..., now ff695246.... Intentional rebaseline.
# d73i.4 spike: `make gen` with the pinned duel-capable native generator appends
# the folded duel content expand entry (fxdata/generated/duel.txt: per-move/
# species/retype folded type tables, beta trainer persona/rate/overrides and the
# demo sample teams) and stamps the generator identity as 0.3.0+g543e2c22, so the
# packed payload and later symbol addresses shift. Previous cart sha256
# ff695246..., now 69dede79.... Intentional rebaseline; `make gen` leaves
# src/fxdata.h, fxdata/ and tst/fxdatatest/generated/ byte-identical to HEAD.
# 4w89.2: the packed moveList/moveNames/move_table fields and duelMoveTypes are
# retired; creatureData drops 3 bytes/row (14->11) for the folded attackMask, the
# per-creature AttackMasks table replaces movelists, and the opponents payload
# shrinks to three-byte <BBB rows. The generator identity is 0.3.0+ge3d84b31.
# Previous cart sha256 69dede79..., now 518f4d84.... Intentional rebaseline.
# The permanent layout-equivalence fixtures now reference vendored copies of the
# retired moves.txt/movelists.txt/moves.bin beside this script.
# jp8.11.21: the appends GiveItem opcode 9 generator (sibling HEAD 94c24be,
# binary sha256 87918fe2...) stamps generator identity 0.3.0+g94c24bef instead
# of 0.3.0+ge3d84b31. Only the 27-byte generator_version payload changes (the
# identity string length is unchanged, so no field address moves); every other
# packed byte is identical. Verified by re-packing with the prior identity
# string, which reproduces 518f4d84.... Previous cart sha256 518f4d84..., now
# fe067475.... Intentional rebaseline.
# jp8.11.11: the native generator now derives the explicit first-stage evolution
# mapping from data/json/creatures.json `evolves_to` targets and appends the
# `Evolution` expand entry (fxdata/generated/evolution.txt: evolutionSource
# {0,3,6}, evolutionTarget {1,4,7}, evolutionLevel {8,8,8}, 9 bytes) after the
# duel content, so every earlier field address stays fixed. The rebuilt tool
# (sibling HEAD a5ffdd1d, binary sha256 25d5c14a...) also stamps generator
# identity 0.3.0+ga5ffdd1d (same 27-byte length). Isolation: re-packing with the
# prior identity string but the new entry produced d051c1242938...; the prior
# baseline (no entry, prior identity) was fe067475. Previous cart sha256
# fe067475..., now fcda6f8b.... Intentional rebaseline.
# jp8.11.3: the native generator now derives the first-area material/plant/heal
# reward mapping from data/beta-area.toml and appends the `Rewards` expand entry
# (fxdata/generated/rewards.txt: materialItem/materialCount [64+64], plantItem/
# plantCount [1+1], healItem/healCount [2+2] = 134 bytes) after the evolution
# mapping. FX_DATA_BYTES grows 748732 -> 748866 and every prior field address is
# byte-identical (append-only; only FX_DATA_PAGE/FX_DATA_BYTES and the appended
# Rewards namespace change in src/fxdata.h). The rebuilt tool (sibling HEAD
# e8afd0a7, binary sha256 c80df77b...) also stamps generator identity
# 0.3.0+ge8afd0a7 (same 27-byte length), which rewrites only the 27-byte
# generator_version payload. Previous cart sha256 fcda6f8b..., now
# 418ac63b.... Intentional rebaseline.
# rhs7.3: retire the five dead stat seeds. creatureData shrinks 11 -> 6 bytes
# per record (64 records, -320 B payload); the appended fields after it shift
# and the pack input fxdata/data/creatureData.txt loses the columns. The sibling
# core generator rebuilt at the current tools HEAD also stamps generator
# identity 0.3.0+g55501d10 (previously 0.3.0+ge8afd0a7), rewriting only the
# 27-byte generator_version payload. Previous cart sha256 418ac63b..., now
# 6bbcabeb.... Intentional rebaseline.
# rhs7.4: author the first-beta starter chains 1->2, 4->5, 7->8 in
# data/json/creatures.json (thresholds already authored at evelevel 16), so the
# Evolution expand entry grows from 9 to 18 bytes (evolutionSource
# {0,1,3,4,6,7}, evolutionTarget {1,2,4,5,7,8}, evolutionLevel
# {8,16,8,16,8,16}); FX_DATA_BYTES grows 748546 -> 748555 and every field after
# the Evolution entry (Rewards) shifts by +9. The tool change was test-only, so
# the generator identity stays 0.3.0+g55501d10. Previous cart sha256
# 6bbcabeb..., now f8bd14fd.... Intentional rebaseline.
# u0ez: remove the eleven superseded legacy packed sheets (creatureSprites,
# fieldBacground, fightMenu, ArduFont, worldTiles, npc, ecreatureSprites,
# arduboyFont, blankMenu, tilesheet, letters) from the first fxsprites.toml
# [[set]]; no live symbol referenced them. FX_DATA_BYTES shrinks 748555 -> 708708
# (-39847 B) and every kept symbol address shifts, so the packed payload and cart
# change. All kept images/PNGs and the [strings] ArduFont_5x6 rendering source stay.
# Previous cart sha256 f8bd14fd..., now 617a72a5.... Intentional rebaseline.
# jp8.11.5: add the LureText "lureTextFull" = "Bag Full" string (adjacent to
# lureTextGathered in both data/text/strings.txt and the fxsprites.toml [strings]
# order) so the mature-harvest full-bag feedback has a symbol. The text block
# grows by one 8-glyph string, so the packed payload and cart change and every
# later symbol address shifts. The frozen 304-declaration migration fixture adds
# "lureTextFull" to its exclude list to keep the proof at its original
# declarations. Previous cart sha256 617a72a5..., now a91840f3.... Intentional
# rebaseline.
# jp8.11.25: author the first-beta encounter, zone and warp content. fxdata/data/
# encounters.txt moves zoneDefs from the three test zones (chunks 0/1/33) to the
# four authored route chunks (1/33/65/66, lure nibbles 1..4, typeMask 0b1111) and
# replaces the 0..31 test tables with the five contract tables (base, spirit,
# water, wind, earth; floor 2 / ceil 5). maps/world_map.json replaces the
# out-of-area @test_house/@test_teleport warp fixtures with the in-bounds
# @beta_door (12,7) <-> @beta_gate (12,9) pair. No tile or property changed, so
# raw_map is byte-identical; the encounters payload, the scripts payload and the
# legacy map_data NPC markers change, so the cart changes while every symbol
# address and src/fxdata.h stay byte-identical. Previous cart sha256
# a91840f3..., now 2e56abb2.... Intentional rebaseline.
# 9ha9.1: append the eleven Creature Gatherer theme streams as named raw FX
# entries before [save] (data/music/<theme>.bin, 42584 B total at
# 0x0AD0C1..0x0B7719). Every earlier field address is byte-identical and
# FX_DATA_BYTES grows 708801 -> 751385, so only the appended payload changes
# the cart. Previous cart sha256 2e56abb2..., now d6f8daa3.... Intentional
# append-only rebaseline.
expected=d6f8daa3e53ee32e13c7a3441eeaaf595637e3278be801f090eb99982e931989

for layout in "$old_layout" "$new_layout" "$perturbed_layout"; do
    test -f "$layout" || {
        printf 'layout equivalence: missing fixture %s\n' "$layout" >&2
        exit 2
    }
done
# u0ez removed the eleven superseded legacy packed sheets, so the frozen
# migration fixture mirrors the slimmer live declaration order (370 entries).
test "$(grep -c '^\[\[entry\]\]' "$old_layout")" -eq 370 || {
    printf 'layout equivalence: old fixture must contain 370 entries\n' >&2
    exit 2
}
test "$(grep -c '^\[\[entry\]\]' "$new_layout")" -eq 20 || {
    printf 'layout equivalence: expand fixture must contain 20 entries\n' >&2
    exit 2
}

# Rebuild pack inputs before comparison; both plans below execute in this exact
# cgfx-core binary, directly over the project-relative permanent fixtures.
make --no-print-directory pack
tool=cgfx-tools
"$tool" --assert-layout-equivalent "$old_layout" "$new_layout"

# Permanent negative proof: removing one early declaration must name the first
# divergent field and publish both compared addresses rather than a hash only.
if diagnostic=$("$tool" --assert-layout-equivalent "$old_layout" "$perturbed_layout" 2>&1); then
    printf 'layout equivalence: perturbed fixture unexpectedly matched\n' >&2
    exit 1
fi
printf '%s\n' "$diagnostic" | grep -Fq 'layout equivalence: field 0 differs' || {
    printf 'layout equivalence: perturbation omitted first differing field\n%s\n' "$diagnostic" >&2
    exit 1
}
printf '%s\n' "$diagnostic" | grep -Eq 'old: name="zoneDefs".*addr=0x[0-9A-F]{6}' || {
    printf 'layout equivalence: perturbation omitted old field address\n%s\n' "$diagnostic" >&2
    exit 1
}
printf '%s\n' "$diagnostic" | grep -Eq 'new: name="opponent_seeds".*addr=0x[0-9A-F]{6}' || {
    printf 'layout equivalence: perturbation omitted new field address\n%s\n' "$diagnostic" >&2
    exit 1
}
printf '%s\n' 'layout equivalence: perturbation diagnostic: PASS'

actual=$(shasum -a 256 dist/fxdata.bin | awk '{print $1}')
test "$actual" = "$expected" || {
    printf 'pack parity: expected %s, observed %s\n' "$expected" "$actual" >&2
    exit 1
}
printf '%s\n' 'pack parity: PASS'
