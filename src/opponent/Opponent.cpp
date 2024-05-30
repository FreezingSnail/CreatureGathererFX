
#include "Opponent.hpp"
#include "../lib/DataTypes.hpp"
#include "../lib/ReadData.hpp"

Opponent::Opponent() {
}

void Opponent::loadOpt(OpponentSeed_t *seed) {
    this->levels[0] = parseOpponentCreatureSeedlvl(seed->firstCreature);
    this->levels[1] = parseOpponentCreatureSeedlvl(seed->secondCreature);
    this->levels[2] = parseOpponentCreatureSeedlvl(seed->thirdCreature);
    this->party[0].loadFromOpponentSeed(seed->firstCreature);
    this->party[1].loadFromOpponentSeed(seed->secondCreature);
    this->party[2].loadFromOpponentSeed(seed->thirdCreature);
}
