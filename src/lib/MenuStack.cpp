#include "MenuStack.hpp"

void MenuStack::push(MenuEnum menu) {
    this->pointer++;
    this->stack[this->pointer] = menu;
}

void MenuStack::pop() {
    this->pointer--;
}

MenuEnum MenuStack::top() {
    return this->stack[this->pointer];
}