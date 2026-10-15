#pragma once

#include "fxtest.hpp"
#include "src/engine/arena/ArenaCatalog.hpp"
#include "src/lib/FxReadCounter.hpp"
#include "arena_demo_ids.hpp"

inline void test_arenacatalog(FxTest &test)
{
    arena::Member member{};
    FxReadCounter::resetFrame();
    test.expectEq(arena::readPlayerMember(ArenaDemoIds::player_blitz, 0, member),
                  true, F("first player member reads"));
    test.expectEq(member.species, static_cast<uint8_t>(2),
                  F("first player species"));
    test.expectEq(member.level, static_cast<uint8_t>(31),
                  F("first player level"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(1),
                  F("player member transition reads exactly once"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("player member transition allowance is exact"));

    FxReadCounter::resetFrame();
    test.expectEq(arena::readPlayerMember(ArenaDemoIds::player_utility, 2, member),
                  true, F("last player member reads"));
    test.expectEq(member.species, static_cast<uint8_t>(13),
                  F("last player species"));
    test.expectEq(member.moveIds[3], static_cast<uint8_t>(10),
                  F("last player member fourth move"));
    test.expectEq(member.moveIds[2], static_cast<uint8_t>(39),
                  F("last player member third move"));
    test.expectEq(member.moveIds[1], static_cast<uint8_t>(23),
                  F("last player member second move"));
    test.expectEq(member.moveIds[0], static_cast<uint8_t>(11),
                  F("last player member first move"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("last player member transition allowance is exact"));

    FxReadCounter::resetFrame();
    test.expectEq(arena::readPlayerMember(ArenaDemoIds::player_utility, 0, member),
                  true, F("member with empty move slots reads"));
    test.expectEq(member.moveIds[2], static_cast<uint8_t>(32),
                  F("empty move sentinel is preserved"));
    test.expectEq(member.moveIds[3], static_cast<uint8_t>(32),
                  F("second empty move sentinel is preserved"));

    arena::Member unchangedMember = member;
    const uint8_t readsBeforeInvalid = FxReadCounter::count();
    test.expectEq(arena::readPlayerMember(ArenaDemoIds::playerCount, 0, member),
                  false, F("invalid team rejected"));
    test.expectEq(arena::readPlayerMember(0, 3, member), false,
                  F("invalid slot rejected"));
    test.expectEq(arena::readPlayerMember(255, 255, member), false,
                  F("255 team and slot rejected"));
    test.expectEq(FxReadCounter::count(), readsBeforeInvalid,
                  F("invalid member indices perform zero reads"));
    test.expectEq(member.species, unchangedMember.species,
                  F("invalid member leaves output unchanged"));

    uint8_t trainer = 0;
    FxReadCounter::resetFrame();
    test.expectEq(arena::readOpponentId(ArenaDemoIds::opponent_starter, trainer),
                  true, F("first opponent trainer ID reads"));
    test.expectEq(trainer, ArenaDemoIds::trainer_starter,
                  F("first opponent trainer ID"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(1),
                  F("opponent trainer transition reads exactly once"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("opponent trainer transition allowance is exact"));
    test.expectEq(arena::readOpponentId(ArenaDemoIds::opponent_champion, trainer),
                  true, F("last opponent trainer ID reads"));
    test.expectEq(trainer, ArenaDemoIds::trainer_champion,
                  F("last opponent trainer ID"));

    arena::ArenaPreview preview{};
    FxReadCounter::resetFrame();
    test.expectEq(arena::loadPreview(arena::ArenaScreen::OpponentTeam,
                                     ArenaDemoIds::opponent_starter, preview),
                  true, F("first opponent preview loads"));
    test.expectEq(preview.species[0], static_cast<uint8_t>(3),
                  F("opponent preview preserves first species"));
    test.expectEq(preview.species[2], static_cast<uint8_t>(0),
                  F("species zero is valid in opponent preview"));
    test.expectEq(preview.labelAddress != 0, true, F("opponent label address loads"));
    test.expectEq(preview.labelWidth != 0, true, F("opponent label width loads"));
    test.expectEq(preview.nameAddress[2] != 0, true,
                  F("species zero name address loads"));
    test.expectEq(preview.nameWidth[2] != 0, true,
                  F("species zero name width loads"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(6),
                  F("opponent preview uses six transition reads"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("opponent preview transition allowance is exact"));

    FxReadCounter::resetFrame();
    test.expectEq(arena::loadPreview(arena::ArenaScreen::OpponentTeam,
                                     ArenaDemoIds::opponent_champion, preview),
                  true, F("last opponent preview loads"));
    test.expectEq(preview.species[0], static_cast<uint8_t>(31),
                  F("last opponent species triple starts at final record"));
    test.expectEq(preview.labelWidth != 0, true, F("last opponent label loads"));

    FxReadCounter::resetFrame();
    test.expectEq(arena::loadPreview(arena::ArenaScreen::PlayerTeam,
                                     ArenaDemoIds::player_blitz, preview),
                  true, F("player preview loads"));
    test.expectEq(preview.species[0], static_cast<uint8_t>(2),
                  F("player preview reads first member"));
    test.expectEq(preview.species[2], static_cast<uint8_t>(7),
                  F("player preview reads third member"));
    test.expectEq(preview.labelWidth != 0, true, F("player label width loads"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(8),
                  F("player preview uses eight transition reads"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("player preview transition allowance is exact"));

    const uint8_t previewSpecies = preview.species[0];
    const uint8_t readsBeforeRejectedPreview = FxReadCounter::count();
    test.expectEq(arena::loadPreview(arena::ArenaScreen::Result, 0, preview),
                  false, F("result screen preview rejected"));
    test.expectEq(arena::loadPreview(arena::ArenaScreen::PlayerTeam,
                                     ArenaDemoIds::playerCount, preview),
                  false, F("invalid player preview index rejected"));
    test.expectEq(arena::loadPreview(arena::ArenaScreen::OpponentTeam,
                                     ArenaDemoIds::opponentCount, preview),
                  false, F("invalid opponent preview index rejected"));
    test.expectEq(arena::loadPreview(arena::ArenaScreen::OpponentTeam, 255, preview),
                  false, F("255 opponent preview index rejected"));
    test.expectEq(FxReadCounter::count(), readsBeforeRejectedPreview,
                  F("rejected preview IDs perform zero reads"));
    test.expectEq(preview.species[0], previewSpecies,
                  F("rejected preview leaves output unchanged"));
}
