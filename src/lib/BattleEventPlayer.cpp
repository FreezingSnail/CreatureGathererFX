#include "../globals.hpp"
#include "BattleEventPlayer.hpp"

void BattleEventPlayer::push(BattleEvent event) {
    this->stackPointer++;
    battleEventStack[this->stackPointer] = event;
}

void BattleEventPlayer::tick() {
    if (this->playing) {
        this->frame++;
        if (this->frame >= this->runlength) {
            this->frame = 0;
            this->stackPointer--;
            if (this->stackPointer < 0) {
                this->playing = false;
            }
        }
    }
}

void BattleEventPlayer::start() {
    this->playing = true;
    this->frame = 0;
    this->runlength = 60;
}

void BattleEventPlayer::play() {
    if (this->playing) {
        BattleEvent event = battleEventStack[this->stackPointer];
        switch (event.type) {
        case BattleEventType::NONE:
            // Handle NONE event
            break;
        case BattleEventType::ATTACK:
            // Handle ATTACK event
            break;
        case BattleEventType::OPPONENT_ATTACK:
            // Handle OPPONENT_ATTACK event
            break;
        case BattleEventType::DAMAGE:
            // Handle DAMAGE event
            break;
        case BattleEventType::OPPONENT_DAMAGE:
            // Handle OPPONENT_DAMAGE event
            break;
        case BattleEventType::HEAL:
            // Handle HEAL event
            break;
        case BattleEventType::OPPONENT_HEAL:
            // Handle OPPONENT_HEAL event
            break;
        case BattleEventType::STATUS:
            // Handle STATUS event
            break;
        case BattleEventType::OPPONENT_STATUS:
            // Handle OPPONENT_STATUS event
            break;
        case BattleEventType::STAT:
            // Handle STAT event
            break;
        case BattleEventType::EVOLVE:
            // Handle EVOLVE event
            break;
        case BattleEventType::LEVELUP:
            // Handle LEVELUP event
            break;
        case BattleEventType::FAINT:
            // Handle FAINT event
            break;
        case BattleEventType::OPPONENT_FAINT:
            // Handle OPPONENT_FAINT event
            break;
        case BattleEventType::WIN:
            // Handle WIN event
            break;
        case BattleEventType::LOSE:
            // Handle LOSE event
            break;
        case BattleEventType::RUN:
            // Handle RUN event
            break;
        case BattleEventType::CATCH:
            // Handle CATCH event
            break;
        case BattleEventType::ESCAPE:
            // Handle ESCAPE event
            break;
        case BattleEventType::ITEM:
            // Handle ITEM event
            break;
        case BattleEventType::SWITCH:
            // Handle SWITCH event
            break;
        case BattleEventType::FLEE:
            // Handle FLEE event
            break;
        case BattleEventType::WAIT:
            // Handle WAIT event
            break;
        }
    }
}