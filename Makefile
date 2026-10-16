.PHONY: help setup doctor plant test test-debug testvm testvm-debug sim-test gen gen-if-needed gen-data gen-sprites gen-fixtures pack full build mini ram run dev arena-demo arena-test lure-demo check final-gate verify-generated test-manifest test-generated-libs test-doctor test-fxtest-ram test-avr-build-budget test-world-script-profile world-script-profile-check test-encounter-profile encounter-profile-check encounter-packed-profile-check fxtest fxtest-headless fxtest-spike fxtest-preflight fxtest-headless-preflight fxtest-build fxtest-run new-fxtest clean-build-artifacts test-clean-build-artifacts

# Public command API. Override tool, board, and output variables per workspace/CI.
CXX ?= g++
ARDUINO_CLI ?= arduino-cli
FQBN ?= arduboy-homemade:avr:arduboy-fx
MINI_FQBN ?= arduboy-homemade:avr:arduboy-mini
# Shared AVR toolchain properties for shipping and device-test builds. Relaxed
# linking plus shared function prologues save flash without changing measured
# static RAM or painted stack. Shipping builds add the no-USB entry separately.
AVR_RELAX_FLAGS ?= -mrelax -mcall-prologues -fno-move-loop-invariants -mstrict-X
# Propagate only world-profile selection to tests, preserving their USB entry.
WORLD_VM_PROFILE_FLAGS = $(filter -DCGFX_FULL_WORLD_VM -DCGFX_FULL_WORLD_VM=%,$(AVR_SHIPPING_CPP_FLAGS))
COMPACT_WARP_VM_FLAGS = $(filter -DCGFX_COMPACT_WARP_VM -DCGFX_COMPACT_WARP_VM=%,$(AVR_SHIPPING_CPP_FLAGS))
BATTLE_AUTO_REPLACE_FLAGS = $(filter -DCGFX_BATTLE_AUTO_REPLACE -DCGFX_BATTLE_AUTO_REPLACE=%,$(AVR_SHIPPING_CPP_FLAGS))
# Instrument FX test firmware with the logical read counter while preserving
# the stock USB main used by the serial P/F harness.
AVR_FXTEST_CPP_FLAGS ?= $(AVR_RELAX_FLAGS) -DFX_READ_COUNTER -DCGFX_DUEL_BATTLE $(WORLD_VM_PROFILE_FLAGS) $(COMPACT_WARP_VM_FLAGS) $(BATTLE_AUTO_REPLACE_FLAGS)
AVR_FXTEST_BUILD_PROPERTIES ?= --build-property "compiler.cpp.extra_flags=$(AVR_FXTEST_CPP_FLAGS)" \
	--build-property "compiler.c.extra_flags=$(AVR_RELAX_FLAGS)" \
	--build-property "compiler.c.elf.extra_flags=$(AVR_RELAX_FLAGS)"
AVR_FXTEST_NO_FACTS_BUILD_PROPERTIES ?= --build-property "compiler.cpp.extra_flags=$(AVR_FXTEST_CPP_FLAGS) -DCGFX_BATTLE_NO_FACTS" \
	--build-property "compiler.c.extra_flags=$(AVR_RELAX_FLAGS)" \
	--build-property "compiler.c.elf.extra_flags=$(AVR_RELAX_FLAGS)"
AVR_FXTEST_COMPACT_RESULT_BUILD_PROPERTIES ?= --build-property "compiler.cpp.extra_flags=$(AVR_FXTEST_CPP_FLAGS) -DCGFX_BATTLE_NO_FACTS -DCGFX_BATTLE_COMPACT_RESULT" \
	--build-property "compiler.c.extra_flags=$(AVR_RELAX_FLAGS)" \
	--build-property "compiler.c.elf.extra_flags=$(AVR_RELAX_FLAGS)"
# The dedicated duel suite must compile every TU with the compact duel profile;
# a sketch-local define alone would not reach separately compiled TUs and could
# silently build a legacy implementation. Selected for test_battle_duel only.
AVR_FXTEST_DUEL_BUILD_PROPERTIES ?= --build-property "compiler.cpp.extra_flags=$(AVR_FXTEST_CPP_FLAGS) -DCGFX_BATTLE_NO_FACTS -DCGFX_BATTLE_COMPACT_RESULT -DCGFX_DUEL_BATTLE" \
	--build-property "compiler.c.extra_flags=$(AVR_RELAX_FLAGS)" \
	--build-property "compiler.c.elf.extra_flags=$(AVR_RELAX_FLAGS)"
# Only shipping FX/Mini sketches compile their no-USB main. Device-test
# sketches keep the stock core main so Serial P/F output remains available.
AVR_SHIPPING_CPP_FLAGS ?= $(AVR_RELAX_FLAGS) -DCGFX_SHIPPING_NO_USB -fno-inline-small-functions -DCGFX_BATTLE_NO_FACTS -DCGFX_BATTLE_COMPACT_RESULT -DCGFX_COMPACT_WARP_VM -DCGFX_DUEL_BATTLE
AVR_SHIPPING_BUILD_PROPERTIES ?= --build-property "compiler.cpp.extra_flags=$(AVR_SHIPPING_CPP_FLAGS)" \
	--build-property "compiler.c.extra_flags=$(AVR_RELAX_FLAGS)" \
	--build-property "compiler.c.elf.extra_flags=$(AVR_RELAX_FLAGS)"
