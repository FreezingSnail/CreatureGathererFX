.PHONY : plant

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
	g++ -std=c++17 -I/src \
		tst/src/ReadData.cpp \
		tst/src/DialogMenu.cpp \
		src/plants/PlantStage.cpp \
		src/plants/PlantPair.cpp \
		src/creature/Creature.cpp \
		src/player/Player.cpp \
		src/opponent/Opponent.cpp \
		src/action/Action.cpp \
		src/lib/MenuStack.cpp \
		src/lib/BattleEventPlayer.cpp \
		src/engine/battle/Battle.cpp \
		tst/main.cpp \
		-w \
		-O0 -g3 \
		-o tst/test.o && ./tst/test.o && rm tst/test.o

fxtest:
	cp -r src tst/fxdatatest
	cd tst/fxdatatest && arduino-cli compile --fqbn "arduboy-homemade:avr:arduboy-fx" --optimize-for-debug --output-dir .
#	rm -r tst/fxdatatest/src
#	open bin/Ardens.app  file="tst/fxdatatest/fxdatatest.ino.elf" file="dist/fxdata.bin"