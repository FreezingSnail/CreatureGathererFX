.PHONY : plant test test-debug testvm testvm-debug gen gen-data gen-sprites gen-fixtures pack full check fxtest fxtest-preflight fxtest-build fxtest-run new-fxtest

# Common compiler flags
CXX_FLAGS = -std=c++17 -I/src -w -O0 -g3
TEST_FLAGS = -DTEST
DEBUG_FLAGS = -DDEBUG

# Common source files for main tests
TEST_SOURCES = tst/src/ReadData.cpp \
	tst/src/DialogMenu.cpp \
	tst/src/random.cpp \
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

# Function to run tests
define run_test
	g++ $(1) $(CXX_FLAGS) $(2) $(3) -o tst/test.o && ./tst/test.o && rm tst/test.o
endef

full: gen build

build:
	arduino-cli compile --fqbn "arduboy-homemade:avr:arduboy-fx" --optimize-for-debug --output-dir dist

mini:
	arduino-cli compile --fqbn "arduboy-homemade:avr:arduboy-mini" --optimize-for-debug --output-dir dist

gen: gen-data gen-fixtures gen-sprites pack

gen-data:
	@set -e; \
	tool="$$(./tools/cgfx-tools.sh)"; \
	"$$tool" --project cgfx-project.json; \
	"$$tool" --arena-csv data/arena.csv --arena-output fxdata/generated; \
	"$$tool" --type-table-csv data/typetable.csv --type-table-output fxdata/generated; \
	sed '$$s/^}$$/} namespace_end/' fxdata/generated/moves.hpp > fxdata/generated/moves.txt

gen-fixtures:
	@set -e; \
	tool="$$(./tools/cgfx-tools.sh)"; \
	"$$tool" --project cgfx-project.json --emit-fixtures; \
	./tools/emit-rust-teams.sh; \
	./tools/emit-tool-version-stamp.sh

gen-sprites:
	@set -e; \
	mkdir -p fxdata/generated/images; \
	python3 tools/text2bmp.py --font ArduboyFXFonts/Fontbitmaps/Font4x6/Font_5x6.png --input data/text/strings.txt --output_dir fxdata/generated/images --mode joined --greyscale; \
	python3 tools/convert-sprite.py ../images -s 4 -o ../fxdata/; \
	python3 tools/convert-sprite.py ../images/battleEffects -s 4 -o ../fxdata/battleEffects/; \
	python3 tools/convert-sprite.py ../fxdata/generated/images -s 4 -o ../fxdata/generated/; \
	cat fxdata/generated/images/string_images.txt >> fxdata/generated/Sprites.txt; \
	rm -rf fxdata/generated/images

pack:
	@set -e; \
	./tools/record-fxdata-manifest.sh fxdata/fxdata.txt fxdata/generated/manifest.json; \
	./tools/assert-fxdata-manifest.sh fxdata/fxdata.txt fxdata/generated/manifest.json; \
	python3 Arduboy-Python-Utilities/fxdata-build.py fxdata/fxdata.txt; \
	mkdir -p dist; \
	mv -f fxdata/fxdata.h src/fxdata.h; \
	mv -f fxdata/fxdata.bin dist; \
	mv -f fxdata/fxdata-data.bin dist

check: gen test fxtest

sim:
	g++  -g -std=c++17 simulator/creature/Creature.cpp simulator/opponent/Opponent.cpp simulator/player/Player.cpp src/action/Action.cpp simulator/Battle.cpp simulator/main.cpp  -o simulator/simu.o

test:
	$(call run_test,,$(TEST_FLAGS),$(TEST_SOURCES))

test-debug:
	$(call run_test,$(DEBUG_FLAGS),$(TEST_FLAGS),$(TEST_SOURCES))

testvm:
	$(call run_test,,$(TEST_FLAGS),$(TESTVM_SOURCES))

testvm-debug:
	$(call run_test,$(DEBUG_FLAGS),$(TEST_FLAGS),$(TESTVM_SOURCES))

FXDATA_BIN        ?= dist/fxdata.bin
ARDENS            ?=
FXTEST_MS         ?= 3000
FXTEST_BUILD_DIR  ?= build/fxtest
FXTEST_INOS       = $(wildcard tst/fxdatatest/test_*.ino)
FXTEST_NAMES      = $(basename $(notdir $(FXTEST_INOS)))

fxtest:
	@if [ -z "$(ARDENS)" ] || [ ! -x "$(ARDENS)" ]; then \
		echo "fxtest: SKIPPED (Ardens unavailable; set ARDENS=/path/to/Ardens to run device tests)"; \
	else \
		$(MAKE) fxtest-preflight fxtest-build fxtest-run; \
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
		arduino-cli compile --fqbn "arduboy-homemade:avr:arduboy-fx" \
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