# Use the physical application flash limit. Keep 400 B static free as an
# independent guard; it does not measure painted stack.
AVR_FLASH_BUDGET ?= 29696
AVR_STATIC_RAM_BUDGET ?= 2160
BUILD_DIR ?= build
GEN_CACHE_STAMP ?= $(BUILD_DIR)/gen-cache/fingerprint
ARDUINO_BUILD_PATH ?= $(BUILD_DIR)/arduino-build
ARDUINO_BUILD_CACHE_PATH ?= $(BUILD_DIR)/arduino-cache
DIST_DIR ?= dist
FXDATA_BIN ?= $(DIST_DIR)/fxdata.bin
FXDATA_DATA_BIN ?= $(DIST_DIR)/fxdata-data.bin
FXDATA_SAVE_BIN ?= $(DIST_DIR)/fxdata-save.bin
RUN_HEX ?= $(BUILD_DIR)/CreatureGathererFX.ino.hex
RUN_FXPORT ?= d1
RUN_DISPLAY ?= ssd1306
FX_LAYOUT ?= fxlayout.toml
FXDATA_MANIFEST ?= fxdata/generated/manifest.json
FXDATA_DIST_DIR ?= $(DIST_DIR)
ARDENS ?= $(shell command -v Ardens 2>/dev/null)
# Emulated serial-capture window. The v3 save suite performs several full
# 4096-byte checkpoint verify/erase cycles, so it needs more emulated time than
# the former 3000 default; raise the window rather than drop device coverage.
FXTEST_MS ?= 12000
# The duel resource suite renders many full frames, so its serial capture needs
# a longer default than the wildcard gate's ordinary suites.
FXTEST_MS_RESOURCES ?= 30000
# The v3 save suite grew boxed-instance-HP publication cuts (jp8.11.14), each
# doing full checkpoint verify/erase cycles, so it needs a longer window than the
# 12000 default. Raise the window rather than drop device coverage.
FXTEST_MS_SAVE ?= 30000
FXTEST_RAM_BUDGET ?= 2160
FXTEST_BUILD_DIR ?= $(BUILD_DIR)/fxtest
FINAL_GATE_LOG_DIR ?= $(BUILD_DIR)/final-gate
HOST_TEST_BIN ?= $(BUILD_DIR)/tests/host
WORLD_TEST_BIN ?= $(BUILD_DIR)/tests/world
VM_TEST_BIN ?= $(BUILD_DIR)/tests/vm
GENERATED_TEST_BIN ?= $(BUILD_DIR)/tests/generated
WORLD_SCRIPT_PROFILE_BIN ?= $(BUILD_DIR)/tools/check-world-script-profile
WORLD_SCRIPT_PROFILE_TEST_BIN ?= $(BUILD_DIR)/tests/world-script-profile
WORLD_SCRIPT_PROFILE_INPUT ?= fxdata/generated/scripts.bin
WORLD_SCRIPT_PROFILE_TEXT ?= fxdata/generated/text.bin
ENCOUNTER_PROFILE_BIN ?= $(BUILD_DIR)/tools/check-encounter-profile
ENCOUNTER_PROFILE_TEST_BIN ?= $(BUILD_DIR)/tests/encounter-profile
ENCOUNTER_PROFILE_INPUT ?= fxdata/data/encounters.txt
RAM_ELF ?= $(BUILD_DIR)/CreatureGathererFX.ino.elf
AVR_NM ?= $(shell command -v avr-nm 2>/dev/null || find "$(HOME)/Library/Arduino15/packages" "$(HOME)/.arduino15/packages" -type f -path '*/tools/avr-gcc/*/bin/avr-nm' -print -quit 2>/dev/null)
AVR_SIZE ?= $(shell command -v avr-size 2>/dev/null || find "$(HOME)/Library/Arduino15/packages" "$(HOME)/.arduino15/packages" -type f -path '*/tools/avr-gcc/*/bin/avr-size' -print -quit 2>/dev/null)

# The five-type duel is the sole battle engine; select it for every build.
CPPFLAGS ?= -I. -DCGFX_DUEL_BATTLE
CXXFLAGS ?= -std=c++17 -w -O0 -g3
TEST_CPPFLAGS = -I tst/host $(WORLD_VM_PROFILE_FLAGS) $(COMPACT_WARP_VM_FLAGS) $(BATTLE_AUTO_REPLACE_FLAGS)
TEST_FLAGS = -DTEST
DEBUG_FLAGS = -DDEBUG

help:
	@printf '%s\n' \
		'CreatureGathererFX Make targets:' \
		'  setup    print non-mutating first-run guidance; prerequisite: make' \
		'  doctor   report local tool readiness; prerequisite: tools/doctor.sh' \
		'  gen      package committed/generated FX inputs; prerequisite: cgfx-tools; output: $(DIST_DIR)/fxdata.bin' \
		'  gen-if-needed reuse current generated FX outputs for opt-in demos; stale content regenerates' \
		'  test     run fast host C++ tests; prerequisite: $(CXX); output: $(HOST_TEST_BIN)' \
		'  sim-test run the duel kernel parity check; output: node --test tools/tests/duel-kernel-parity_test.mjs' \
		'  testvm   run fast ScriptVM C++ tests; prerequisite: $(CXX); output: $(VM_TEST_BIN)' \
		'  test-world-script-profile validate the compact world VM script profile' \
		'  build    compile Arduboy FX sketch; prerequisite: $(ARDUINO_CLI); output: $(BUILD_DIR)' \
		'  ram      build and report FX flash/RAM plus largest static symbols; prerequisite: $(ARDUINO_CLI), avr-size, avr-nm' \
		'  arena-demo generate FX data, build the opt-in arena+duel firmware, then launch Ardens' \
		'  arena-test alias for arena-demo' \
		'  lure-demo  generate FX data, build the opt-in lure-wedge firmware, then launch Ardens' \
		'  test-avr-build-budget exercise the fail-closed shipping flash/RAM parser' \
		'  final-gate  run the full check then the shipping RAM report; requires ARDENS' \
		'  run      launch Ardens with the sketch, FX data, and FX save images; prerequisite: ARDENS' \
		'  dev      regenerate FX data, rebuild, then launch Ardens (gen + run); prerequisite: cgfx-tools, ARDENS' \
		'  check    run generation, host tests, VM tests, then optional FX runtime tests' \
		'  test-manifest run permanent generated-artifact tests' \
		'  test-generated-libs check generated libs against the packed image; prerequisite: $(CXX)' \
		'  test-pack-parity verify native packed-image SHA-256 baseline' \
		'  test-doctor run permanent setup-diagnostic tests' \
		'  clean-build-artifacts preview generated Arduino cache cleanup; add CLEAN_APPLY=1 to delete' \
		'  test-clean-build-artifacts exercise cleanup safety in isolated build fixtures' \
		'  test-fxtest-ram exercise the fail-closed device ELF static-RAM guard' \
		'  fxtest   alias for fxtest-headless; skips only when ARDENS is unset' \
		'  fxtest-headless  run every FX device sketch through Ardens serial capture; blocks unsupported Ardens' \
		'  fxtest-spike  run one selected FX suite plus test_stack; set FXTEST_SPIKE_INO and ARDENS' \
		'' \
		'Overrides: CXX, ARDUINO_CLI, FQBN, BUILD_DIR, ARDUINO_BUILD_PATH, ARDUINO_BUILD_CACHE_PATH, DIST_DIR, FXDATA_BIN, ARDENS, FXTEST_MS, FXTEST_RAM_BUDGET, AVR_FLASH_BUDGET, AVR_STATIC_RAM_BUDGET, RAM_ELF, AVR_SIZE, AVR_NM, AVR_RELAX_FLAGS, AVR_FXTEST_BUILD_PROPERTIES, AVR_FXTEST_NO_FACTS_BUILD_PROPERTIES, AVR_FXTEST_COMPACT_RESULT_BUILD_PROPERTIES, AVR_FXTEST_CPP_FLAGS, AVR_SHIPPING_BUILD_PROPERTIES, AVR_SHIPPING_CPP_FLAGS, WORLD_SCRIPT_PROFILE_INPUT, WORLD_SCRIPT_PROFILE_TEXT, ARENA_DEMO_BUILD_DIR, ARENA_DEMO_CART_DIR, ARENA_DEMO_CPP_FLAGS, ARENA_DEMO_FLASH_BUDGET.'

