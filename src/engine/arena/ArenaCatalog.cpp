#include "ArenaCatalog.hpp"

#include "../../../fxdata/generated/arena_demo_ids.hpp"
#include "../../lib/FxRead.hpp"
#include "../../fxdata.h"
#include "../../lib/ReadData.hpp"

namespace arena {
namespace {

constexpr uint8_t kMemberBytes = ArenaDemoIds::MemberBytes;
constexpr uint8_t kPlayerMemberBytes = ArenaDemoIds::PlayerBytes;
constexpr uint8_t kOpponentSpeciesBytes = ArenaDemoIds::OpponentSpeciesBytes;

bool readLabel(ArenaScreen screen, uint8_t index, ArenaPreview &preview)
{
    const uint24_t addressTable = screen == ArenaScreen::PlayerTeam
        ? ArenaDemoData::playerLabels : ArenaDemoData::opponentLabels;
    const uint24_t widthTable = screen == ArenaScreen::PlayerTeam
        ? ArenaDemoData::playerLabelWidths : ArenaDemoData::opponentLabelWidths;
    preview.labelAddress = FxRead::indexed24(addressTable, index);
    FxRead::bytes(widthTable + index, &preview.labelWidth, 1);
    return preview.labelWidth != 0;
}

bool readName(uint8_t species, uint24_t &address, uint8_t &width)
{
    width = readCreatureNameWidth(species);
    // The canonical width table has one entry per species and returns zero
    // outside that table. Check it before touching the FX address table.
    if (width == 0) return false;
    address = readCreatureNameAddress(species);
    return true;
}

} // namespace

bool readPlayerMember(uint8_t team, uint8_t slot, Member &out)
{
    if (team >= ArenaDemoIds::playerCount || slot >= 3) return false;

    uint8_t bytes[kMemberBytes];
    const uint24_t address = ArenaDemoData::playerMembers +
        static_cast<uint24_t>(team) * kPlayerMemberBytes +
        static_cast<uint24_t>(slot) * kMemberBytes;
    FxRead::bytes(address, bytes, sizeof(bytes));
    Member member{bytes[0], bytes[1], {bytes[2], bytes[3], bytes[4], bytes[5]}};
    out = member;
    FxReadCounter::transitionExact(1);
    return true;
}

bool readOpponentId(uint8_t opponent, uint8_t &out)
{
    if (opponent >= ArenaDemoIds::opponentCount) return false;

    uint8_t trainerId;
    FxRead::bytes(ArenaDemoData::opponentTrainerIds + opponent, &trainerId, 1);
    out = trainerId;
    FxReadCounter::transitionExact(1);
    return true;
}

bool loadPreview(ArenaScreen screen, uint8_t index, ArenaPreview &out)
{
    if (screen != ArenaScreen::PlayerTeam && screen != ArenaScreen::OpponentTeam)
        return false;
    const uint8_t count = screen == ArenaScreen::PlayerTeam
        ? ArenaDemoIds::playerCount : ArenaDemoIds::opponentCount;
    if (index >= count) return false;

    ArenaPreview preview{};
    if (screen == ArenaScreen::PlayerTeam) {
        for (uint8_t slot = 0; slot < 3; ++slot) {
            Member member{};
            if (!readPlayerMember(index, slot, member)) return false;
            preview.species[slot] = member.species;
        }
    } else {
        FxRead::bytes(ArenaDemoData::opponentSpecies +
                      static_cast<uint24_t>(index) * kOpponentSpeciesBytes,
                      preview.species, sizeof(preview.species));
    }

    if (!readLabel(screen, index, preview)) return false;
    for (uint8_t slot = 0; slot < 3; ++slot) {
        if (!readName(preview.species[slot], preview.nameAddress[slot],
                      preview.nameWidth[slot])) return false;
    }

    out = preview;
    if (screen == ArenaScreen::PlayerTeam) {
        // The three member reads each approve their own one-read transition.
        // Approve the label address, label width, and three creature names.
        FxReadCounter::transitionExact(5);
    } else {
        // Species triple, label address/width, and three creature names.
        FxReadCounter::transitionExact(6);
    }
    return true;
}

} // namespace arena
