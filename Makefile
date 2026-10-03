.PHONY: help setup doctor plant test test-debug testvm testvm-debug gen gen-data gen-sprites gen-fixtures pack full build mini ram run dev check final-gate verify-generated test-manifest test-generated-libs test-doctor test-fxtest-ram test-avr-build-budget fxtest fxtest-headless fxtest-spike fxtest-preflight fxtest-headless-preflight fxtest-build fxtest-run new-fxtest

# Public command API. Override tool, board, and output variables per workspace/CI.
CXX ?= g++
ARDUINO_CLI ?= arduino-cli
FQBN ?= arduboy-homemade:avr:arduboy-fx
MINI_FQBN ?= arduboy-homemade:avr:arduboy-mini
# Shared AVR toolchain properties for shipping and device-test builds. Relaxed
# linking plus shared function prologues save flash without changing measured
# static RAM or painted stack. Shipping builds add the no-USB entry separately.
AVR_RELAX_FLAGS ?= -mrelax -mcall-prologues
# Instrument FX test firmware with the logical read counter while preserving
# the stock USB main used by the serial P/F harness.
AVR_FXTEST_CPP_FLAGS ?= $(AVR_RELAX_FLAGS) -DFX_READ_COUNTER
AVR_FXTEST_BUILD_PROPERTIES ?= --build-property "compiler.cpp.extra_flags=$(AVR_FXTEST_CPP_FLAGS)" \
	--build-property "compiler.c.extra_flags=$(AVR_RELAX_FLAGS)" \
	--build-property "compiler.c.elf.extra_flags=$(AVR_RELAX_FLAGS)"
# Only shipping FX/Mini sketches compile their no-USB main. Device-test
# sketches keep the stock core main so Serial P/F output remains available.
AVR_SHIPPING_CPP_FLAGS ?= $(AVR_RELAX_FLAGS) -DCGFX_SHIPPING_NO_USB
AVR_SHIPPING_BUILD_PROPERTIES ?= --build-property "compiler.cpp.extra_flags=$(AVR_SHIPPING_CPP_FLAGS)" \
	--build-property "compiler.c.extra_flags=$(AVR_RELAX_FLAGS)" \
	--build-property "compiler.c.elf.extra_flags=$(AVR_RELAX_FLAGS)"
# Leave 5,986 B above the current 18,014 B FX image for feature growth. Keep
# 400 B static free as an independent guard; it does not measure painted stack.
AVR_FLASH_BUDGET ?= 24000
AVR_STATIC_RAM_BUDGET ?= 2160
BUILD_DIR ?= build
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
ARDENS ?=
FXTEST_MS ?= 3000
FXTEST_RAM_BUDGET ?= 2160
FXTEST_BUILD_DIR ?= $(BUILD_DIR)/fxtest
FINAL_GATE_LOG_DIR ?= $(BUILD_DIR)/final-gate
HOST_TEST_BIN ?= $(BUILD_DIR)/tests/host
WORLD_TEST_BIN ?= $(BUILD_DIR)/tests/world
VM_TEST_BIN ?= $(BUILD_DIR)/tests/vm
GENERATED_TEST_BIN ?= $(BUILD_DIR)/tests/generated
RAM_ELF ?= $(BUILD_DIR)/CreatureGathererFX.ino.elf
AVR_NM ?= $(shell command -v avr-nm 2>/dev/null || find "$(HOME)/Library/Arduino15/packages" "$(HOME)/.arduino15/packages" -type f -path '*/tools/avr-gcc/*/bin/avr-nm' -print -quit 2>/dev/null)
AVR_SIZE ?= $(shell command -v avr-size 2>/dev/null || find "$(HOME)/Library/Arduino15/packages" "$(HOME)/.arduino15/packages" -type f -path '*/tools/avr-gcc/*/bin/avr-size' -print -quit 2>/dev/null)

CPPFLAGS ?= -I.
CXXFLAGS ?= -std=c++17 -w -O0 -g3
TEST_CPPFLAGS = -I tst/host
TEST_FLAGS = -DTEST
DEBUG_FLAGS = -DDEBUG

