.PHONY: help setup doctor plant test test-debug testvm testvm-debug gen gen-data gen-sprites gen-fixtures pack full build mini check verify-generated test-manifest test-doctor fxtest fxtest-preflight fxtest-build fxtest-run new-fxtest

# Public command API. Override tool, board, and output variables per workspace/CI.
CXX ?= g++
ARDUINO_CLI ?= arduino-cli
FQBN ?= arduboy-homemade:avr:arduboy-fx
MINI_FQBN ?= arduboy-homemade:avr:arduboy-mini
BUILD_DIR ?= build
DIST_DIR ?= dist
FXDATA_BIN ?= $(DIST_DIR)/fxdata.bin
FXDATA_FILE ?= fxdata/fxdata.txt
FXDATA_MANIFEST ?= fxdata/generated/manifest.json
FXDATA_DIST_DIR ?= $(DIST_DIR)
ARDENS ?=
FXTEST_MS ?= 3000
FXTEST_BUILD_DIR ?= $(BUILD_DIR)/fxtest
HOST_TEST_BIN ?= $(BUILD_DIR)/tests/host
VM_TEST_BIN ?= $(BUILD_DIR)/tests/vm

CPPFLAGS ?= -I.
CXXFLAGS ?= -std=c++17 -w -O0 -g3
TEST_FLAGS = -DTEST
DEBUG_FLAGS = -DDEBUG

help:
	@printf '%s\n' \
		'CreatureGathererFX Make targets:' \
		'  setup    print non-mutating first-run guidance; prerequisite: make' \
		'  doctor   report local tool readiness; prerequisite: tools/doctor.sh' \
		'  gen      generate FX data/assets; prerequisites: cgfx-tools, python3 for the transitional legacy bridge; output: $(DIST_DIR)/fxdata.bin' \
		'  test     run fast host C++ tests; prerequisite: $(CXX); output: $(HOST_TEST_BIN)' \
		'  testvm   run fast ScriptVM C++ tests; prerequisite: $(CXX); output: $(VM_TEST_BIN)' \
		'  build    compile Arduboy FX sketch; prerequisite: $(ARDUINO_CLI); output: $(BUILD_DIR)' \
		'  check    run generation, host tests, VM tests, then optional FX runtime tests' \
		'  test-manifest run permanent generated-artifact tests' \
		'  test-pack-parity compare cgfx-tools packing with fxdata-build.py' \
		'  test-doctor run permanent setup-diagnostic tests' \
		'  fxtest   run FX device tests; optional: ARDENS=/path/to/Ardens; skips when unavailable' \
		'' \
		'Overrides: CXX, ARDUINO_CLI, FQBN, BUILD_DIR, DIST_DIR, FXDATA_BIN, ARDENS, FXTEST_MS.'

setup:
	@printf '%s\n' \
		'setup: guidance only; no installs or Arduino configuration changes.' \
		'1. Install Python 3 for the transitional cgfx-tools legacy bridge, plus g++, make, and arduino-cli with your platform package manager.' \
		'2. Review and run: SETUP_APPLY=1 tools/setup.sh' \
		'3. Verify readiness: make doctor' \
		'For manual setup, see each remedy printed by make doctor.'

doctor:
	@CXX="$(CXX)" ARDUINO_CLI="$(ARDUINO_CLI)" ARDENS="$(ARDENS)" ./tools/doctor.sh

# Common source files for main tests
TEST_SOURCES = tst/src/ReadData.cpp \
	tst/src/DialogMenu.cpp \
	tst/src/random.cpp \
	tst/src/FlashBackendFake.cpp \
	src/save/SaveFile.cpp \
	src/plants/PlantStage.cpp \
	src/plants/PlantPair.cpp \
	src/creature/Creature.cpp \
	src/player/Player.cpp \
	src/opponent/Opponent.cpp \
	src/action/Action.cpp \
	src/lib/MenuStack.cpp \
	src/lib/BattleEventPlayer.cpp \
	src/engine/battle/Battle.cpp \
	src/GameState.cpp \
	src/flags/flag_bit_array.cpp \
	tst/main.cpp

# Common source files for VM tests
TESTVM_SOURCES = src/vm/ScriptVM.cpp \
	src/GameState.cpp \
	src/flags/flag_bit_array.cpp \
	tst/script_tests/action_test.cpp \
	tst/script_tests/main.cpp

# Function to run tests in target-owned output directories.
define run_test
	@mkdir -p "$(dir $(4))"
	$(CXX) $(1) $(CPPFLAGS) $(CXXFLAGS) $(2) $(3) -o "$(4)" && "$(4)"
endef

full: gen build

build:
	@mkdir -p "$(BUILD_DIR)"
	$(ARDUINO_CLI) compile --fqbn "$(FQBN)" --optimize-for-debug --output-dir "$(BUILD_DIR)" .

mini:
	@mkdir -p "$(BUILD_DIR)"
	$(ARDUINO_CLI) compile --fqbn "$(MINI_FQBN)" --optimize-for-debug --output-dir "$(BUILD_DIR)" .

gen: gen-data gen-fixtures gen-sprites pack

gen-data:
	@set -e; \
	tool="$$(./tools/cgfx-tools.sh)"; \
	"$$tool" --project cgfx-project.json; \
	"$$tool" --arena-csv data/arena.csv --arena-output fxdata/generated; \
	"$$tool" --type-table-csv data/typetable.csv --type-table-output fxdata/generated; \

