#pragma once
#include "src/lib/DataTypes.hpp"
#include "src/opponent/Opponent.hpp"
#include "src/fxdata.h"
#include "opponent_data.hpp"
#include "src/lib/ReadData.hpp"

void test_opponents() {
    Opponent optFX;
    OpponentSeed seed_base = opponentSeeds[0];
    Opponent control;
    control.loadOpt(&seed_base);

    ReadOpt(&optFX, 0);

    if (control.party[0].id != optFX.party[0].id) {
        Serial.print(F("Opponent 0 id failed; "));
        Serial.print(control.party[0].id);
        Serial.print(F(" "));
        Serial.println(optFX.party[0].id);
    }
    if (control.party[1].id != optFX.party[1].id) {
        Serial.print(F("Opponent 1 id failed"));
        Serial.println();
    }
    if (control.party[2].id != optFX.party[2].id) {
        Serial.print(F("Opponent 2 id failed: "));
        Serial.print(control.party[2].id);
        Serial.print(F(" "));
        Serial.println(optFX.party[2].id);
    }
    if (control.levels[0] != optFX.levels[0]) {
        Serial.print(F("Opponent 0 level failed: "));
        Serial.print(control.levels[0]);
        Serial.print(F(" "));
        Serial.println(optFX.levels[0]);
    }
    if (control.levels[1] != optFX.levels[1]) {
        Serial.print(F("Opponent 1 level failed: "));
        Serial.print(control.levels[1]);
        Serial.print(F(" "));
        Serial.println(optFX.levels[1]);
    }
    if (control.levels[2] != optFX.levels[2]) {
        Serial.print(F("Opponent 2 level failed: "));
        Serial.print(control.levels[2]);
        Serial.print(F(" "));
        Serial.println(optFX.levels[2]);
    }

    if (control.party[0].level != optFX.party[0].level) {
        Serial.print(F("Opponent 0 level failed: "));
        Serial.print(control.party[0].level);
        Serial.print(F(" "));
        Serial.println(optFX.party[0].level);
    }
    if (control.party[1].level != optFX.party[1].level) {
        Serial.print(F("Opponent 1 level failed: "));
        Serial.print(control.party[1].level);
        Serial.print(F(" "));
        Serial.println(optFX.party[1].level);
    }
    if (control.party[2].level != optFX.party[2].level) {
        Serial.print(F("Opponent 2 level failed: "));
        Serial.print(control.party[2].level);
        Serial.print(F(" "));
        Serial.println(optFX.party[2].level);
    }
}