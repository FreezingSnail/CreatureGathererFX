# Battle chart and modifier sharing spike

Bead: CreatureGathererFX-886. GPT-6-Luna implemented and measured the prototypes;
the parent reviewed the source changes and ELF symbols. Baseline: 0b9d57a.
Research only: production and characterization-test changes were restored.

## Whole shipping image measurements

| Checkpoint | Flash | Static SRAM | Flash delta |
| --- | ---: | ---: | ---: |
| Baseline | 27,894 B | 1,787 B | — |
| A: one canonical type chart | 27,712 B | 1,787 B | -182 B from baseline |
| B: A plus paired status helper | 27,672 B | 1,787 B | -40 B from A; -222 B total |

Baseline ELF contains two distinct 81-byte PROGMEM typeTable objects. Both
prototypes contain one 81-byte typeTable. In B, the two LTO helper symbols share
the same code address. The parent independently confirmed these symbols.
The 182-byte saving exceeds the removed table's size; the whole-image result is
measured, but the remaining saving has not been attributed through disassembly.

A changes the header definition to an extern declaration and defines the chart
in Type.cpp, with AVR PROGMEM and native const storage. Explicit host/simulator
source lists include Type.cpp. Review found all rows preserved, including
Lightning's Water entry and the implicit zero STATUS row/column. No enum, FX
data, per-frame reads or persistent state changes are involved.

B adds typeEffectPairModifier(first, second, types), combining the two existing
typeEffectModifier results. Damage and Resolve call it without changing their
outer grouping, validation, inversion or STAB order. Native helpers and shared
storage provide the saving; a battle VM is unnecessary for this result.

## Behavior discrepancy discovered

combineModifier clamps at every call and is not associative. Damage groups the
attacker modifier with the inverse defender modifier before combining the type
matchup. Resolve combines the matchup, attacker and defender sequentially.

A host characterization used WATER/WIND with DRNCHD+AIRSWPT against EARTH/PLANT
with SOILED+TANGLD, a WATER move of power 10, attack 40 and defense 20. It confirmed
40 damage (Double modifier on base 20), but Quadruple in the resolved result.
Both A and B preserve this existing behavior. Bug **CreatureGathererFX-46z**
tracks making the reported modifier agree with damage without silently changing
balance. The characterization patch was saved before restoration.

## Verification and review

A host: 155,553/0; world 190/0. B host: 155,553/0; world 190/0; VM 42/0.
B device presentation: 210/0; stack: 4/0, painted 418 B / effective 349 B after
the 69 B ISR allowance. B session: 57/0, painted 440 B / effective 371 B.
All exceed the 150 B effective reserve. No full gate was run on either prototype;
the parent runs the final gate on restored production sources for this research
commit. An implementation must run its own integrated gate.

One host compile attempt failed because the characterization used nonexistent
addEffect instead of applyEffect. Corrected and rerun successfully. No failed
resource/device result was reported. Simulator wiring was added but its build
was not exercised; that remains part of implementation acceptance.

Parent review: the chart move and paired helper meet the spike contract and are
recommended for implementation in **CreatureGathererFX-ecu**. Preserve existing
clamp order there; handle the discrepancy in 46z with its own tests. Broader
effect-rate, stage-table, row-packing and presentation changes were excluded.

Local ignored evidence is under build/battle-sharing-spike: baseline/table/helpers
ELFs, reviewed-full.patch and Type.cpp.prototype. These are local spike artifacts,
not durable implementation deliverables. Exact commands are recorded in output.md.

## ecu implementation checkpoint

The reviewed chart and paired helper were implemented in CreatureGathererFX-ecu.
The shipping ELF measured 27,672 B flash / 1,787 B static SRAM, matching the
prototype's -222 B flash / unchanged SRAM delta. `avr-nm` found exactly one
0x51-byte `typeTable`. Permanent native coverage checks all fixture-backed
elemental cells, the STATUS row and column, NONE sentinel handling, and the
reachable damage/result saturation difference. That difference remains owned
by CreatureGathererFX-46z.

The ecu checkpoint passed host 155,573/0 and world 190/0, VM 42/0, simulator
prepared-state tests 155,745/0, and simulator anchor mode (4 matches). Generated
verification passed. Presentation passed 210/0; stack passed 4/0 with 418 B
painted / 349 B effective headroom. Session passed 57/0 with 440 B painted /
371 B effective headroom. The parent owns the integrated final gate and commit.

The ecu full gate passed: host155,573/0, world190/0, VM42/0, all27 FX suites and
generated checks. Shipping27672/1787 B; stack418 B and arena callback effective
reserve233 B. Parent reviewed and committed ecu before starting46z.
