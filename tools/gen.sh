#!/bin/bash
#python3 tools/movelistConverter.py > src/fxdata/data/movelists.txt

mkdir -p fxdata/generated
python3 tools/text2bmp.py --font images/arduboyFont_6x8.png --input data/text/strings.txt --output_dir fxdata/generated
python3 tools/convert-sprite.py ../images -s 4 -o ../fxdata/
cp -r images fxdata/
python3 Arduboy-Python-Utilities/fxdata-build.py fxdata/fxdata.txt
rm -rf fxdata/images
mv fxdata/fxdata.h src/fxdata.h
mv fxdata/fxdata.bin dist
mv fxdata/fxdata-data.bin dist
rm -rf fxdata/generated
