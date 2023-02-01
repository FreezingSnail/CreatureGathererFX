# Ardumon (pending) design doc

This doc details plans / designs / layouts of:
- progmem data
- in-memory representation
- functions
- codepaths
- class responsibilities and dependancys
- 
## Opponents

Actor storage class that holds 3 monster instances 

## Character 
Extension of the actor that 

## World

## Matches

## Monsters 

Monster class is repsonsible for transforms program data into in memory representation

### Key Features

### Monster  Progmem layout
Monster seed used to generate in memory representations 

|    |           |   |
|----|-----------|---|
|type|   monster | id|
|000 |   00000   |   |

|      |       |        |        |
|------|-------|--------|--------|
|0000  |  0000 |   0000 |    0000|
|atk   |  def  |   hp   |   speed|

32bit flip of allowed moves per monster

Monster:

type, Id, stats, move pool