help:
	@printf '%s\n' \
		'CreatureGathererFX Make targets:' \
		'  setup    print non-mutating first-run guidance; prerequisite: make' \
		'  doctor   report local tool readiness; prerequisite: tools/doctor.sh' \
		'  gen      package committed/generated FX inputs; prerequisite: cgfx-tools; output: $(DIST_DIR)/fxdata.bin' \
		'  test     run fast host C++ tests; prerequisite: $(CXX); output: $(HOST_TEST_BIN)' \
		'  testvm   run fast ScriptVM C++ tests; prerequisite: $(CXX); output: $(VM_TEST_BIN)' \
		'  build    compile Arduboy FX sketch; prerequisite: $(ARDUINO_CLI); output: $(BUILD_DIR)' \
		'  ram      build and report FX flash/RAM plus largest static symbols; prerequisite: $(ARDUINO_CLI), avr-size, avr-nm' \
		'  test-avr-build-budget exercise the fail-closed shipping flash/RAM parser' \
		'  final-gate  run the full check then the shipping RAM report; requires ARDENS' \
		'  run      launch Ardens with the sketch, FX data, and FX save images; prerequisite: ARDENS' \
		'  dev      regenerate FX data, rebuild, then launch Ardens (gen + run); prerequisite: cgfx-tools, ARDENS' \
		'  check    run generation, host tests, VM tests, then optional FX runtime tests' \
		'  test-manifest run permanent generated-artifact tests' \
		'  test-generated-libs check generated libs against the packed image; prerequisite: $(CXX)' \
		'  test-pack-parity verify native packed-image SHA-256 baseline' \
		'  test-doctor run permanent setup-diagnostic tests' \
		'  test-fxtest-ram exercise the fail-closed device ELF static-RAM guard' \
		'  fxtest   alias for fxtest-headless; skips only when ARDENS is unset' \
		'  fxtest-headless  run every FX device sketch through Ardens serial capture; blocks unsupported Ardens' \
		'  fxtest-spike  run one selected FX suite plus test_stack; set FXTEST_SPIKE_INO and ARDENS' \
		'' \
		'Overrides: CXX, ARDUINO_CLI, FQBN, BUILD_DIR, ARDUINO_BUILD_PATH, ARDUINO_BUILD_CACHE_PATH, DIST_DIR, FXDATA_BIN, ARDENS, FXTEST_MS, FXTEST_RAM_BUDGET, AVR_FLASH_BUDGET, AVR_STATIC_RAM_BUDGET, RAM_ELF, AVR_SIZE, AVR_NM, AVR_RELAX_FLAGS, AVR_FXTEST_BUILD_PROPERTIES, AVR_FXTEST_CPP_FLAGS, AVR_SHIPPING_BUILD_PROPERTIES, AVR_SHIPPING_CPP_FLAGS.'

setup:
	@printf '%s\n' \
		'setup: guidance only; no installs or Arduino configuration changes.' \
		'1. Install g++, make, and arduino-cli with your platform package manager.' \
		'2. Review and run: SETUP_APPLY=1 tools/setup.sh' \
		'3. Verify readiness: make doctor' \
		'For manual setup, see each remedy printed by make doctor.'

doctor:
	@CXX="$(CXX)" ARDUINO_CLI="$(ARDUINO_CLI)" ARDENS="$(ARDENS)" ./tools/doctor.sh

# Common source files for main tests
TEST_SOURCES = tst/src/Arduboy2Host.cpp \
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
	src/opponent/Opponent.cpp \
	src/action/Action.cpp \
	src/lib/MenuStack.cpp \
	src/lib/ListView.cpp \
	src/lib/BattleEventPlayer.cpp \
	src/engine/battle/Battle.cpp \
	src/engine/battle/Damage.cpp \
	src/engine/battle/Effects.cpp \
	src/engine/battle/Resolve.cpp \
	src/engine/battle/BattleSetup.cpp \
	src/engine/battle/BattlePresenter.cpp \
	src/engine/ModeState.cpp \
	src/engine/world/World.cpp \
	src/engine/world/Encounter.cpp \
	src/engine/world/StepEvent.cpp \
	src/GameState.cpp \
	src/flags/flag_bit_array.cpp \
	tst/main.cpp

# Common source files for VM tests
TESTVM_SOURCES = src/vm/ScriptVM.cpp \
	src/engine/menu/DialogQueue.cpp \
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

build:
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

mini:
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

# Full interactive loop: fresh FX data, fresh sketch, then Ardens. Sequenced
# through a sub-make so `make -j dev` cannot launch before generation finishes.
dev: gen
	@$(MAKE) --no-print-directory run

gen: gen-data gen-sprites gen-fixtures pack

gen-data:
	@set -e; \
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

