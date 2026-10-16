# duel-parity — host driver for the RPS duel kernel

`duel_kernel_driver.cpp` is a tiny host-only line-protocol harness around the *actual*
`src/engine/battle/DuelKernel.hpp` helpers. It exists so
`tools/tests/duel-kernel-parity_test.mjs` can compare the real C++ kernel against the
canonical mock (`mock/battle-mock.js`, 5-type matrix, bonus scoring) without
reimplementing any rule.

The kernel now stores a creature's folded kit as a 5-bit `attackMask` (bit *i* =
duel type *i* in the order `[spirit, water, earth, fire, plant]`), matching the
generated per-creature `attackMask[]`. `MEM` still takes the folded option list on
the wire and folds it into that mask; the parity test derives that list from the
mock's own kit (`mock.duelOptionsFor`), which for the 5-type table is the same mask
bit order.

Build (one small reuse of the shared host lane, no AVR):

```sh
c++ -std=c++17 -w -O0 -I . tools/duel-parity/duel_kernel_driver.cpp -o build/duel-sim/duel_kernel_driver
node --test tools/tests/duel-kernel-parity_test.mjs   # compiles the driver if stale
```

The bounded draw is xorshift32 with an integer floor stream: advance the state, then
`(state * bound) >> 32`. That is bit-for-bit equal to the mock's
`Math.floor(state / 2^32 * bound)` for the small bounds used, so RNG-driven
tie-breaking matches.

Commands (stdin, one per line) and their single output line:

| Command | Output |
| --- | --- |
| `STATE <c0> <c1> <a0> <a1> <outcome>` | `OK` |
| `MEM <side> <slot> <id> <tc> <t0> <t1> <hp> <maxHp> <oc> <o...>` | `OK` (the `o...` folded types become the member's `attackMask`) |
| `CLEARHIST` / `HIST <n> <h...>` | `OK` |
| `SEED <seed> <skip>` | `OK` |
| `AICONT <persona> <readRate>` | `PICK <kind> <index>` |
| `AIFRESH <seed> <persona> <readRate>` | `PICK <kind> <index>` |
| `PREDICT <persona> <seed> <n> <h...>` | `PRED <type>` |
| `ROLL <seed> <n> <bound...>` | `ROLLS <v...>` |
| `ACT <k0> <i0> <k1> <i1> <recordType>` | `RES ...` |
| `DONE` | `BYE` |

`kind`: 0 Attack, 1 Switch (index = target original slot), 2 Wait (index = 255).

The parity test always rebuilds this driver, so a modified `DuelKernel.hpp` /
`BattleRng.hpp` is picked up on every run. Its comparisons are strict: TRADE rounds
with asymmetric creature modifiers and dual-typed defenders are included, with no
quarantine or skips.
