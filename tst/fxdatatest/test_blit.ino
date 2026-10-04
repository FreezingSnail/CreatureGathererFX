#include "src/common.hpp"
#include "blit_test.hpp"
decltype(arduboy) arduboy;
void setup() {
    Serial.begin(9600);
    arduboy.begin();
    FX::begin(FX_DATA_PAGE, FX_SAVE_PAGE);
    FxTest test;
    blit_test::run(test);
    test.report(F("test_blit"));
}
void loop() { exit(0); }
