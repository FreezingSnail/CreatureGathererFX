#include "lib/TypeTable.h"
#include <iostream>
#include <stdint.h>

int main() {
    uint8_t result = getMatchupModifier(0, 1);
    std::cout << "matchup returned: " << unsigned(result) << "." << std::endl;
    return 0;
}