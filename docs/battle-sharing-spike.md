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
