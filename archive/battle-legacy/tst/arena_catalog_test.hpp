#pragma once

#include "test.hpp"
#include "src/FXDataFake.hpp"
#include "src/engine/arena/ArenaCatalog.hpp"
#include "src/fxdata.h"
#include "src/lib/FxReadCounter.hpp"

void ArenaCatalogSuite(TestRunner &runner)
{
    Test test("Arena catalog readers");
    fxDataFake::dataBase = ArenaDemoData::playerMembers;
    for (uint8_t i = 0; i < sizeof(fxDataFake::dataBytes); ++i)
        fxDataFake::dataBytes[i] = i;
    fxDataFake::readCount = 0;
    FxReadCounter::resetFrame();

    arena::Member member{};
    test.assert(arena::readPlayerMember(2, 2, member), true,
                "last player member is readable");
    test.assert(member.species, static_cast<uint8_t>((2 * 18 + 2 * 6) & 0xff),
                "player records use an 18-byte team and six-byte member stride");
    test.assert(member.level, static_cast<uint8_t>((2 * 18 + 2 * 6 + 1) & 0xff),
                "member level follows species");
    test.assert(member.moveIds[3], static_cast<uint8_t>((2 * 18 + 2 * 6 + 5) & 0xff),
                "fourth move ends the six-byte member");
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(1),
                "member reader uses one FX read");
    test.assert(fxDataFake::lastDataAddress,
                ArenaDemoData::playerMembers + 2 * 18 + 2 * 6,
                "member address uses team and slot strides");
    test.assert(fxDataFake::lastDataLength, static_cast<size_t>(6),
                "member reads exactly six bytes");

    const arena::Member savedMember = member;
    const uint32_t readsBeforeInvalidMember = fxDataFake::readCount;
    test.assert(arena::readPlayerMember(3, 0, member), false,
                "player team bound is enforced");
    test.assert(arena::readPlayerMember(255, 255, member), false,
                "255 player indices are rejected");
    test.assert(arena::readPlayerMember(0, 3, member), false,
                "player slot bound is enforced");
    test.assert(fxDataFake::readCount, readsBeforeInvalidMember,
                "invalid member indices perform no FX reads");
    test.assert(member.species, savedMember.species,
                "invalid member leaves caller output unchanged");

    fxDataFake::dataBase = ArenaDemoData::opponentTrainerIds;
    fxDataFake::dataBytes[0] = 19;
    fxDataFake::dataBytes[4] = 23;
    uint8_t trainer = 0;
    test.assert(arena::readOpponentId(4, trainer), true,
                "last opponent trainer ID is readable");
    test.assert(trainer, static_cast<uint8_t>(23), "opponent ID uses one-byte stride");
    test.assert(fxDataFake::lastDataAddress, ArenaDemoData::opponentTrainerIds + 4,
                "opponent trainer ID uses the fourth one-byte entry");
    const uint32_t readsBeforeInvalidOpponent = fxDataFake::readCount;
    trainer = 77;
    test.assert(arena::readOpponentId(5, trainer), false,
                "opponent bound is enforced");
    test.assert(arena::readOpponentId(255, trainer), false,
                "255 opponent index is rejected");
    test.assert(trainer, static_cast<uint8_t>(77),
                "invalid opponent leaves caller output unchanged");
    test.assert(fxDataFake::readCount, readsBeforeInvalidOpponent,
                "invalid opponent index performs no FX reads");

    fxDataFake::dataBase = ArenaDemoData::playerLabelWidths;
    fxDataFake::dataBytes[0] = 10;
    arena::ArenaPreview preview{};
    FxReadCounter::resetFrame();
    test.assert(arena::loadPreview(arena::ArenaScreen::PlayerTeam, 0, preview), true,
                "player preview loads all member names");
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(8),
                "player preview reads three members and five label/name entries");
    test.assert(preview.species[0], static_cast<uint8_t>(0),
                "species zero is a valid preview member");
    test.assert(preview.species[2], static_cast<uint8_t>(0),
                "player preview preserves member ordering");
    test.assert(preview.labelAddress, ArenaDemoData::playerLabels,
                "player label address uses the player address table");
    test.assert(preview.labelWidth, static_cast<uint8_t>(10),
                "player label width is read at transition time");

    fxDataFake::dataBase = ArenaDemoData::playerLabelWidths;
    fxDataFake::dataBytes[2] = 12;
    FxReadCounter::resetFrame();
    test.assert(arena::loadPreview(arena::ArenaScreen::PlayerTeam, 2, preview), true,
                "last player preview loads");
    test.assert(preview.labelAddress, ArenaDemoData::playerLabels + 2 * 3,
                "player labels use three-byte address table entries");
    test.assert(preview.labelWidth, static_cast<uint8_t>(12),
                "player label widths use one-byte table entries");

    fxDataFake::dataBase = ArenaDemoData::opponentLabelWidths;
    fxDataFake::dataBytes[4] = 14;
    FxReadCounter::resetFrame();
    test.assert(arena::loadPreview(arena::ArenaScreen::OpponentTeam, 4, preview), true,
                "last opponent preview loads");
    test.assert(preview.labelAddress, ArenaDemoData::opponentLabels + 4 * 3,
                "opponent labels use three-byte address table entries");
    test.assert(preview.labelWidth, static_cast<uint8_t>(14),
                "opponent label widths use one-byte table entries");

    arena::ArenaPreview unchanged{};
    unchanged.species[0] = 99;
    const uint32_t readsBeforeInvalidPreview = fxDataFake::readCount;
    test.assert(arena::loadPreview(arena::ArenaScreen::Result, 0, unchanged), false,
                "result screen has no team preview");
    test.assert(arena::loadPreview(arena::ArenaScreen::OpponentTeam, 5, unchanged), false,
                "opponent preview bounds are enforced");
    test.assert(fxDataFake::readCount, readsBeforeInvalidPreview,
                "invalid preview indices perform no FX reads");
    test.assert(unchanged.species[0], static_cast<uint8_t>(99),
                "rejected preview leaves caller output unchanged");

    TestSuite suite("Arena Catalog Suite");
    suite.addTest(test);
    runner.addTestSuite(suite);
}
