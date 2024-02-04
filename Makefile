build:
	arduino-cli compile --fqbn "arduboy-homemade:avr:arduboy" --optimize-for-debug  --output-dir dist

gen:
	./tools/gen.sh

sim:
	g++  -g -std=c++17 simulator/creature/Creature.cpp simulator/opponent/Opponent.cpp simulator/player/Player.cpp src/action/Action.cpp simulator/Battle.cpp simulator/main.cpp  -o simulator/simu.o