setup:
	@printf '%s\n' \
		'setup: guidance only; no installs or Arduino configuration changes.' \
		'1. Install g++, make, and arduino-cli with your platform package manager.' \
		'2. Review and run: SETUP_APPLY=1 tools/setup.sh' \
		'3. Verify readiness: make doctor' \
		'For manual setup, see each remedy printed by make doctor.'

doctor:
	@CXX="$(CXX)" ARDUINO_CLI="$(ARDUINO_CLI)" FQBN="$(FQBN)" ARDENS="$(ARDENS)" ./tools/doctor.sh

# Common source files for main tests
TEST_SOURCES = src/lib/Blit.cpp \
	src/lib/Text.cpp \
	src/lib/Type.cpp \
	tst/src/Arduboy2Host.cpp \
	tst/src/ReadData.cpp \
	tst/src/DialogMenu.cpp \
	src/engine/menu/DialogQueue.cpp \
	src/engine/menu/MenuNav.cpp \
	src/engine/menu/MenuV2.cpp \
	src/save/SaveController.cpp \
	tst/src/random.cpp \
	tst/src/FlashBackendFake.cpp \
	src/save/SaveFile.cpp \
	src/save/Journal.cpp \
	src/save/Compaction.cpp \
	src/plants/PlantStage.cpp \
	src/plants/PlantPair.cpp \
	src/plants/PlantGamestate.cpp \
	src/item/Inventory.cpp \
	src/item/KeyItems.cpp \
	src/item/ConsumableDef.cpp \
	src/item/ItemNames.cpp \
	src/creature/Creature.cpp \
	src/player/Player.cpp \
	src/action/Action.cpp \
	src/lib/ListView.cpp \
	src/engine/battle/Ai.cpp \
	src/engine/battle/Damage.cpp \
	src/engine/battle/Effects.cpp \
	src/engine/battle/Resolve.cpp \
	src/engine/battle/BattleSession.cpp \
	src/engine/battle/BattleFlow.cpp \
	src/engine/battle/BattlePresets.cpp \
	src/engine/battle/BattleSetup.cpp \
	src/engine/battle/BattlePresenter.cpp \
	src/engine/battle/BattleMenu.cpp \
	src/engine/battle/BattleFeedback.cpp \
	src/engine/battle/BattleScene.cpp \
	src/engine/arena/ArenaCatalog.cpp \
	src/engine/arena/ArenaNavigation.cpp \
	src/engine/arena/ArenaDemo.cpp \
	src/engine/arena/ArenaView.cpp \
	src/engine/ModeState.cpp \
	src/engine/world/World.cpp \
	src/engine/world/Encounter.cpp \
	src/engine/world/LureZone.cpp \
	src/engine/world/StepEvent.cpp \
	src/GameState.cpp \
	src/flags/flag_bit_array.cpp \
	tst/main.cpp

# Common source files for VM tests. GiveItem executes against the one shared
# Player bag, so the VM test links the Player/Inventory/key sources plus the
# creature/read-data chain the Player constructor pulls in.
TESTVM_SOURCES = src/vm/ScriptVM.cpp \
	src/engine/menu/DialogQueue.cpp \
	src/item/Inventory.cpp \
	src/item/KeyItems.cpp \
	src/creature/Creature.cpp \
	src/player/Player.cpp \
	src/lib/Type.cpp \
	tst/src/ReadData.cpp \
	tst/src/DialogMenu.cpp \
	src/GameState.cpp \
	src/flags/flag_bit_array.cpp \
	tst/script_tests/action_test.cpp \
	tst/script_tests/main.cpp

# Function to run tests in target-owned output directories.
define run_test
	@mkdir -p "$(dir $(4))"
	$(CXX) $(1) $(CPPFLAGS) $(TEST_CPPFLAGS) $(CXXFLAGS) $(2) $(3) -o "$(4)" && "$(4)"
endef

full: gen build

build: world-script-profile-check encounter-packed-profile-check
	@mkdir -p "$(BUILD_DIR)"
	@mkdir -p "$(ARDUINO_BUILD_PATH)/fx"
	@set -eu; \
	log="$(BUILD_DIR)/build.log"; \
	if ARDUINO_BUILD_CACHE_PATH="$(ARDUINO_BUILD_CACHE_PATH)" $(ARDUINO_CLI) compile --fqbn "$(FQBN)" $(AVR_SHIPPING_BUILD_PROPERTIES) --build-path "$(ARDUINO_BUILD_PATH)/fx" --output-dir "$(BUILD_DIR)" . >"$$log" 2>&1; then \
		cat "$$log"; \
	else \
		cat "$$log"; exit 1; \
	fi; \
	./tools/check-avr-build-budget.sh "$$log" "$(AVR_FLASH_BUDGET)" "$(AVR_STATIC_RAM_BUDGET)" FX

