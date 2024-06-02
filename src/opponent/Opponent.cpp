
#include "Opponent.hpp"
#include "../lib/DataTypes.hpp"
#include "../lib/ReadData.hpp"

Opponent::Opponent() {
}

void Opponent::loadOpt(OpponentSeed *seed) {
    this->levels[0] = (seed->firstCreature.lvl);
    this->levels[1] = (seed->secondCreature.lvl);
    this->levels[2] = (seed->thirdCreature.lvl);
    this->party[0].loadFromOpponentSeed(seed->firstCreature);
    this->party[1].loadFromOpponentSeed(seed->secondCreature);
    this->party[2].loadFromOpponentSeed(seed->thirdCreature);
}
