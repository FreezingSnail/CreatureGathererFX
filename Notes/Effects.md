- Two effects possible at once
- 3 overarching types
	- Battle modifiers
	- Ticker effect
	- Stat modifiers

Each status can have up to 2 effects

### Stat modifiers
when afflicted, the stat modifier is adjusted
when status is cleared, the stat change is removed

### Ticker effects
Every turn a tick effect happens, hp loss, stat change

### Battle Modifiers
Effects creature move
	- Accuracy override 
	- move override?
	- type bonus overide

### Type of modifiers
	- type modifer reducer
	- type modifier enhancer
	- stat increaser
	- stat reducer
	- tick damage
	- tick heal
	



## Impl

Each effect compiled to a bytecode dictating what effects are applied
code processed in each turn step to see if applicable

2 bytes per effect
	-	FF reserved for no effect





