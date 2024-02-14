#!/bin/bash
#python3 tools/movelistConverter.py > src/fxdata/data/movelists.txt

mkdir -p fxdata/generated/images
python3 tools/text2bmp.py --font ArduboyFXFonts/Fontbitmaps/Font4x6/Font_5x6.png --input data/text/strings.txt --output_dir fxdata/generated/images --mode joined --greyscale
python3 tools/convert-sprite.py ../images -s 4 -o ../fxdata/
python3 tools/convert-sprite.py ../fxdata/generated/images -s 4 -o ../fxdata/generated/
cat fxdata/generated/images/string_images.txt >>fxdata/generated/Sprites.txt
#cp -r images fxdata/
python3 Arduboy-Python-Utilities/fxdata-build.py fxdata/fxdata.txt
#rm -rf fxdata/images
mv fxdata/fxdata.h src/fxdata.h
mv fxdata/fxdata.bin dist
mv fxdata/fxdata-data.bin dist
#rm -rf fxdata/generated
