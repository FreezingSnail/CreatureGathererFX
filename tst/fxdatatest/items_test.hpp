#pragma once

#include <avr/pgmspace.h>
#include <string.h>

#include "fxtest.hpp"
#include "src/fxdata.h"
#include "src/item/ItemNames.hpp"
#include "src/lib/FxRead.hpp"

namespace {

static_assert(sizeof(uint24_t) == 3, "FX addresses must stay three bytes");

inline void expect_uint24_bytes(FxTest &test, const uint24_t &actual,
                                const uint24_t &expected,
                                const __FlashStringHelper *label, uint8_t index) {
    uint8_t actualBytes[3];
    uint8_t expectedBytes[3];
    memcpy(actualBytes, &actual, sizeof(actualBytes));
    memcpy(expectedBytes, &expected, sizeof(expectedBytes));
    for (uint8_t byte = 0; byte < sizeof(actualBytes); ++byte) {
        test.expectEqIdx(actualBytes[byte], expectedBytes[byte], label, index);
    }
}

inline void expect_name_table(FxTest &test, uint24_t table,
                              const uint24_t *publishedAddresses,
                              uint8_t count,
                              const __FlashStringHelper *label) {
    for (uint8_t index = 0; index < count; ++index) {
        const uint24_t actual = FxRead::indexed24(table, index);
        uint8_t actualBytes[3];
        uint8_t publishedBytes[3];
        memcpy(actualBytes, &actual, sizeof(actualBytes));
        memcpy_P(publishedBytes, publishedAddresses + index, sizeof(publishedBytes));
        for (uint8_t byte = 0; byte < sizeof(actualBytes); ++byte) {
            test.expectEqIdx(actualBytes[byte], publishedBytes[byte], label, index);
        }
    }
}

inline void test_consumable_records(FxTest &test) {
    // Literal values from data/consumables.csv; ItemKind values are None=0,
    // Heal=1, Cure=2, and Charge=3.
    static const uint8_t expectedKinds[] PROGMEM = {1, 1, 1, 2, 2, 2, 3, 3};
    static const uint8_t expectedArgs[] PROGMEM = {16, 32, 64, 1, 2, 3, 16, 32};

    uint24_t recordAddress = consumable_table;
    for (uint8_t index = 0; index < 8; ++index) {
        const uint24_t expectedAddress = consumable_table +
            static_cast<uint16_t>(index) * 2;
        expect_uint24_bytes(test, recordAddress, expectedAddress,
                            F("consumable_table record address"), index);

        uint8_t record[2];
        FX::readDataBytes(recordAddress, record, sizeof(record));
        test.expectEqIdx(record[0], pgm_read_byte(expectedKinds + index),
                         F("consumable kind"), index);
        test.expectEqIdx(record[1], pgm_read_byte(expectedArgs + index),
                         F("consumable arg"), index);
        recordAddress += 2;
    }
}

inline void test_item_name_tables(FxTest &test) {
    // These entries follow data/text/strings.txt declaration and table order.
    static const uint24_t publishedLureTiers[] PROGMEM = {
        lureTier0, lureTier1, lureTier2,
    };
    static const uint24_t publishedLureTypes[] PROGMEM = {
        lureType0, lureType1, lureType2, lureType3,
        lureType4, lureType5, lureType6, lureType7,
    };
    static const uint24_t publishedConsumables[] PROGMEM = {
        consumable0, consumable1, consumable2, consumable3,
        consumable4, consumable5, consumable6, consumable7,
    };

    expect_name_table(test, LureTierNames::LureTierNames, publishedLureTiers, 3,
                      F("LureTierNames published address"));
    expect_name_table(test, LureTypeNames::LureTypeNames, publishedLureTypes, 8,
                      F("LureTypeNames published address"));
    expect_name_table(test, ConsumableNames::ConsumableNames, publishedConsumables, 8,
                      F("ConsumableNames published address"));
}

inline void expect_lure_name(FxTest &test, uint8_t id,
                             const uint24_t &expectedTier,
                             const uint24_t &expectedType) {
    const item::ItemName actual = item::itemNameAddr(item::ItemKind::Lure, id);
    test.expectEq(actual.parts, 2, F("lure name part count"));
    expect_uint24_bytes(test, actual.part[0], expectedTier, F("lure tier name address"), id);
    expect_uint24_bytes(test, actual.part[1], expectedType, F("lure type name address"), id);
}

} // namespace

inline void test_items(FxTest &test) {
    test_consumable_records(test);
    test_item_name_tables(test);

    // Lure IDs are type * 3 + tier: 0 is tier/type 0/0, and 23 is 2/7.
    const uint24_t tier0 = FxRead::indexed24(LureTierNames::LureTierNames, 0);
    const uint24_t type0 = FxRead::indexed24(LureTypeNames::LureTypeNames, 0);
    expect_lure_name(test, 0, tier0, type0);

    const uint24_t tier2 = FxRead::indexed24(LureTierNames::LureTierNames, 2);
    const uint24_t type7 = FxRead::indexed24(LureTypeNames::LureTypeNames, 7);
    expect_lure_name(test, 23, tier2, type7);
}
