#pragma once

#include "../lib/Type.hpp"

//todo research huffman encoding to squash these in mem
// struct CreatureData{
// 	unsigned char type1: 		3;
// 	unsigned char type2: 		3;
// 	unsigned char hpSeed:		4;
// 	unsigned char atkSeed:		4;
// 	unsigned char defSeed:		4;	
// 	unsigned char spcAtkSeed:	4;	
// 	unsigned char spcDefSeed:	4;	
// 	unsigned char spdSeed:		4;
// 	unsigned char evoLevel:   	5;
// 	unsigned char creatureID: 	5;
// 	unsigned char move1:		5;
// 	unsigned char move2:		5;
// 	unsigned char move3:		5;
// 	unsigned char move4:		5;
// };

typedef struct stats{
	uint8_t attack;
	uint8_t defense;
	uint8_t health;
	uint8_t speed;
	uint8_t spcAtk;
	uint8_t spcDef;
}stats_t;

class Creature{

	public:
			// potential saving: read this from progmem when need it ? 
			// 6 instances to 1 stack var
			uint64_t seed;
			//todo(snail)
			// need to squash these down into 1 byte -> 10 byte loss to 2byte loss
			//if I squash these i can use the 5thbit as a flag to show the second type is none
			// or just have the two types be the same
			//if therye the same there's going to be issues with the modifer double counting tho
			DualType types;
			uint8_t level;

			// Moves are held as an index to the move array 
			uint8_t moves[4];
			stats_t statlist;

			const unsigned char* sprite;

			Creature();
            void load(uint64_t seed);
			void loadFromOpponentSeed(uint32_t seed);
			
			void setStats();
			void loadMoves();
			void loadSprite();
			void loadTypes();
			void setMove(uint8_t move, uint8_t slot);

			uint8_t getAdvantage(DualType opponent);  //finds best advantage


			uint8_t getMove(uint8_t slot);
			uint8_t getLevel();
			uint8_t getAtkStatSeed();
			uint8_t getDefStatSeed();
			uint8_t getHpStatSeed();
			uint8_t getSpdStatSeed();
			uint8_t getSpcAtkStatSeed();
			uint8_t getSpcDefStatSeed();
			uint8_t getEvolutionLevel();
			uint8_t getID();


			//todo(snail)
			//do I actually need these?
			// the idea was to just use the seed to calc the stats on the fly as needed
			//maybe I should go back to that aproach and save the 6 bytes per creature from ram

			uint8_t getAtkStat();
			uint8_t getDefStat();
			uint8_t getHpStat();
			uint8_t getSpdStat();
			uint8_t getSpcAtkStat();
			uint8_t getSpcDefStat();

			uint8_t seedToStat(uint8_t seed);
			bool moveTypeBonus(uint8_t move);

			#ifdef CLI
				void printMoves();
				void printStats();
			#endif


};