ram: build
	@set -eu; \
	elf="$(RAM_ELF)"; \
	test -f "$$elf" || { echo "ram: ELF not found at $$elf" >&2; exit 1; }; \
	command -v "$(AVR_SIZE)" >/dev/null 2>&1 || { echo "ram: avr-size not found; install Arduino AVR-GCC or set AVR_SIZE=/path/to/avr-size" >&2; exit 1; }; \
	command -v "$(AVR_NM)" >/dev/null 2>&1 || { echo "ram: avr-nm not found; install Arduino AVR-GCC or set AVR_NM=/path/to/avr-nm" >&2; exit 1; }; \
	size_output=$$("$(AVR_SIZE)" --format=avr --mcu=atmega32u4 "$$elf"); \
	printf '%s\n' "$$size_output"; \
	flash_bytes=$$(printf '%s\n' "$$size_output" | awk '$$1 == "Program:" {print $$2; exit}'); \
	static_bytes=$$(printf '%s\n' "$$size_output" | awk '$$1 == "Data:" {print $$2; exit}'); \
	test -n "$$flash_bytes" && test -n "$$static_bytes" || { echo "ram: unable to parse avr-size output" >&2; exit 1; }; \
	flash_limit=29696; static_limit=2560; free_bytes=$$((static_limit - static_bytes)); \
	printf 'RAM_ELF=%s\nRAM_FLASH_BYTES=%s\nRAM_FLASH_LIMIT=%s\nRAM_STATIC_BYTES=%s\nRAM_STATIC_LIMIT=%s\nRAM_FREE_BYTES=%s\n' \
	    "$$elf" "$$flash_bytes" "$$flash_limit" "$$static_bytes" "$$static_limit" "$$free_bytes"; \
	echo 'RAM_TOP_SYMBOLS_BEGIN'; \
	echo 'address size type name'; \
	"$(AVR_NM)" --print-size --size-sort --radix=d "$$elf" | awk '$$3 ~ /^[bBdD]$$/' | tail -15; \
	echo 'RAM_TOP_SYMBOLS_END'

mini: world-script-profile-check encounter-packed-profile-check
	@mkdir -p "$(BUILD_DIR)"
	@mkdir -p "$(ARDUINO_BUILD_PATH)/mini"
	@set -eu; \
	log="$(BUILD_DIR)/mini-build.log"; \
	if ARDUINO_BUILD_CACHE_PATH="$(ARDUINO_BUILD_CACHE_PATH)" $(ARDUINO_CLI) compile --fqbn "$(MINI_FQBN)" $(AVR_SHIPPING_BUILD_PROPERTIES) --build-path "$(ARDUINO_BUILD_PATH)/mini" --output-dir "$(BUILD_DIR)" . >"$$log" 2>&1; then \
		cat "$$log"; \
	else \
		cat "$$log"; exit 1; \
	fi; \
	./tools/check-avr-build-budget.sh "$$log" "$(AVR_FLASH_BUDGET)" "$(AVR_STATIC_RAM_BUDGET)" Mini

# Interactive development run, not a test path: FX suites still execute only
# through fxtest-headless. Data and save images load separately because Ardens
# routes `save=` into its FX save region and a plain `file=` .bin into FX data.
# Every path is passed as `key=value`; a bare path parses as an empty value and
# Ardens reports `Could not open file: ""`.
run: build
	@test -n "$(ARDENS)" || { echo "run: ARDENS is unset; set ARDENS=/path/to/Ardens" >&2; exit 1; }
	@test -x "$(ARDENS)" || { echo "run: Ardens executable not found at $(ARDENS)" >&2; exit 1; }
	@test -f "$(RUN_HEX)" || { echo "run: sketch hex missing at $(RUN_HEX); run make build" >&2; exit 1; }
	@test -f "$(FXDATA_DATA_BIN)" || { echo "run: FX data image missing at $(FXDATA_DATA_BIN); run make gen" >&2; exit 1; }
	@test -f "$(FXDATA_SAVE_BIN)" || { echo "run: FX save image missing at $(FXDATA_SAVE_BIN); run make gen" >&2; exit 1; }
	"$(ARDENS)" \
	    fxport=$(RUN_FXPORT) display=$(RUN_DISPLAY) \
	    file="$(RUN_HEX)" \
	    file="$(FXDATA_DATA_BIN)" \
	    save="$(FXDATA_SAVE_BIN)"

# Opt-in trainer battle demo. Keeps demo firmware/cart/save isolated from the
# normal development build and launches graphical Ardens directly.
BATTLE_TEST_BUILD_DIR ?= build/trainer-demo
BATTLE_TEST_CART_DIR ?= $(BATTLE_TEST_BUILD_DIR)/isolated
BATTLE_TEST_CPP_FLAGS ?= $(AVR_RELAX_FLAGS) -DCGFX_SHIPPING_NO_USB -DCGFX_TRAINER_DEMO
ifeq ($(BATTLE_TEST_SWITCH_DRILL),1)
BATTLE_TEST_CPP_FLAGS += -DCGFX_TRAINER_DEMO_SWITCH_DRILL
endif
ifeq ($(BATTLE_TEST_EXPANSION),1)
BATTLE_TEST_CPP_FLAGS += -DCGFX_TRAINER_DEMO_EXPANSION
endif
ifeq ($(BATTLE_TEST_48),1)
BATTLE_TEST_CPP_FLAGS += -DCGFX_BATTLE_48_SPIKE
endif

.PHONY: battle-test
battle-test: gen
	@test -n "$(ARDENS)" || { echo "battle-test: ARDENS is unset; set ARDENS=/path/to/Ardens" >&2; exit 1; }
	@test -x "$(ARDENS)" || { echo "battle-test: Ardens executable not found at $(ARDENS)" >&2; exit 1; }
	@$(MAKE) --no-print-directory build BUILD_DIR="$(BATTLE_TEST_BUILD_DIR)" AVR_SHIPPING_CPP_FLAGS="$(BATTLE_TEST_CPP_FLAGS)"
	@mkdir -p "$(BATTLE_TEST_CART_DIR)"
	@cp -f "$(FXDATA_DATA_BIN)" "$(BATTLE_TEST_CART_DIR)/fxdata-data.bin"
	@cp -f "$(FXDATA_SAVE_BIN)" "$(BATTLE_TEST_CART_DIR)/fxdata-save.bin"
	"$(ARDENS)" \
	    fxport=$(RUN_FXPORT) display=$(RUN_DISPLAY) \
	    file="$(BATTLE_TEST_BUILD_DIR)/CreatureGathererFX.ino.hex" \
	    file="$(BATTLE_TEST_CART_DIR)/fxdata-data.bin" \
	    save="$(BATTLE_TEST_CART_DIR)/fxdata-save.bin"

# Full interactive loop: fresh FX data, normal sketch, then Ardens. Sequenced
# through a sub-make so `make -j dev` cannot launch before generation finishes.
dev: gen
	@$(MAKE) --no-print-directory run

