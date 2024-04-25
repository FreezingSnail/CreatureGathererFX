git submodule update --init --recursive
arduino-cli config init --additional-urls https://arduboy.github.io/board-support/package_arduboy_index.json
arduino-cli config add board_manager.additional_urls https://raw.githubusercontent.com/MrBlinky/Arduboy-homemade-package/master/package_arduboy_homemade_index.json
arduino-cli core update-index
arduino-cli core install arduino:avr
arduino-cli core install arduboy-homemade:avr
arduino-cli lib install Arduboy2
arduino-cli compile --fqbn "arduboy-homemade:avr:arduboy" . --output-dir .

