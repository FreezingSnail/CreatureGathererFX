#pragma once
#include "../../action/Action.hpp"
#include "../../creature/Creature.hpp"
#include "../../opponent/Opponent.hpp"
#include "../../player/Player.hpp"
#include "../game/Gamestate.hpp"
#include "../../lib/StatModifier.hpp"

class MenuV2;

class BattleEngine {
  public:
    uint8_t debug;
    Creature *playerParty[3];
    Creature *playerCur;
    Creature *opponentCur;

    Opponent opponent;
    uint16_t playerHealths[3];
    uint16_t opponentHealths[3];
    uint8_t awakeMons;   // 11100111 player and opponet bit array

    StatModifer playerModifiers;
    StatModifer opponentModifiers;

    uint8_t playerIndex;
    uint8_t opponentIndex;
    Action playerAction;
    Action opponentAction;

    bool activeBattle;

    bool queued;
    Action queuedAction;

    BattleEngine();
    void init();

    // entry points
    void startFight(uint8_t optID);
    void startArena(uint8_t optID);
    void startEncounter(uint8_t creatureID, uint8_t level);

    // flow control
    void encounter();

    // menu interface
    void queueAction(ActionType type, uint8_t index);

    uint8_t *getPlayerCurCreatureMoves();
    void updateInactiveCreatures(uint8_t *list);
    Creature *getCreature(uint8_t index);

    // flow control
    void turnTick();
    bool checkLoss();
    bool checkWin();
    bool checkPlayerFaint();
    bool checkOpponentFaint();
    void playerActionFirst();
    void opponentActionFirst();
    void setMoveList(uint8_t **pointer);
    void changeCurMon(uint8_t index);
    void changeOptMon(uint8_t index);
    bool tryCapture();
    void endEncounter();

    // inputs
    bool getInput();
    void opponentInput();

    // event execution
    void commitAction(Action *action, Creature *commiter, Creature *receiver, bool isPlayer);
    void applyDamage(uint16_t damage, Creature *receiver);

    // data loading
    void loadPlayer();
    void loadOpponent(uint8_t optID);
    void LoadCreature(uint8_t creatureID, uint8_t level);
    void resetOpponent();

    // drawing
    void drawScene();
    void drawPlayer();
    void drawOpponent();
    void drawPlayerHP();
    void drawOpponentHP();
    void applyEffects();
    void applyEffect(Creature *target, Effect effect);
    void applyBattleEffect(Creature *target, Effect effect);
    void runEffect(Creature *commiter, Creature *other, Effect effect);
};