# Opt-in arena demo. Generate the cart, build/report the isolated firmware, copy
# the split FX data/save inputs beside it, then launch Ardens.
ARENA_DEMO_BUILD_DIR ?= build/arena-demo
ARENA_DEMO_CART_DIR ?= $(ARENA_DEMO_BUILD_DIR)/isolated
# Full shipping defaults plus the arena UI and the opt-in duel engine, so the
# arena demo launches the current RPS duel spike from the existing bootstrap.
ARENA_DEMO_CPP_FLAGS ?= $(AVR_SHIPPING_CPP_FLAGS) -DCGFX_ARENA_DEMO -DCGFX_DUEL_BATTLE
ARENA_DEMO_FLASH_BUDGET ?= 29184

arena-demo:
	@test -n "$(ARDENS)" || { echo "arena-demo: ARDENS is unset; set ARDENS=/path/to/Ardens" >&2; exit 1; }
	@test -x "$(ARDENS)" || { echo "arena-demo: Ardens executable not found at $(ARDENS)" >&2; exit 1; }
	@$(MAKE) --no-print-directory -j1 gen-if-needed
	@$(MAKE) --no-print-directory ram BUILD_DIR="$(ARENA_DEMO_BUILD_DIR)" AVR_SHIPPING_CPP_FLAGS="$(ARENA_DEMO_CPP_FLAGS)" AVR_FLASH_BUDGET="$(ARENA_DEMO_FLASH_BUDGET)"
	@test -f "$(ARENA_DEMO_BUILD_DIR)/CreatureGathererFX.ino.hex" || { echo "arena-demo: sketch hex missing from $(ARENA_DEMO_BUILD_DIR)" >&2; exit 1; }
	@test -f "$(FXDATA_DATA_BIN)" || { echo "arena-demo: FX data image missing at $(FXDATA_DATA_BIN); run make gen" >&2; exit 1; }
	@test -f "$(FXDATA_SAVE_BIN)" || { echo "arena-demo: FX save image missing at $(FXDATA_SAVE_BIN); run make gen" >&2; exit 1; }
	@mkdir -p "$(ARENA_DEMO_CART_DIR)"
	@cp -f "$(FXDATA_DATA_BIN)" "$(ARENA_DEMO_CART_DIR)/fxdata-data.bin"
	@cp -f "$(FXDATA_SAVE_BIN)" "$(ARENA_DEMO_CART_DIR)/fxdata-save.bin"
	"$(ARDENS)" \
	    fxport=$(RUN_FXPORT) display=$(RUN_DISPLAY) \
	    file="$(ARENA_DEMO_BUILD_DIR)/CreatureGathererFX.ino.hex" \
	    file="$(ARENA_DEMO_CART_DIR)/fxdata-data.bin" \
	    save="$(ARENA_DEMO_CART_DIR)/fxdata-save.bin"

# Convenience alias for the arena duel spike launch.
arena-test: arena-demo

# Opt-in lure playtest. Mirrors arena-demo: generate, build the wedge firmware
# into an isolated directory, copy the split FX data/save inputs beside it, then
# launch Ardens. Shipping build is unaffected; the define only sets the boot
# location inside the pocket and seeds the world inventory.
LURE_DEMO_BUILD_DIR ?= build/lure-demo
LURE_DEMO_CART_DIR ?= $(LURE_DEMO_BUILD_DIR)/isolated
LURE_DEMO_CPP_FLAGS ?= $(AVR_SHIPPING_CPP_FLAGS) -DCGFX_LURE_WEDGE
LURE_DEMO_FLASH_BUDGET ?= 29184

lure-demo:
	@test -n "$(ARDENS)" || { echo "lure-demo: ARDENS is unset; set ARDENS=/path/to/Ardens" >&2; exit 1; }
	@test -x "$(ARDENS)" || { echo "lure-demo: Ardens executable not found at $(ARDENS)" >&2; exit 1; }
	@$(MAKE) --no-print-directory -j1 gen-if-needed
	@$(MAKE) --no-print-directory ram BUILD_DIR="$(LURE_DEMO_BUILD_DIR)" AVR_SHIPPING_CPP_FLAGS="$(LURE_DEMO_CPP_FLAGS)" AVR_FLASH_BUDGET="$(LURE_DEMO_FLASH_BUDGET)"
	@test -f "$(LURE_DEMO_BUILD_DIR)/CreatureGathererFX.ino.hex" || { echo "lure-demo: sketch hex missing from $(LURE_DEMO_BUILD_DIR)" >&2; exit 1; }
	@test -f "$(FXDATA_DATA_BIN)" || { echo "lure-demo: FX data image missing at $(FXDATA_DATA_BIN); run make gen" >&2; exit 1; }
	@test -f "$(FXDATA_SAVE_BIN)" || { echo "lure-demo: FX save image missing at $(FXDATA_SAVE_BIN); run make gen" >&2; exit 1; }
	@mkdir -p "$(LURE_DEMO_CART_DIR)"
	@cp -f "$(FXDATA_DATA_BIN)" "$(LURE_DEMO_CART_DIR)/fxdata-data.bin"
	@cp -f "$(FXDATA_SAVE_BIN)" "$(LURE_DEMO_CART_DIR)/fxdata-save.bin"
	"$(ARDENS)" \
	    fxport=$(RUN_FXPORT) display=$(RUN_DISPLAY) \
	    file="$(LURE_DEMO_BUILD_DIR)/CreatureGathererFX.ino.hex" \
	    file="$(LURE_DEMO_CART_DIR)/fxdata-data.bin" \
	    save="$(LURE_DEMO_CART_DIR)/fxdata-save.bin"

gen: gen-data gen-sprites gen-fixtures pack
	@./tools/ensure-generated.sh --record . "$(GEN_CACHE_STAMP)" \
	    "$(FX_LAYOUT)" "$(FXDATA_MANIFEST)" "$(FXDATA_DIST_DIR)" \
	    "$(DIST_DIR)" "$(FXDATA_BIN)" "$(FXDATA_DATA_BIN)" "$(FXDATA_SAVE_BIN)"

# Opt-in demos reuse a checked cache; ordinary `make gen` stays forceful.
gen-if-needed:
	@node tools/export-lure-world-spike.mjs
	@./tools/ensure-generated.sh --ensure . "$(GEN_CACHE_STAMP)" \
	    "$(FX_LAYOUT)" "$(FXDATA_MANIFEST)" "$(FXDATA_DIST_DIR)" \
	    "$(DIST_DIR)" "$(FXDATA_BIN)" "$(FXDATA_DATA_BIN)" "$(FXDATA_SAVE_BIN)" \
	    -- $(MAKE) --no-print-directory -j1 gen

