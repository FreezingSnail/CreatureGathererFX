
#include "Opponent.hpp"
#include "OpponentSeed.hpp"

Opponent::Opponent(){

}


void Opponent::load(OpponentSeed* seed) {
    this->levels[0] = parseOpponentCreatureSeedlvl(seed->firstCreature);
    this->levels[1] = parseOpponentCreatureSeedlvl(seed->secondCreature);
    this->levels[2] = parseOpponentCreatureSeedlvl(seed->thirdCreature);

    
}
