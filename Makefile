build:
	arduino-cli compile --fqbn "arduboy-homemade:avr:arduboy" --optimize-for-debug  --output-dir . 

gen:
	./gen.sh
	python3 convert-sprite.py src/fxdata/images/ -s 4 -o src/fxdata/
	python3 Arduboy-Python-Utilities/fxdata-build.py src/fxdata/fxdata.txt

sim:
	g++  -g -std=c++17 simulator/creature/Creature.cpp simulator/opponent/Opponent.cpp simulator/player/Player.cpp src/action/Action.cpp simulator/Battle.cpp simulator/main.cpp  -o simulator/simu.o