# CreatureGather (pending) design doc

This doc details plans / designs / layouts of:
- progmem data
- in-memory representation
- functions
- codepaths
- class responsibilities and dependancys

# Features

## Overworld
- random encounters on tagged tiles
- interaction with opponents
- 
## Combat
- select attack
- change current creature
- defeat for exp
  - random encounters with creatures
    - capture
    - escape
  - opponent matchs (1-3 monster teams)


## Opponents
- 1 - 3 creatures


## Creatures 
### Memory Representaion
- seed
  - stat seeds
  - default moves
  - table of learnable moves
### Game representation
- decompresses seed into useable format


## Gathering
Instead of attacking creatures to weaken and capture them, the main mechanic will be focused around gathering 

- areas to gather in
- area with encounter rate determined by item in gathering lure box in center of area
maybe gather in combat for items to use at the collection box?
unsure exactly how want to have player add creatures into ownership


## events
### data format -> linked list type event data, load one, when pop see if another in chain to load
