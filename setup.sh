mkdir lib
cd lib
git clone --depth=1 --branch v1.13.0 https://github.com/google/googletest/


arduino-cli config init --additional-urls https://arduboy.github.io/board-support/package_arduboy_index.json
#arduino-cli compile --fqbn "arduboy:avr:arduboy"  .  --output-dir .