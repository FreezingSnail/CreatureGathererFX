#pragma once

#include <avr/pgmspace.h>

#include "fxtest.hpp"
#include "generated/version_data.hpp"
#include "src/fxdata.h"

inline void test_version(FxTest &test) {
    char imageVersion[generatedToolVersionBytes];
    for (uint8_t index = 0; index < generatedToolVersionBytes; ++index) {
        imageVersion[index] = static_cast<char>(
            FX::readIndexedUInt8(generator_version, index));
    }
    test.expectVersion(imageVersion, generatedToolVersion, generatedToolVersionBytes);
}