gen-data:
	@set -e; \
	node tools/export-lure-world-spike.mjs; \
	cgfx-tools --project cgfx-project.json; \
	: 'Firmware includes generated headers from src; refresh copies atomically with generation.'; \
	cp -f fxdata/generated/opcodes.hpp src/vm/opcodes.hpp; \
	cp -f fxdata/generated/flags.hpp src/flags/flags.hpp; \
	cp -f fxdata/generated/flag_bit_array.hpp src/flags/flag_bit_array.hpp; \
	cp -f fxdata/generated/flag_bit_array.cpp src/flags/flag_bit_array.cpp; \
	cgfx-tools --arena-csv data/arena.csv --arena-output fxdata/generated; \
	cgfx-tools --type-table-csv data/typetable.csv --type-table-output fxdata/generated; \
	cgfx-tools --consumables-csv data/consumables.csv --consumables-output fxdata/generated; \

gen-fixtures:
	@set -e; \
	cgfx-tools --project cgfx-project.json --emit-fixtures; \
	./tools/emit-rust-teams.sh; \
	./tools/emit-tool-version-stamp.sh

gen-sprites:
	@set -e; \
	cgfx-tools --sprite-config fxsprites.toml

pack: encounter-profile-check
	@set -e; \
	mkdir -p "$(BUILD_DIR)" "$(DIST_DIR)"; \
	stage="$$(cd "$$(mktemp -d "$(BUILD_DIR)/cgfx-pack.XXXXXX")" && pwd -P)"; \
	trap 'rm -rf "$$stage"' 0; \
	trap 'exit 1' HUP INT TERM; \
	tar -cf - \
	    --exclude='./.git' --exclude='./.beads' --exclude='./.codex' \
	    --exclude='./.battle-carve-spike' --exclude='./build' --exclude='./dist' \
	    --exclude='./.cache' --exclude='./harness/workspaces' . | tar -xf - -C "$$stage"; \
	cgfx-tools --project "$$stage/cgfx-project.json" --pack --layout "$$stage/fxlayout.toml"; \
	mv -f "$$stage/src/fxdata.h" src/fxdata.h; \
	mv -f "$$stage/dist/fxdata.bin" "$(DIST_DIR)/fxdata.bin"; \
	mv -f "$$stage/dist/fxdata-data.bin" "$(DIST_DIR)/fxdata-data.bin"; \
	mv -f "$$stage/dist/fxdata-save.bin" "$(DIST_DIR)/fxdata-save.bin"; \
	./tools/record-fxdata-manifest.sh "$(FX_LAYOUT)" "$(FXDATA_MANIFEST)"; \
	./tools/assert-fxdata-manifest.sh "$(FX_LAYOUT)" "$(FXDATA_MANIFEST)"; \
	"$(ENCOUNTER_PROFILE_BIN)" "$(ENCOUNTER_PROFILE_INPUT)" src/fxdata.h "$(FXDATA_DATA_BIN)"

check: gen test testvm test-beta-area test-world-script-profile test-encounter-profile test-manifest test-generated-libs test-fxtest-ram test-avr-build-budget verify-generated build fxtest

# Authored first-beta contract (CreatureGathererFX-o845). The node suite pins the
# authored start/rest/landmarks, the generated collision and the shipping
# fresh-game start literal to each other, so a one-sided edit of either side
# fails the standard gate. It validates the generated collision, so it depends on
# gen (also ordered by check before the other gate prerequisites).
test-beta-area: gen
	node --test tools/tests/beta-area_test.mjs

# Final pre-commit gate. Keep the integrated check and shipping RAM build
# sequential even when the caller invokes make with -j.
final-gate:
	@test -n "$(ARDENS)" || { echo "final-gate: ARDENS is unset; set ARDENS=/path/to/Ardens" >&2; exit 1; }
	@./tools/run-logged-command.sh "$(FINAL_GATE_LOG_DIR)/check.log" $(MAKE) --no-print-directory -j1 check FXTEST_INOS="$(wildcard tst/fxdatatest/*.ino)"
	@./tools/run-logged-command.sh "$(FINAL_GATE_LOG_DIR)/ram.log" $(MAKE) --no-print-directory -j1 ram

verify-generated:
	@if test ! -e "$(FXDATA_DIST_DIR)/fxdata-data.bin" && test ! -e "$(FXDATA_DIST_DIR)/fxdata.bin"; then \
		echo 'verify-generated: image not built; run make gen to build packed FX artifacts'; \
		./tools/assert-fxdata-manifest.sh --skip-image "$(FX_LAYOUT)" "$(FXDATA_MANIFEST)"; \
	else \
		./tools/assert-fxdata-manifest.sh "$(FX_LAYOUT)" "$(FXDATA_MANIFEST)"; \
	fi

test-manifest:
	./tools/tests/fxdata-manifest_test.sh
	./tools/tests/ensure-generated_test.sh

# Generated-library integration tests: packed image <-> src/fxdata.h <-> the
# generated sources, plus generated-header drift and text-block framing.
test-generated-libs:
	@mkdir -p "$(dir $(GENERATED_TEST_BIN))"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tst/generated/generated_libs_test.cpp -o "$(GENERATED_TEST_BIN)"
	"$(GENERATED_TEST_BIN)" . "$(FX_LAYOUT)" src/fxdata.h "$(FXDATA_DIST_DIR)/fxdata-data.bin"
	./tools/tests/generated-libs_test.sh
	./tools/tests/arena-demo-data_test.sh
	./tools/tests/first-unqualified-alias_test.sh

test-world-script-profile:
	@mkdir -p "$(dir $(WORLD_SCRIPT_PROFILE_TEST_BIN))"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tools/tests/world-script-profile_test.cpp -o "$(WORLD_SCRIPT_PROFILE_TEST_BIN)"
	"$(WORLD_SCRIPT_PROFILE_TEST_BIN)"

$(WORLD_SCRIPT_PROFILE_BIN): tools/check-world-script-profile.cpp src/vm/opcodes.hpp src/flags/flag_bit_array.hpp
	@mkdir -p "$(dir $@)"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tools/check-world-script-profile.cpp -o "$@"

# Compact world scripts have no dialog text: scripts.bin stays fixed-size and
# text.bin is the two-byte zero-count header. Full-VM opt-in skips both checks.
world-script-profile-check: $(WORLD_SCRIPT_PROFILE_BIN) $(WORLD_SCRIPT_PROFILE_INPUT) $(WORLD_SCRIPT_PROFILE_TEXT)
	@if test -n "$(WORLD_VM_PROFILE_FLAGS)"; then \
		echo 'world-script-profile: full VM opt-in enabled'; \
	else \
		"$(WORLD_SCRIPT_PROFILE_BIN)" "$(WORLD_SCRIPT_PROFILE_INPUT)" "$(WORLD_SCRIPT_PROFILE_TEXT)"; \
	fi

