## Things needed to persist
- Creature states
- player location
- plant states
- Inventory
- gatherer fight completion (just bit array?)

## Design
The main thing I’ve been thinking about is a 2 tier saving system using the FX memory and EEPROM. The FX savedata would contain everything, and on game start the EEPROM would be written too from the FX save data. During play EEPROM would be written to and read from as needed. On game save, everything would be read then written to the FX memory. My main concern with this approach is the complexity.