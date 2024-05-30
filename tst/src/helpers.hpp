#pragma once
#include <string>
#include "../../src/globals.hpp"

std::string BattleEventTypeToString(BattleEventType e) {
    switch (e) {
    case BattleEventType::NONE:
        return "NONE";
    case BattleEventType::ATTACK:
        return "ATTACK";
    case BattleEventType::OPPONENT_ATTACK:
        return "OPPONENT_ATTACK";
    case BattleEventType::DAMAGE:
        return "DAMAGE";
    case BattleEventType::OPPONENT_DAMAGE:
        return "OPPONENT_DAMAGE";
    case BattleEventType::HEAL:
        return "HEAL";
    case BattleEventType::OPPONENT_HEAL:
        return "OPPONENT_HEAL";
    case BattleEventType::STATUS:
        return "STATUS";
    case BattleEventType::OPPONENT_STATUS:
        return "OPPONENT_STATUS";
    case BattleEventType::STAT:
        return "STAT";
    case BattleEventType::EVOLVE:
        return "EVOLVE";
    case BattleEventType::LEVELUP:
        return "LEVELUP";
    case BattleEventType::FAINT:
        return "FAINT";
    case BattleEventType::OPPONENT_FAINT:
        return "OPPONENT_FAINT";
    case BattleEventType::WIN:
        return "WIN";
    case BattleEventType::LOSE:
        return "LOSE";
    case BattleEventType::RUN:
        return "RUN";
    case BattleEventType::CATCH:
        return "CATCH";
    case BattleEventType::ESCAPE:
        return "ESCAPE";
    case BattleEventType::ITEM:
        return "ITEM";
    case BattleEventType::SWITCH:
        return "SWITCH";
    case BattleEventType::FLEE:
        return "FLEE";
    case BattleEventType::WAIT:
        return "WAIT";
    default:
        return "UNKNOWN";
    }
}