gen-fixtures:
	@set -e; \
	tool="$$(./tools/cgfx-tools.sh)"; \
	"$$tool" --project cgfx-project.json --emit-fixtures; \
	./tools/emit-rust-teams.sh; \
	./tools/emit-tool-version-stamp.sh

gen-sprites:
	@set -e; \
	rm -f fxdataSprites.txt fxdata/battleEffectsSprites.txt fxdata/generatedSprites.txt; \
	mkdir -p fxdata/generated/images; \
	python3 tools/text2bmp.py --font ArduboyFXFonts/Fontbitmaps/Font4x6/Font_5x6.png --input data/text/strings.txt --output_dir fxdata/generated/images --mode joined --greyscale; \
	python3 tools/convert-sprite.py images -s 4 -o fxdata/; \
	python3 tools/convert-sprite.py images/battleEffects -s 4 -o fxdata/battleEffects/; \
	python3 tools/convert-sprite.py fxdata/generated/images -s 4 -o fxdata/generated/; \
	printf '\n' >> fxdata/generated/Sprites.txt; \
	cat fxdata/generated/images/string_images.txt >> fxdata/generated/Sprites.txt; \
	rm -rf fxdata/generated/images

pack:
	@set -e; \
	mkdir -p "$(BUILD_DIR)" "$(DIST_DIR)"; \
	stage="$$(cd "$$(mktemp -d "$(BUILD_DIR)/cgfx-pack.XXXXXX")" && pwd -P)"; \
	tar -cf - --exclude './.git' --exclude './build' --exclude './dist' . | tar -xf - -C "$$stage"; \
	tool="$$(./tools/cgfx-tools.sh)"; \
	"$$tool" --project "$$stage/cgfx-project.json" --pack --layout "$$stage/fxlayout.toml"; \
	mv -f "$$stage/src/fxdata.h" src/fxdata.h; \
	mv -f "$$stage/dist/fxdata.bin" "$(DIST_DIR)/fxdata.bin"; \
	mv -f "$$stage/dist/fxdata-data.bin" "$(DIST_DIR)/fxdata-data.bin"; \
	mv -f "$$stage/dist/fxdata-save.bin" "$(DIST_DIR)/fxdata-save.bin"; \
	./tools/record-fxdata-manifest.sh fxdata/fxdata.txt fxdata/generated/manifest.json; \
	./tools/assert-fxdata-manifest.sh fxdata/fxdata.txt fxdata/generated/manifest.json

check: gen test testvm test-manifest verify-generated fxtest

verify-generated:
	@if test ! -e "$(FXDATA_DIST_DIR)/fxdata-data.bin" && test ! -e "$(FXDATA_DIST_DIR)/fxdata.bin"; then \
		echo 'verify-generated: image not built; run make gen to build packed FX artifacts'; \
		./tools/assert-fxdata-manifest.sh --skip-image "$(FXDATA_FILE)" "$(FXDATA_MANIFEST)"; \
	else \
		./tools/assert-fxdata-manifest.sh "$(FXDATA_FILE)" "$(FXDATA_MANIFEST)"; \
	fi

test-manifest:
	./tools/tests/fxdata-manifest_test.sh

test-doctor:
	./tools/tests/doctor_test.sh

sim:
	g++  -g -std=c++17 simulator/creature/Creature.cpp simulator/opponent/Opponent.cpp simulator/player/Player.cpp src/action/Action.cpp simulator/Battle.cpp simulator/main.cpp  -o simulator/simu.o

test-pack-parity:
	./tools/tests/pack-parity_test.sh

test:
	$(call run_test,,$(TEST_FLAGS),$(TEST_SOURCES),$(HOST_TEST_BIN))

test-debug:
	$(call run_test,$(DEBUG_FLAGS),$(TEST_FLAGS),$(TEST_SOURCES),$(HOST_TEST_BIN))

testvm:
	$(call run_test,,$(TEST_FLAGS),$(TESTVM_SOURCES),$(VM_TEST_BIN))

testvm-debug:
	$(call run_test,$(DEBUG_FLAGS),$(TEST_FLAGS),$(TESTVM_SOURCES),$(VM_TEST_BIN))

FXTEST_INOS ?= $(wildcard tst/fxdatatest/test_*.ino)
FXTEST_NAMES = $(basename $(notdir $(FXTEST_INOS)))

fxtest:
	@if [ -z "$(ARDENS)" ]; then \
		echo "fxtest: SKIPPED (Ardens unavailable; set ARDENS=/path/to/Ardens to run device tests)"; \
	else \
		$(MAKE) --no-print-directory fxtest-preflight fxtest-build fxtest-run; \
	fi

fxtest-preflight:
	@test -x "$(ARDENS)" || { echo "fxtest: Ardens executable not found at $(ARDENS); build Ardens or set ARDENS=/path/to/Ardens" >&2; exit 1; }
	@test -f "$(FXDATA_BIN)" || { echo "fxtest: FX data image missing at $(FXDATA_BIN); run make gen or set FXDATA_BIN=/path/to/fxdata.bin" >&2; exit 1; }

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
		$(ARDUINO_CLI) compile --fqbn "$(FQBN)" \
		    --optimize-for-debug --output-dir "$$stage/output" \
		    "$$stage/$$ino.ino"; \
	done

fxtest-run:
	@failed=0; \
	for name in $(FXTEST_NAMES); do \
		echo "=== $$name ==="; \
		if out="$$($(ARDENS) captureserial=$(FXTEST_MS) fxport=d1 display=ssd1306 file=$(FXTEST_BUILD_DIR)/$$name/output/$$name.ino.hex file=$(FXDATA_BIN) 2>&1)"; then \
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
