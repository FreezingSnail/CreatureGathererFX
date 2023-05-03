#pragma once

#include "../lib/TypeTable.hpp"

typedef struct CreatureSeed{

		uint8_t evoLevel; 			// 000     00000
									// waste   level to evolution

        uint8_t creatureID;			//000     00000
								    //type    Creature id

		uint8_t TypeEvoPtr;			// 000     00000
									// type2   ptr to evolution
		
		uint8_t special;			// 0000     0000
									// socDef   spcAtk stat

		uint16_t statSeed;			// 0000		0000	0000	0000
									// atk		def		hp		spd

        uint32_t movelist;			//32 binary flips

		uint32_t startingMoves;		// need 20bits to store the 4 initial moves

}CreatureSeed_t;

//todo research huffman encoding to squash these in mem
struct CreatureData{
		uint32_t movelist;	//32 binary flips
		unsigned char evoLevel:   	5;
        unsigned char creatureID: 	3;
		unsigned char type1: 		3;
		unsigned char type2: 		3;
		unsigned char EvoPtr:		5;
		unsigned char hpSeed:		4;
		unsigned char atkSeed:		4;
		unsigned char defSeed:		4;	
		unsigned char spcAtkSeed:	4;	
		unsigned char spcDefSeed:	4;	
		unsigned char spdSeed:		4;
		unsigned char move1:		5;
		unsigned char move2:		5;
		unsigned char move3:		5;
		unsigned char move4:		5;
};

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
			CreatureSeed_t seed;
			Type_t type1;
			Type_t type2;
			uint8_t level;

			// Moves are held as an index to the move array 
			uint8_t moves[4];
			stats_t statlist;

			const unsigned char* sprite;

			Creature();
            void load(CreatureSeed_t seed);
			void loadFromOpponentSeed(uint32_t seed);
			
			void setStats();
			void loadMoves();
			void loadSprite();
			void setMove(uint8_t move, uint8_t slot);

			uint8_t getAdvantage(Type_t opponent);  //finds best advantage


			uint8_t getMove(uint8_t slot);
			uint8_t getLevel();
			uint8_t getAtkStatSeed();
			uint8_t getDefStatSeed();
			uint8_t getHpStatSeed();
			uint8_t getSpdStatSeed();
			uint8_t getSpcStatSeed();

			uint8_t getAtkStat();
			uint8_t getDefStat();
			uint8_t getHpStat();
			uint8_t getSpdStat();
			uint8_t getSpcStat();

			uint8_t seedToStat(uint8_t seed);

			uint8_t getID();

			bool moveTypeBonus(uint8_t move);

			#ifdef CLI
				void printMoves();
				void printStats();
			#endif


};