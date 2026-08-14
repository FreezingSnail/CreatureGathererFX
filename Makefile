.PHONY : plant test test-debug testvm testvm-debug fxtest

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
	arduino-cli compile --fqbn "arduboy-homemade:avr:arduboy-fx" --optimize-for-debug  --output-dir dist

mini:
	arduino-cli compile --fqbn "arduboy-homemade:avr:arduboy-mini" --optimize-for-debug  --output-dir dist

gen:
	./tools/gen.sh

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

FXDATA_BIN        = dist/fxdata.bin
ARDENS            = /Users/connorfranc/code/Ardens/build/Ardens.app/Contents/MacOS/Ardens
FXTEST_MS         = 3000
FXTEST_INOS       = $(wildcard tst/fxdatatest/test_*.ino)
FXTEST_NAMES      = $(basename $(notdir $(FXTEST_INOS)))

.PHONY: fxtest fxtest-build fxtest-run

fxtest: fxtest-build fxtest-run

fxtest-build:
	for ino in $(FXTEST_NAMES); do \
		rm -rf tst/fxdatatest/$$ino && mkdir tst/fxdatatest/$$ino ; \
		cp -r src tst/fxdatatest/$$ino/src ; \
		cp tst/fxdatatest/$$ino.ino tst/fxdatatest/$$ino/ ; \
		cp tst/fxdatatest/*.hpp tst/fxdatatest/$$ino/ ; \
		echo $$ino ; \
		arduino-cli compile --fqbn "arduboy-homemade:avr:arduboy-fx" \
		    --optimize-for-debug --output-dir tst/fxdatatest \
		    tst/fxdatatest/$$ino/$$ino.ino ; \
	done

fxtest-run:
	@for name in $(FXTEST_NAMES); do \
		echo "=== $$name ==="; \
		out="$$($(ARDENS) captureserial=$(FXTEST_MS) fxport=d1 display=ssd1306 file=tst/fxdatatest/$$name.ino.hex file=$(FXDATA_BIN) 2>&1)"; \
		printf '%s\n' "$$out"; \
		if printf '%s' "$$out" | grep -q "P"; then \
			echo "$$name: PASS"; \
		else \
			echo "$$name: FAIL (missing 'P' marker)"; \
			exit 1; \
		fi; \
	done