# Encounter ABI validation. The authored check proves the encounter content
# invariants once at generation; the packed check also proves the published
# src/fxdata.h addresses match the authored bytes in the packed data image, so
# shipping FX and Mini compile only against validated content.
test-encounter-profile: encounter-profile-check
	@mkdir -p "$(dir $(ENCOUNTER_PROFILE_TEST_BIN))"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tools/tests/encounter-profile_test.cpp -o "$(ENCOUNTER_PROFILE_TEST_BIN)"
	"$(ENCOUNTER_PROFILE_TEST_BIN)"

$(ENCOUNTER_PROFILE_BIN): tools/check-encounter-profile.cpp src/engine/world/Encounter.hpp src/engine/world/Chunk.hpp
	@mkdir -p "$(dir $@)"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tools/check-encounter-profile.cpp -o "$@"

encounter-profile-check: $(ENCOUNTER_PROFILE_BIN) $(ENCOUNTER_PROFILE_INPUT)
	"$(ENCOUNTER_PROFILE_BIN)" --source "$(ENCOUNTER_PROFILE_INPUT)"

encounter-packed-profile-check: encounter-profile-check src/fxdata.h $(FXDATA_DATA_BIN)
	"$(ENCOUNTER_PROFILE_BIN)" "$(ENCOUNTER_PROFILE_INPUT)" src/fxdata.h "$(FXDATA_DATA_BIN)"

test-doctor:
	./tools/tests/doctor_test.sh

clean-build-artifacts:
	@case "$(CLEAN_APPLY)" in \
		1) ./tools/clean-build-artifacts.sh --apply ;; \
		*) ./tools/clean-build-artifacts.sh ;; \
	esac

test-clean-build-artifacts:
	@./tools/clean-build-artifacts_test.sh

test-fxtest-ram:
	./tools/tests/fxtest-ram_test.sh

test-avr-build-budget:
	./tools/tests/avr-build-budget_test.sh

# The legacy ordered-turn batch simulator (tools/battle-sim/) was retired after
# the 5-type duel migration; its source and provenance live under
# archive/battle-legacy/. sim-test now always routes to the seeded C++/JS parity
# check that compiles and runs the actual duel kernel
# (src/engine/battle/DuelKernel.hpp) against the canonical mock.
sim-test:
	node --test tools/tests/duel-kernel-parity_test.mjs

test-pack-parity:
	./tools/tests/pack-parity_test.sh

test:
	$(call run_test,,$(TEST_FLAGS),$(TEST_SOURCES),$(HOST_TEST_BIN))
	$(call run_test,-DWORLD_TEST,$(TEST_FLAGS),$(WORLD_TEST_SOURCES),$(WORLD_TEST_BIN))

test-debug:
	$(call run_test,$(DEBUG_FLAGS),$(TEST_FLAGS),$(TEST_SOURCES),$(HOST_TEST_BIN))
	$(call run_test,-DWORLD_TEST,$(TEST_FLAGS),$(WORLD_TEST_SOURCES),$(WORLD_TEST_BIN))

WORLD_TEST_SOURCES = src/GameState.cpp src/engine/world/World.cpp src/engine/world/Encounter.cpp src/engine/world/LureZone.cpp src/engine/world/StepEvent.cpp src/plants/PlantGamestate.cpp src/plants/PlantStage.cpp src/plants/PlantPair.cpp src/item/Inventory.cpp src/item/KeyItems.cpp src/item/ItemNames.cpp src/creature/Creature.cpp src/player/Player.cpp src/lib/Type.cpp src/lib/ListView.cpp src/flags/flag_bit_array.cpp tst/src/ReadData.cpp tst/src/random.cpp tst/world_test_main.cpp

testvm:
	$(call run_test,,$(TEST_FLAGS),$(TESTVM_SOURCES),$(VM_TEST_BIN))

testvm-debug:
	$(call run_test,$(DEBUG_FLAGS),$(TEST_FLAGS),$(TESTVM_SOURCES),$(VM_TEST_BIN))