pack:
	@set -e; \
	mkdir -p "$(BUILD_DIR)" "$(DIST_DIR)"; \
	stage="$$(cd "$$(mktemp -d "$(BUILD_DIR)/cgfx-pack.XXXXXX")" && pwd -P)"; \
	tar -cf - --exclude './.git' --exclude './build' --exclude './dist' . | tar -xf - -C "$$stage"; \
	cgfx-tools --project "$$stage/cgfx-project.json" --pack --layout "$$stage/fxlayout.toml"; \
	mv -f "$$stage/src/fxdata.h" src/fxdata.h; \
	mv -f "$$stage/dist/fxdata.bin" "$(DIST_DIR)/fxdata.bin"; \
	mv -f "$$stage/dist/fxdata-data.bin" "$(DIST_DIR)/fxdata-data.bin"; \
	mv -f "$$stage/dist/fxdata-save.bin" "$(DIST_DIR)/fxdata-save.bin"; \
	./tools/record-fxdata-manifest.sh "$(FX_LAYOUT)" "$(FXDATA_MANIFEST)"; \
	./tools/assert-fxdata-manifest.sh "$(FX_LAYOUT)" "$(FXDATA_MANIFEST)"

check: gen test testvm test-manifest test-generated-libs test-fxtest-ram test-avr-build-budget verify-generated build fxtest

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

# Generated-library integration tests: packed image <-> src/fxdata.h <-> the
# generated sources, plus generated-header drift and text-block framing.
test-generated-libs:
	@mkdir -p "$(dir $(GENERATED_TEST_BIN))"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tst/generated/generated_libs_test.cpp -o "$(GENERATED_TEST_BIN)"
	"$(GENERATED_TEST_BIN)" . "$(FX_LAYOUT)" src/fxdata.h "$(FXDATA_DIST_DIR)/fxdata-data.bin"
	./tools/tests/generated-libs_test.sh
	./tools/tests/first-unqualified-alias_test.sh

test-doctor:
	./tools/tests/doctor_test.sh

test-fxtest-ram:
	./tools/tests/fxtest-ram_test.sh

test-avr-build-budget:
	./tools/tests/avr-build-budget_test.sh

sim:
	g++  -g -std=c++17 simulator/creature/Creature.cpp simulator/opponent/Opponent.cpp simulator/player/Player.cpp src/action/Action.cpp simulator/Battle.cpp simulator/main.cpp  -o simulator/simu.o

test-pack-parity:
	./tools/tests/pack-parity_test.sh

test:
	$(call run_test,,$(TEST_FLAGS),$(TEST_SOURCES),$(HOST_TEST_BIN))
	$(call run_test,-DWORLD_TEST,$(TEST_FLAGS),$(WORLD_TEST_SOURCES),$(WORLD_TEST_BIN))

test-debug:
	$(call run_test,$(DEBUG_FLAGS),$(TEST_FLAGS),$(TEST_SOURCES),$(HOST_TEST_BIN))
	$(call run_test,-DWORLD_TEST,$(TEST_FLAGS),$(WORLD_TEST_SOURCES),$(WORLD_TEST_BIN))

WORLD_TEST_SOURCES = src/GameState.cpp src/engine/world/World.cpp src/engine/world/Encounter.cpp src/engine/world/StepEvent.cpp src/plants/PlantGamestate.cpp src/plants/PlantStage.cpp src/plants/PlantPair.cpp src/flags/flag_bit_array.cpp tst/src/random.cpp tst/world_test_main.cpp

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
		mkdir "$$stage/generated"; \
		for fixture in tst/fxdatatest/generated/*.hpp; do \
			name="$$(basename "$$fixture")"; \
			printf '#include "../%s"\n' "$$name" > "$$stage/generated/$$name"; \
		done; \
		echo $$ino; \
		ARDUINO_BUILD_CACHE_PATH="$(ARDUINO_BUILD_CACHE_PATH)" $(ARDUINO_CLI) compile --fqbn "$(FQBN)" \
		    $(AVR_FXTEST_BUILD_PROPERTIES) \
		    --build-path "$$stage/build" \
		    --output-dir "$$stage/output" \
		    "$$stage/$$ino.ino"; \
		./tools/check-fxtest-ram.sh "$$stage/build/$$ino.ino.elf" "$(AVR_SIZE)" "$(FXTEST_RAM_BUDGET)" "$$ino"; \
	done

fxtest-run:
	@failed=0; \
	for name in $(FXTEST_NAMES); do \
		stage="$(FXTEST_BUILD_DIR)/$$name"; \
		echo "=== $$name ==="; \
		cp -f "$(FXDATA_BIN)" "$$stage/fxdata.bin"; \
		if out="$$($(ARDENS) captureserial=$(FXTEST_MS) fxport=d1 display=ssd1306 file=$$stage/output/$$name.ino.hex file=$$stage/fxdata.bin 2>&1)"; then \
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
