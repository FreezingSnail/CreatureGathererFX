// Exercise the default native48 renderer and production menu.
#ifndef CGFX_BATTLE_48_SPIKE
#define CGFX_BATTLE_48_SPIKE
#endif
#include "harness/fx_globals.hpp"
#include "src/engine/draw.h"
#include "generated/creature_data.hpp"
#include "fxtest.hpp"

bool matchesFrame(uint8_t x, uint16_t frame) {
    bool matches = true;
    for (uint8_t page = 0; page < 6; ++page) {
        for (uint8_t col = 0; col < 48; ++col) {
            uint8_t bytes[2];
            FX::readDataBytes(battleSprites48 + 4 +
                static_cast<uint24_t>(frame) * battle_layout::spriteStride48 +
                static_cast<uint16_t>(page * 48u + col) * 2u, bytes, 2);
            if (Arduboy2Base::sBuffer[page * 128u + x + col] != (bytes[0] & bytes[1])) matches = false;
        }
    }
    return matches;
}

bool blankSprite(uint8_t x) {
    for (uint8_t page = 0; page < 6; ++page)
        for (uint8_t col = 0; col < 48; ++col)
            if (Arduboy2Base::sBuffer[page * 128u + x + col]) return false;
    return true;
}

bool hudPixel(uint8_t x, uint8_t y) {
    return Arduboy2Base::sBuffer[(y >> 3) * 128 + x] & (1u << (y & 7));
}

uint16_t referenceHudGlyph(char c) {
    switch (c) {
    case 'F': return 4815; case 'O': return 31599; case 'E': return 29391;
    case 'Y': return 9389; case 'U': return 31597;
    case '/': return 0b001001010100100;
    default: {
        static const uint16_t digits[] PROGMEM = {31599, 29850, 29671, 31207, 18925, 31183, 31695, 9383, 31727, 31215};
        return pgm_read_word(digits + c - '0');
    }
    }
}

bool compactTextMatches(uint8_t y, const char *text) {
    uint8_t x = 50;
    for (char c; (c = pgm_read_byte(text++)) != 0; x += 4) {
        uint16_t bits = referenceHudGlyph(c);
        for (uint8_t row = 0; row < 5; ++row)
            for (uint8_t col = 0; col < 4; ++col) {
                const bool expected = col < 3 && (bits & (1u << (row * 3 + col)));
                if (hudPixel(x + col, y + row) != expected) return false;
            }
    }
    for (; x < 80; ++x)
        for (uint8_t row = 0; row < 5; ++row)
            if (hudPixel(x, y + row)) return false;
    return true;
}

bool opponentNumbersHidden() {
    for (uint8_t x = 49; x < 80; ++x)
        for (uint8_t y = 13; y < 20; ++y)
            if (hudPixel(x, y)) return false;
    return true;
}

void optionsFrame(const battle::BattleView &view, uint8_t cursor) {
    arduboy.clear();
    menu.clear(); menu.push(BATTLE_OPTIONS); menu.cursorIndex = cursor;
    Blit::draw(0, 48, 128, 16, battleOptions48 + 4, FRAME(cursor), Blit::OVERWRITE);
    drawScene(view);
}

#ifdef CGFX_BATTLE48_CAPTURE
void dumpFrame(const __FlashStringHelper *name) {
    Serial.print(F("SCREEN:")); Serial.print(name); Serial.print(':');
    for (uint16_t i = 0; i < 1024; ++i) {
        const uint8_t byte = Arduboy2Base::sBuffer[i];
        if (byte < 16) Serial.print('0');
        Serial.print(byte, HEX);
    }
    Serial.println();
}
#else
void dumpFrame(const __FlashStringHelper *) {}
#endif

