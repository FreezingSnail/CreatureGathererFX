#pragma once
#include <stdint.h>
using uint24_t = __uint24;

struct Animation {
    uint8_t xOrigin, yOrigin;
    uint8_t frames;
    uint24_t data;
};

enum class AnimationType {
    ATTACK,
    DAMAGE,
    HEAL,
    STATUS,
    STAT,
    EVOLVE,
    LEVELUP,
    FAINT,
    WIN,
    LOSE,
    RUN,
    CATCH,
    ESCAPE,
    ITEM,
    SWITCH,
    FLEE,
    WAIT,
    NONE
};

struct AnimationEvent {
    AnimationType type;
    // used for name index, committer, move, etc
    uint8_t index;
    // used for numerical data like damage
    uint8_t data;
};

class Animator {
  public:
    Animation animationStack[3];
    bool playing;
    uint8_t currentFrame;
    int8_t stackPointer;
    uint8_t ticker = 0;

    Animator();
    void push(Animation animation);
    void pop();
    void start();
    void play();
};