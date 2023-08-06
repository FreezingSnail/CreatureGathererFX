build:
	arduino-cli compile --fqbn "arduboy-homemade:avr:arduboy" --optimize-for-debug  --output-dir .

gen:
	python3 Arduboy-Python-Utilities/fxdata-build.py src/fxdata/fxdata.txt