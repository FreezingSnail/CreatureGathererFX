#include "Animator.hpp"
#include "common.hpp"

Animator::Animator() {
    stackPointer = -1;
}

void Animator::push(Animation animation) {
    if (stackPointer >= 1)
        return;

    stackPointer++;
    animationStack[stackPointer] = animation;
    start();
}

void Animator::pop() {
    if (stackPointer < 0)
        return;
    stackPointer--;
}

void Animator::play() {
    ticker++;

    if (!playing)
        return;
    SpritesU::drawPlusMaskFX(animationStack[stackPointer].xOrigin, animationStack[stackPointer].yOrigin, animationStack[stackPointer].data,
                             FRAME(currentFrame));
    if (ticker % 20 != 0) {
        return;
    }
    currentFrame++;
    if (currentFrame >= animationStack[stackPointer].frames) {
        pop();
        playing = false;
        if (stackPointer >= 0)
            start();
    }
}

void Animator::start() {
    playing = true;
    currentFrame = 0;
    ticker = 0;
}