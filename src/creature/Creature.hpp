#pragma once

#include "lib/TypeTable.hpp"

typedef struct CreatureSeed{

        uint8_t Creatureid;			//000     00000
								    //type    Creature id

		uint16_t statSeed;			// 0000		0000	0000	0000
									// atk		def		hp		spd

        uint32_t movelist;			//32 binary flips
}CreatureSeed_t;

typedef struct stats{
	uint16_t attack;
	uint16_t defense;
	uint16_t health;
	uint16_t speed;
}stats_t;

class Creature{

	public:
			CreatureSeed_t seed;
			Type_t type;
			uint8_t level;
			uint8_t type;
			uint8_t moves[4];
			stats_t statlist;

            void Load(CreatureSeed_t seed);
			Creature();
			Creature(uint32_t seed);
			
			void changeMove(uint8_t slot, uint8_t newMove);
			void setStats();

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

			void printStats();


};


