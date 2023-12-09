build:
	arduino-cli compile --fqbn "arduboy-homemade:avr:arduboy" --optimize-for-debug  --output-dir .

gen:
	./gen.sh
	python3 Arduboy-Python-Utilities/fxdata-build.py src/fxdata/fxdata.txt

sim:
	g++   simulator/creature/Creature.cpp simulator/opponent/Opponent.cpp simulator/player/Player.cpp src/action/Action.cpp simulator/Battle.cpp simulator/main.cpp  -o simulator/simu.o