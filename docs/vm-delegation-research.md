# Further VM delegation research

Research bead: CreatureGathererFX-syd. Baseline: `575911c`, 2026-10-06.
Production prototypes were restored; this report does not migrate game behavior.

## Measured opportunities

Each experiment starts from the baseline shipping image: 27,894 B flash and
1,787 B static SRAM. These are independent builds, not a measured combined image.

| Change | Flash delta | Static SRAM delta | Evidence |
| --- | ---: | ---: | --- |
| Remove unused ScriptVM memory[8] and stack[8] | 0 B | -16 B | Shipping build; VM 42/0; device scripts 32/0; stack 4/0 |
| Remove unused Animator global and per-frame play() | -150 B | -28 B | Shipping build only; behavior verification remains for implementation |

The VM object shrinks from 27 to 11 B without changing command behavior. Neither
array has a consumer. ReadFlag currently consumes operands without returning a
register result. Follow-up: **CreatureGathererFX-2ua**.

The legacy Animator has no production push/start/pop callers. Its idle playback
still ticks every frame, retaining code and a 28 B global. BattlePresenter owns
the active battle animations. Follow-up: **CreatureGathererFX-hdc**. Removing this
path is a cleanup, not a benefit of introducing more VM commands.

## Where scripts can help

| Priority | Area | Implementation boundary | Savings outlook |
| --- | --- | --- | --- |
| 1 | NPC dialogue, quest flags, doors, tutorials | Existing Msg/TMsg/SMsg, If, flag commands and teleport commands | Avoids future bespoke branches; current map already scripts its three teleport/door interactions |
| 2 | Rewards and item acquisition | GiveItem command calls the native inventory helper | Strong reusable seam; initially adds interpreter/compiler code, so net savings require representative flows |
| 3 | Trainer challenges and story encounters | Script returns a deferred battle-start intent; world caller performs transition | May consolidate authored setup; no measured saving yet |
| 4 | Shop, harvest and lure conversations | Native operations with scripted gating/order; interactive choices require yield/resume design | Future content opportunity; UI and persistent inventory/growth state remain necessary |
| 5 | Battle presentation recipes | First compare bounded native descriptors against a small command runner | Current world VM cannot run safely in battle; no established saving |

The existing **CreatureGathererFX-jp8.5.17** plans GiveItem=9 with three byte
operands and a native inventoryAdd call. Refresh its stale tool and Msg guidance
before dispatch: use the installed cgfx-tools through make gen, and update both
firmware command validation and execution. Freeze full-bag, partial-grant and
flag ordering for one-time rewards; no-op/continue semantics alone do not make
reward completion atomic. Research notes were appended to that bead.

The next content spike should compare a representative native reward flow with
Msg/If/flags/GiveItem scripting, measuring the whole shipping image and stack.
Do not predict savings solely from moving source lines into an FX script.

## Current VM constraints

- Commands are Msg, TMsg, SMsg, Tp, TpIf, If, SetFlag, UnsetFlag, ReadFlag and End.
  There are no battle, item, animation, wait or choice commands yet.
- Execution is synchronous, capped at 30 commands, and end rewinds to the base.
  Messages enqueue dialogs; there is no coroutine or selection-result mechanism.
- Scripts occupy a borrowed 128 B slot in WorldTransient, overlaid with world
  movement state. runInteractionScript saves movement and restores it after run.
- Entering battle replaces WorldTransient in the ModeState union. A command
  must stop and return its intent before that replacement. The world caller must
  restore/check state, perform the transition and return early. Continuing VM
  execution or restoring movement after replacement would corrupt battle state.
- A battle VM cannot borrow the world script slot. New buffer/state, suspension
  and command machinery all count against the savings.
- New opcodes span the tool's AST/DSL/emitter and generated header, plus firmware
  initVM operand-boundary validation and execution. Operands are big-endian;
  raw map text framing is little-endian. Generated headers must not be edited.

## Keep native

Movement/collision, random encounter selection, battle rules/HP/AI and save
journal operations need their native invariants. Encounter selection already
uses a 20 B transition-loaded cache; executing FX-loaded scripts on each step
would add reads and violate the transition-only cart-read rule. Inventory,
plant growth and save state do not disappear when scripts invoke their helpers.

MenuV2 owns navigation and cached assets. A script can open a menu once a yielding
contract exists, but cannot eliminate that controller. BattlePresenter owns
frame timing, shake, animations and dynamic damage captions. Type/category
animation groups are better evaluated as compact native tables first.

LurePrototype is not wired into shipping, and arena bootstrap is conditional.
Rewriting either cannot be credited with savings in the standard shipping image.
The generic menu cleanup bead jp8.4.11 also contains historical resource figures
and must be refreshed before use.

## Verification and resource interpretation

The VM trim device spike passed scripts 32/0 and stack 4/0. Its stack harness
reported 434 B painted headroom, 365 B after the 69 B USB ISR allowance; mode
transition headroom was 545 B. These are harness figures, not proof that shipping
free SRAM equals stack reserve. Baseline physical SRAM remaining is 773 B.

Exact commands and integrated verification are recorded in output.md. Production
ScriptVM.hpp and CreatureGathererFX.ino were restored before the final gate.
Implement the two measured cleanups separately, then evaluate GiveItem against
actual repeated acquisition flows.
