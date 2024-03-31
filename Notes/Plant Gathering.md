

## Plants
- Collectible item that grows on world map

- multiple stages
	- grow based on steps
	- seed -> sprout -> youth? -> pickable


## Tiles

each map may / may not have planting tiles


# Design
- global ticker that increments stage
- global state tracker? -> 64 bits
## Memory 
### plant state
- 3 bits for type
- 1 bit for active
- 4 left over -> can use nibbles
-> can compact to 16 bytes + 8 bytes for stage

### stage 
reverse ordering ->lowest 2 bits index 0, highest 31.

### map state
- 32 total berry locations
	- MapID
	- tile id (0-31)
	- map x,y
	- plant information can be accessed by using tile ID and lookup
	  
- 4 max per map -> 4 in ram -> 2 byte ram imprint


### loading 
upon entering area:
- 0 out active plants
- read fx data using map ID as key and load the 4 max plants into ram
- set count of active berries

### saving
on area exit:
- write the state to eeprom using count and initial berry index as offset

### EEPROM
0-31 index