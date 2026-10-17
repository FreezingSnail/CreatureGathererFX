# 2. Platform and resource budgets

| Property | Value |
|---|---|
| Hardware | Arduboy FX, ATmega32u4; 128×64 SSD1306 1bpp; external FX flash for the cart image |
| Rendering | Native `Arduboy2Base` framebuffer, one update+render per loop, `FX::display(CLEAR_BUFFER)` presents |
| Frame rate | 52 fps (`setFrameRate(52)`) |
| Math | Integer/fixed-point only; no `float`/`double` on device |
| RNG | Seeded once in setup from the hardware entropy source; xorshift16 (`randomRoll`); injectable `battle::Rng` seam for tests/parity |
| USB | Shipping builds disable the USB/CDC stack; DOWN at boot (or hold while resetting) enters the bootloader for upload |

## Measured budgets <span class="tag live">LIVE</span>

| Metric | Current | Limit | Headroom |
|---|---|---|---|
| Program flash (shipping FX) | **23,380 B** | 29,184 B planning / 29,696 B hard | 6,316 B to hard limit |
| Static RAM (shipping FX) | **1,680 B** | 2,160 B project / 2,560 B physical | 480 B to project budget |
| Painted stack headroom (test_stack) | **598 B** | 219 B minimum asserted (≈150 B reserve + 69 B USB ISR) | 379 B |
| Arena demo firmware | 23,382 B flash / 1,765 B static | 29,184 B arena ceiling | 5,802 B |

These are dated whole-image measurements (latest `make ram`, 2026-10-17), not guarantees. Every resource-changing work item measures a fresh same-source control and keeps the ~150 B effective reserve. Per `AGENTS.md`, LTO makes per-symbol arithmetic meaningless — only whole-image A/B counts.

## Build profiles and defines

| Define | Effect |
|---|---|
| `CGFX_SHIPPING_NO_USB` | Default shipping: USB/CDC stack removed |
| `CGFX_DUEL_BATTLE` | 5-type duel engine (default in shipping and device tests) |
| `CGFX_BATTLE_NO_FACTS` / `CGFX_BATTLE_COMPACT_RESULT` | Compact result/feedback representation |
| `CGFX_COMPACT_WARP_VM` | Default world script profile (If/TpIf/End only, no popups) |
| `CGFX_FULL_WORLD_VM` | Opt-in full interpreter: messages, popup queue, `GiveItem` |
| `FX_READ_COUNTER` | Device-test builds: counts program-data reads; steady-state must add zero |
| `CGFX_TRAINER_DEMO`, `CGFX_WILD_DEMO`, `CGFX_ARENA_DEMO` | Opt-in developer demos (see the build variants section) |
