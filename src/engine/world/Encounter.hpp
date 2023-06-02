#pragma once

#include <stdint.h>
#include "../../data/Encounters.hpp"



class Encounter {
	private:
		EncounterTable currentEncounterTable;
	public:
		void loadEncounterTable(int areaIndex);
		// returns creatureID based on available encounters and rates of current EncounterTable
		uint8_t rollEncounter();
};
