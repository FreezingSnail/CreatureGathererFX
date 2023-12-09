
#include "Player.hpp"

#include "../creature/Creature.hpp"
#include "../data/teams.hpp"

void Player::Read(uint8_t index) {

    OppTeam opt = Teams::teamList[index];
    party[0].simuLoad(opt.a);
    party[1].simuLoad(opt.b);
    party[2].simuLoad(opt.c);
    teamID = index;
}