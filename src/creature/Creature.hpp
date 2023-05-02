#pragma once

#include "../lib/TypeTable.hpp"

typedef struct CreatureSeed{

        uint8_t creatureID;			//000     00000
								    //type    Creature id

		uint16_t statSeed;			// 0000		0000	0000	0000
									// atk		def		hp		spd

        uint32_t movelist;			//32 binary flips

		uint32_t startingMoves;		// need 20bits to store the 4 initial moves
}CreatureSeed_t;

typedef struct stats{
	uint16_t attack;
	uint16_t defense;
	uint16_t health;
	uint16_t speed;
}stats_t;

class Creature{

	public:
			// potential saving: read this from progmem when need it ? 
			// 6 instances to 1 stack var
			CreatureSeed_t seed;
			Type_t type;
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

			uint8_t getAtkStat();
			uint8_t getDefStat();
			uint8_t getHpStat();
			uint8_t getSpdStat();

			uint8_t getStatAtLevel(uint8_t stat);

			uint8_t getID();

			bool moveTypeBonus(uint8_t move);

			#ifdef CLI
				void printMoves();
				void printStats();
			#endif


};