void setup() {
    fxTestSetup();
    FxTest test;
    CreatureData_t first, last;
    memcpy_P(&first, creatureFixtures, sizeof(first));
    memcpy_P(&last, creatureFixtures + creatureFixtureCount - 1, sizeof(last));
    battle::BattleView view{};
    view.active[1] = {first.id, 35, 100};
    view.active[0] = {last.id, 80, 100};
    view.moveIds[0] = last.move1; view.moveIds[1] = last.move2;
    view.moveIds[2] = last.move3; view.moveIds[3] = last.move4;
    for (uint8_t slot = 0; slot < 4; ++slot) {
        view.remainingUses[slot] = slot == 1 ? 255 : 2;
        view.useLimitsPacked |= 2u << (slot * 2);
    }
    test.expectEq(battleSprites48Width, static_cast<uint16_t>(48), F("packed width is48"));
    test.expectEq(battleSprites48Frames, static_cast<uint8_t>(creatureFixtureCount * 2), F("all species have paired frames"));
    optionsFrame(view, 0);
    test.expectEq(matchesFrame(0, first.id * 2), true, F("first species front48 exact packed pixels"));
    test.expectEq(matchesFrame(80, last.id * 2 + 1), true, F("last species back48 exact packed pixels"));
    test.expectEq(battleHpBarWidth(255, 100), static_cast<uint8_t>(28), F("HP above maximum clamps"));
    test.expectEq(battleHpBarWidth(255, 0), static_cast<uint8_t>(0), F("zero maximum safe"));
    test.expectEq((Arduboy2Base::sBuffer[3 * 128 + 60] & 0b00001000) != 0,
                  true, F("player HP slash draws on its actual page"));
    test.expectEq(compactTextMatches(2, PSTR("FOE")), true, F("upper health label is FOE"));
    test.expectEq(compactTextMatches(20, PSTR("YOU")), true, F("lower health label is YOU"));
    test.expectEq(opponentNumbersHidden(), true, F("opponent current/max HP never shown"));
    test.expectEq(compactTextMatches(27, PSTR("80/100")), true, F("player current/max HP matches player values"));
    optionsFrame(view, 0);
    dumpFrame(F("options"));
    view.active[0] = {last.id, 0, 100}; optionsFrame(view, 0);
    test.expectEq(compactTextMatches(27, PSTR("0/100")), true, F("zero HP has one zero digit"));
    view.active[0] = {last.id, 5, 10}; optionsFrame(view, 0);
    test.expectEq(compactTextMatches(27, PSTR("5/10")), true, F("short HP omits leading zeroes"));
    view.active[0] = {last.id, 255, 255}; optionsFrame(view, 0);
    test.expectEq(compactTextMatches(27, PSTR("255/255")), true, F("three-digit HP fits corridor"));
    Blit::fillRect(0, 40, 128, 24, BLACK);
    test.expectEq(hudPixel(49, 35) && hudPixel(78, 37) && hudPixel(77, 36), true,
                  F("player thin bar remains above feedback"));
    view.active[0] = {last.id, 80, 100};
    view.active[0] = {255, 0, 0}; view.active[1] = {255, 0, 0};
    Blit::fillRect(0, 0, 128, 48, WHITE); drawScene(view);
    test.expectEq(blankSprite(0) && blankSprite(80), true, F("hidden sentinel clears both48 slots"));
    view.active[1] = {first.id, 35, 100}; view.active[0] = {last.id, 80, 100};
    menu.clear(); menu.openMenu(BATTLE_MOVE_SELECT, view);
    FxReadCounter::resetFrame();
    arduboy.clear(); Blit::fillRect(0, 48, 128, 16, WHITE);
    printMoveMenu(0, menu.movesSnapshot(), menu.moveNameAddresses, menu.moveInfo());
    drawScene(view);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0), F("render uses cached move metadata"));
    test.expectEq(matchesFrame(0, first.id * 2) && matchesFrame(80, last.id * 2 + 1), true, F("move info preserves both full sprites"));
    test.expectEq((Arduboy2Base::sBuffer[3 * 128 + 50] & 0b00100000) != 0,
                  true, F("PP label draws below metadata on its actual page"));
    dumpFrame(F("moves"));
    view.moveIds[2] = LEGACY_EMPTY_MOVE_ID; view.moveIds[3] = EMPTY_MOVE_ID;
    menu.clear(); menu.openMenu(BATTLE_MOVE_SELECT, view);
    arduboy.clear(); Blit::fillRect(0, 48, 128, 16, WHITE);
    printMoveMenu(1, menu.movesSnapshot(), menu.moveNameAddresses, menu.moveInfo()); drawScene(view);
    test.expectEq(Arduboy2Base::sBuffer[7 * 128 + 4], static_cast<uint8_t>(255), F("empty lower move slots stay blank"));
    dumpFrame(F("empty-moves"));
    // Exercise the separately compiled production menu and switch presenter.
    battle::ActionResult result{};
    battle::resetActionResult(result); result.kind = battle::ResultKind::Switch;
    result.actor = battle::Side::Player; result.index = last.id;
    result.speciesBefore[0] = first.id; result.speciesBefore[1] = first.id;
    result.maxHpBefore[0] = result.maxHpBefore[1] = 100;
    result.hpBefore[0] = result.hpBefore[1] = result.hpAfter[0] = result.hpAfter[1] = 80;
    battle::BattlePresenter presenter; presenter.begin(result);
    for (uint8_t tick = 0; tick < battle::ANNOUNCE_TICKS; ++tick) presenter.update(false);
    auto incoming = view; incoming.active[0] = {last.id, 80, 100}; presenter.overlay(incoming);
    test.expectEq(incoming.active[0].id, last.id, F("last species survives switch overlay"));
    menu.clear(); menu.push(BATTLE_OPTIONS); menu.cursorIndex = 3;
    arduboy.clear(); menu.printMenu(view); drawScene(view);
    test.expectEq(matchesFrame(80, last.id * 2 + 1), true, F("production options preserve48 scene"));
    dumpFrame(F("escape-selected"));
    menu.clear();
    const uint32_t start = micros();
    for (uint8_t frame = 0; frame < 8; ++frame) optionsFrame(view, frame & 3u);
    const uint32_t average = (micros() - start) / 8;
    Serial.print(F("battle48_render_avg_us=")); Serial.println(average);
    test.expectEq(average < 19230, true, F("48px scene and options render within52fps period"));
    test.report(F("test_battle48"));
}
void loop() { exit(0); }
