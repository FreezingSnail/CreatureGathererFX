#!/bin/bash
#python3 tools/movelistConverter.py > src/fxdata/data/movelists.txt

cp -r images fxdata/
python3 Arduboy-Python-Utilities/fxdata-build.py fxdata/fxdata.txt
rm -rf fxdata/images
mv fxdata/fxdata.h src/fxdata.h
mv fxdata/fxdata.bin dist
mv fxdata/fxdata-data.bin dist
