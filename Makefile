.PHONY : plant test test-debug testvm testvm-debug

# Common compiler flags
CXX_FLAGS = -std=c++17 -I/src -w -O0 -g3
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
	tst/script_tests/main.cpp

# Function to run tests
define run_test
	g++ $(1) $(CXX_FLAGS) $(2) -o tst/test.o && ./tst/test.o && rm tst/test.o
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
	$(call run_test,,$(TEST_SOURCES))

test-debug:
	$(call run_test,$(DEBUG_FLAGS),$(TEST_SOURCES))

testvm:
	$(call run_test,,$(TESTVM_SOURCES))

testvm-debug:
	$(call run_test,$(DEBUG_FLAGS),$(TESTVM_SOURCES))

fxtest:
	cp -r src tst/fxdatatest
	cd tst/fxdatatest && arduino-cli compile --fqbn "arduboy-homemade:avr:arduboy-fx" --optimize-for-debug --output-dir .
#	rm -r tst/fxdatatest/src
#	open bin/Ardens.app  file="tst/fxdatatest/fxdatatest.ino.elf" file="dist/fxdata.bin"
