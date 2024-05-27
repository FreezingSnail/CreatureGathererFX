for each turn in a battle, there are a number of animation events with text to display stating what happened.  The idea is to allow the engine to complete its calculations in a single cycle, while populating a queue.  This queue is then read from on the next cycle.  Until the queue is empty, input is locked.

From [[Battle Engine]]
![[Battle Engine#stages]]


Each stage has corresponding text and  animations 

## Animation instructions

each instruction needs the following information: 
- Animation type 
	- Attacking
	- Receiving
	- Fainting
	- Switching
	- etc
- Numerical information
	- damage
	- move selected
	- effect applied
	- applier / receiver 


The type can be used to determine various fine details, i.e. player or cpu actions, where to draw, which form of animation (l or r)

## Data
info kept on the FX cart:
- animations
- animation length
- position change vectors?