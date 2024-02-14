#include "Animator.hpp"

void Animator::push(Animation animation) {
    if (stackPointer >= 1)
        return;
    stackPointer++;
    animationStack[stackPointer] = animation;
}

void Animator::pop() {
    if (stackPointer < 0)
        return;
    stackPointer--;
}

void Animator::play() {
    if (!playing)
        return;
    SpritesU::drawOverwriteFX(animationStack[stackPointer].xOrigin, animationStack[stackPointer].yOrigin, animationStack[stackPointer].data,
                              FRAME(currentFrame));
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
}