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

    void expectVersion(const char *actual, const char *expected, uint8_t bytes) {
        bool matches = true;
        for (uint8_t index = 0; index < bytes; ++index) {
            if (actual[index] != static_cast<char>(pgm_read_byte(expected + index))) {
                matches = false;
                break;
            }
        }
        if (matches) {
            ++passCount;
            return;
        }

        ++failCount;
        Serial.print(F("FAIL tool_version image=\""));
        for (uint8_t index = 0; index < bytes && actual[index] != '\0'; ++index) {
            Serial.print(actual[index]);
        }
        Serial.print(F("\" fixture=\""));
        for (uint8_t index = 0; index < bytes; ++index) {
            const char value = static_cast<char>(pgm_read_byte(expected + index));
            if (value == '\0') break;
            Serial.print(value);
        }
        Serial.println(F("\""));
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
