#pragma once

#include <Arduino.h>

struct FxTest {
    uint16_t passCount = 0;
    uint16_t failCount = 0;

    void expectEq(uint32_t actual, uint32_t expected,
                  const __FlashStringHelper *label) {
        if (actual == expected) {
            ++passCount;
            return;
        }

        ++failCount;
        Serial.print(F("FAIL "));
        Serial.print(label);
        Serial.print(F(" got="));
        Serial.print(actual);
        Serial.print(F(" want="));
        Serial.println(expected);
    }

    void expectEqIdx(uint32_t actual, uint32_t expected,
                     const __FlashStringHelper *label, uint8_t index) {
        if (actual == expected) {
            ++passCount;
            return;
        }

        ++failCount;
        Serial.print(F("FAIL "));
        Serial.print(label);
        Serial.print(F("["));
        Serial.print(index);
        Serial.print(F("] got="));
        Serial.print(actual);
        Serial.print(F(" want="));
        Serial.println(expected);
    }

    bool ok() const { return failCount == 0; }

    void report(const __FlashStringHelper *suite) {
        Serial.print(suite);
        Serial.print(F(" PASSED="));
        Serial.print(passCount);
        Serial.print(F(" FAILED="));
        Serial.println(failCount);
        Serial.println(ok() ? F("P") : F("F"));
        Serial.flush();
    }
};
