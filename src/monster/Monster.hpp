#pragma once

#include "lib/TypeTable.hpp"

typedef struct monsterSeed{

        uint8_t monsterid;			//000     00000
								    //type    monster id

		uint16_t statSeed;			// 0000		0000	0000	0000
									// atk		def		hp		spd

        uint32_t movelist;			//32 binary flips
}monsterSeed_t;

typedef struct stats{
	uint16_t attack;
	uint16_t defense;
	uint16_t health;
	uint16_t speed;
}stats_t;

class Monster{

	public:
			monsterSeed_t seed;
			uint8_t level;
			uint8_t type;
			uint8_t moves[4];
			stats_t statlist;

            void Load(monsterSeed_t seed);
			Monster(uint32_t seed);
			
			uint8_t getType(); //seed.monsterid >>5 
			void changeMove(uint8_t slot, uint8_t newMove);
			void setStats();

			uint8_t getAdvantage(Type_t opponent);  //finds best advantage


			uint8_t getMove(uint8_t slot);
			uint8_t getLevel();
			uint8_t getAtkStatSeed();
			uint8_t getDefStatSeed();
			uint8_t getHpStatSeed();
			uint8_t getSpdStatSeed();

			void printStats();


};