FXTEST_INOS ?= $(wildcard tst/fxdatatest/*.ino)
FXTEST_NAMES = $(basename $(notdir $(FXTEST_INOS)))
FXTEST_SPIKE_INO ?=
FXTEST_SPIKE_INOS = $(sort $(FXTEST_SPIKE_INO) tst/fxdatatest/test_stack.ino)

fxtest: fxtest-headless

# Early device spike: pair the selected feature suite with the painted stack
# budget suite so increased call depth is caught before the full gate.
fxtest-spike:
	@test -n "$(FXTEST_SPIKE_INO)" || { echo "fxtest-spike: set FXTEST_SPIKE_INO=tst/fxdatatest/test_save.ino (or another suite)" >&2; exit 1; }
	@test -f "$(FXTEST_SPIKE_INO)" || { echo "fxtest-spike: suite not found at $(FXTEST_SPIKE_INO)" >&2; exit 1; }
	@test -n "$(ARDENS)" || { echo "fxtest-spike: ARDENS is unset; set ARDENS=/path/to/Ardens" >&2; exit 1; }
	@$(MAKE) --no-print-directory -j1 fxtest-headless FXTEST_INOS="$(FXTEST_SPIKE_INOS)"

fxtest-headless:
	@if [ -z "$(ARDENS)" ]; then \
		echo "fxtest-headless: SKIPPED (Ardens unavailable; set ARDENS=/path/to/Ardens to run serial device tests)"; \
	else \
		$(MAKE) --no-print-directory fxtest-headless-preflight fxtest-build fxtest-run; \
	fi

# Retained as an alias for callers that used the previous preflight target.
fxtest-preflight: fxtest-headless-preflight

fxtest-headless-preflight:
	@test -x "$(ARDENS)" || { echo "fxtest-headless: Ardens executable not found at $(ARDENS); build headless Ardens or set ARDENS=/path/to/Ardens" >&2; exit 1; }
	@test -f "$(FXDATA_BIN)" || { echo "fxtest-headless: FX data image missing at $(FXDATA_BIN); run make gen or set FXDATA_BIN=/path/to/fxdata.bin" >&2; exit 1; }
	@strings "$(ARDENS)" | grep -Fxq captureserial || { echo "fxtest-headless: BLOCKED (Ardens at $(ARDENS) lacks captureserial; install or build a headless-capable Ardens; the installed 0.24.4 bundle is incompatible)" >&2; exit 2; }

fxtest-build:
	@set -e; \
	for ino in $(FXTEST_NAMES); do \
		stage="$(FXTEST_BUILD_DIR)/$$ino"; \
		rm -rf "$$stage"; \
		mkdir -p "$$stage"; \
		cp -R src "$$stage/src"; \
		cp "tst/fxdatatest/$$ino.ino" "$$stage/"; \
		cp tst/fxdatatest/*.hpp "$$stage/"; \
		cp -R tst/fxdatatest/harness "$$stage/harness"; \
		cp tst/fxdatatest/generated/*.hpp "$$stage/"; \
		cp fxdata/generated/arena_demo_ids.hpp "$$stage/arena_demo_ids.hpp"; \
		mkdir -p "$$stage/fxdata/generated"; \
		cp fxdata/generated/arena_demo_ids.hpp "$$stage/fxdata/generated/arena_demo_ids.hpp"; \
		mkdir "$$stage/generated"; \
		for fixture in tst/fxdatatest/generated/*.hpp; do \
			name="$$(basename "$$fixture")"; \
			printf '#include "../%s"\n' "$$name" > "$$stage/generated/$$name"; \
		done; \
		echo $$ino; \
		if [ "$$ino" = "test_battle_compact" ]; then \
		    ARDUINO_BUILD_CACHE_PATH="$(ARDUINO_BUILD_CACHE_PATH)" $(ARDUINO_CLI) compile --fqbn "$(FQBN)" \
		        $(AVR_FXTEST_COMPACT_RESULT_BUILD_PROPERTIES) \
		        --build-path "$$stage/build" --output-dir "$$stage/output" "$$stage/$$ino.ino"; \
		elif [ "$$ino" = "test_battle_facts" ]; then \
		    ARDUINO_BUILD_CACHE_PATH="$(ARDUINO_BUILD_CACHE_PATH)" $(ARDUINO_CLI) compile --fqbn "$(FQBN)" \
		        $(AVR_FXTEST_NO_FACTS_BUILD_PROPERTIES) \
		        --build-path "$$stage/build" --output-dir "$$stage/output" "$$stage/$$ino.ino"; \
		elif [ "$$ino" = "test_battle_duel" ] || [ "$$ino" = "test_duel_session" ] || [ "$$ino" = "test_duel_resources" ] || [ "$$ino" = "test_duel_ui" ] || [ "$$ino" = "test_stack" ] || [ "$$ino" = "test_win_exp" ]; then \
		    ARDUINO_BUILD_CACHE_PATH="$(ARDUINO_BUILD_CACHE_PATH)" $(ARDUINO_CLI) compile --fqbn "$(FQBN)" \
		        $(AVR_FXTEST_DUEL_BUILD_PROPERTIES) \
		        --build-path "$$stage/build" --output-dir "$$stage/output" "$$stage/$$ino.ino"; \
		else \
		    ARDUINO_BUILD_CACHE_PATH="$(ARDUINO_BUILD_CACHE_PATH)" $(ARDUINO_CLI) compile --fqbn "$(FQBN)" \
		        $(AVR_FXTEST_BUILD_PROPERTIES) \
		        --build-path "$$stage/build" --output-dir "$$stage/output" "$$stage/$$ino.ino"; \
		fi; \
		./tools/check-fxtest-ram.sh "$$stage/build/$$ino.ino.elf" "$(AVR_SIZE)" "$(FXTEST_RAM_BUDGET)" "$$ino"; \
	done

fxtest-run:
	@failed=0; \
	for name in $(FXTEST_NAMES); do \
		stage="$(FXTEST_BUILD_DIR)/$$name"; \
		echo "=== $$name ==="; \
		cp -f "$(FXDATA_BIN)" "$$stage/fxdata.bin"; \
		ms="$(FXTEST_MS)"; \
		if [ "$$name" = "test_duel_resources" ]; then ms="$(FXTEST_MS_RESOURCES)"; fi; \
		if [ "$$name" = "test_save" ]; then ms="$(FXTEST_MS_SAVE)"; fi; \
		if [ "$$name" = "test_save_cuts" ]; then ms="$(FXTEST_MS_SAVE)"; fi; \
		if out="$$($(ARDENS) captureserial=$$ms fxport=d1 display=ssd1306 file=$$stage/output/$$name.ino.hex file=$$stage/fxdata.bin 2>&1)"; then \
			runner_status=0; \
		else \
			runner_status=$$?; \
		fi; \
		printf '%s\n' "$$out"; \
		normalized_out="$$(printf '%s\n' "$$out" | tr -d '\r')"; \
		if [ -z "$$out" ]; then \
			echo "$$name: FAIL (no serial: crash, hang, or ROM not loaded)"; \
			failed=1; \
		elif printf '%s\n' "$$normalized_out" | grep -qx 'F'; then \
			echo "$$name: FAIL"; \
			failed=1; \
		elif [ "$$runner_status" -ne 0 ]; then \
			echo "$$name: FAIL (Ardens exited $$runner_status)"; \
			failed=1; \
		elif printf '%s\n' "$$normalized_out" | grep -qx 'P'; then \
			echo "$$name: PASS"; \
		else \
			echo "$$name: FAIL (missing P/F marker; capture may be truncated, raise FXTEST_MS)"; \
			failed=1; \
		fi; \
	done; \
	test "$$failed" -eq 0

new-fxtest:
	@set -e; \
	name="$(NAME)"; \
	test -n "$$name" || { echo "new-fxtest: NAME is required" >&2; exit 1; }; \
	case "$$name" in *[!A-Za-z0-9_]* ) echo "new-fxtest: NAME must use only letters, digits, and underscores" >&2; exit 1;; esac; \
	header="tst/fxdatatest/$${name}_test.hpp"; \
	sketch="tst/fxdatatest/test_$${name}.ino"; \
	test ! -e "$$header" && test ! -e "$$sketch" || { echo "new-fxtest: $$name already exists" >&2; exit 1; }; \
	sed "s/@NAME@/$$name/g" tst/fxdatatest/harness/fxtest-suite.hpp.in > "$$header"; \
	sed "s/@NAME@/$$name/g" tst/fxdatatest/harness/test_fxtest.ino.in > "$$sketch"; \
	echo "new-fxtest: created $$header and $$sketch"
