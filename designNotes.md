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
