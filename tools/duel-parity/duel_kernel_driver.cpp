// Host parity driver for the RPS duel kernel (CreatureGathererFX-d73i.8).
//
// This is a small, host-only line-protocol harness around the *actual*
// src/engine/battle/DuelKernel.hpp helpers. It does not reimplement the kernel:
// every draw goes through battle::Rng and every rule through duel::resolveRound
// / duel::chooseAction / duel::predictPlayerType. tools/tests/duel-kernel-parity_test.mjs
// feeds it identical states and compares the results against the canonical
// mock (mock/battle-mock.js).
//
// The bounded draw is xorshift32 with an integer floor stream, identical to the
// mock's makeRng: advance the state, then (state * bound) >> 32. This keeps
// JavaScript `Math.floor(state / 2^32 * bound)` and C++ bit-for-bit equal for
// the small bounds used here.
//
// Commands (one per line on stdin):
//   STATE <c0> <c1> <active0> <active1> <outcome>   reset the duel state
//   MEM <side> <slot> <id> <typeCount> <t0> <t1> <hp> <maxHp> <optionCount> <o...>
//   CLEARHIST                                       empty the revealed history
//   HIST <count> <h...>                             append revealed player types
//   SEED <seed> <skip>                              reset the continuous rng
//   AICONT <persona> <readRate>                     chooseAction side 1 (continuous rng)
//   AIFRESH <seed> <persona> <readRate>             chooseAction side 1 (fresh rng)
//   PREDICT <persona> <seed> <count> <h...>         predictPlayerType
//   ROLL <seed> <n> <bound...>                      bounded draw stream
//   ACT <k0> <i0> <k1> <i1> <recordType>            resolve one round
//   DONE                                            exit
//
// Each command prints exactly one result line: OK / RES / PICK / PRED / ROLLS.

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

#include "src/engine/battle/DuelKernel.hpp"

namespace {

uint32_t g_rng = 1;

uint32_t xorshift32(uint32_t x)
{
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

uint8_t rngRoll(uint8_t bound)
{
    g_rng = xorshift32(g_rng);
    if (bound == 0) return 0;
    return static_cast<uint8_t>((static_cast<uint64_t>(g_rng) * bound) >> 32);
}

battle::Rng makeRng()
{
    battle::Rng rng;
    rng.next = &rngRoll;
    return rng;
}

void seedRng(uint32_t seed, uint32_t skip)
{
    g_rng = seed ? seed : 1u;
    for (uint32_t i = 0; i < skip; ++i) rngRoll(1);
}

int intOf(const std::string &token)
{
    return static_cast<int>(std::strtol(token.c_str(), nullptr, 10));
}

} // namespace

int main()
{
    duel::State state = {};
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string cmd;
        if (!(in >> cmd)) continue;

        if (cmd == "STATE") {
            int c0 = 0, c1 = 0, a0 = 0, a1 = 0, outcome = 0;
            in >> c0 >> c1 >> a0 >> a1 >> outcome;
            state = {};
            state.sides[0].count = static_cast<uint8_t>(c0);
            state.sides[1].count = static_cast<uint8_t>(c1);
            state.sides[0].active = static_cast<uint8_t>(a0);
            state.sides[1].active = static_cast<uint8_t>(a1);
            state.outcome = static_cast<duel::Outcome>(outcome);
            std::cout << "OK\n";
        } else if (cmd == "MEM") {
            int side = 0, slot = 0, id = 0, tc = 0, t0 = 0, t1 = 0, hp = 0,
                maxHp = 0, oc = 0;
            in >> side >> slot >> id >> tc >> t0 >> t1 >> hp >> maxHp >> oc;
            duel::Member &m = state.sides[side].members[slot];
            m = {};
            m.id = static_cast<uint8_t>(id);
            m.typeCount = static_cast<uint8_t>(tc);
            m.types[0] = static_cast<uint8_t>(t0);
            m.types[1] = static_cast<uint8_t>(t1);
            m.hp = static_cast<uint8_t>(hp);
            m.maxHp = static_cast<uint8_t>(maxHp);
            // The wire line still lists per-option folded types; the kernel now
            // stores the equivalent 5-bit attack mask. Full mask parity
            // regeneration is CreatureGathererFX-4w89.6.
            uint8_t mask = 0;
            for (int i = 0; i < oc; ++i) {
                int o = 0;
                in >> o;
                if (o >= 0 && o < duel::TYPE_COUNT)
                    mask |= static_cast<uint8_t>(1u << o);
            }
            m.attackMask = mask;
            std::cout << "OK\n";
        } else if (cmd == "CLEARHIST") {
            state.historyCount = 0;
            for (uint8_t i = 0; i < duel::HISTORY_CAP; ++i) state.history[i] = 0;
            std::cout << "OK\n";
        } else if (cmd == "HIST") {
            int count = 0;
            in >> count;
            state.historyCount = 0;
            for (uint8_t i = 0; i < duel::HISTORY_CAP; ++i) state.history[i] = 0;
            for (int i = 0; i < count; ++i) {
                int h = 0;
                in >> h;
                duel::recordPlayerHistory(state, static_cast<uint8_t>(h));
            }
            std::cout << "OK\n";
        } else if (cmd == "SEED") {
            long seed = 1, skip = 0;
            in >> seed >> skip;
            seedRng(static_cast<uint32_t>(seed), static_cast<uint32_t>(skip));
            std::cout << "OK\n";
        } else if (cmd == "AICONT" || cmd == "AIFRESH") {
            if (cmd == "AIFRESH") {
                long seed = 1;
                in >> seed;
                seedRng(static_cast<uint32_t>(seed), 0);
            }
            int persona = 0, readRate = 0;
            in >> persona >> readRate;
            battle::Rng rng = makeRng();
            const duel::Action action = duel::chooseAction(
                state, static_cast<duel::Persona>(persona),
                static_cast<uint8_t>(readRate), rng);
            std::cout << "PICK " << static_cast<int>(action.kind) << ' '
                      << static_cast<int>(action.index) << '\n';
        } else if (cmd == "PREDICT") {
            int persona = 0, count = 0;
            long seed = 1;
            in >> persona >> seed >> count;
            uint8_t history[duel::HISTORY_CAP] = {};
            for (int i = 0; i < count && i < duel::HISTORY_CAP; ++i) {
                int h = 0;
                in >> h;
                history[i] = static_cast<uint8_t>(h);
            }
            seedRng(static_cast<uint32_t>(seed), 0);
            battle::Rng rng = makeRng();
            const uint8_t predicted = duel::predictPlayerType(
                history, static_cast<uint8_t>(count),
                static_cast<duel::Persona>(persona), rng);
            std::cout << "PRED " << static_cast<int>(predicted) << '\n';
        } else if (cmd == "ROLL") {
            long seed = 1;
            int count = 0;
            in >> seed >> count;
            seedRng(static_cast<uint32_t>(seed), 0);
            std::cout << "ROLLS";
            for (int i = 0; i < count; ++i) {
                std::string bound;
                in >> bound;
                std::cout << ' ' << static_cast<int>(rngRoll(
                    static_cast<uint8_t>(intOf(bound))));
            }
            std::cout << '\n';
        } else if (cmd == "ACT") {
            int k0 = 0, i0 = 0, k1 = 0, i1 = 0, record = -1;
            in >> k0 >> i0 >> k1 >> i1 >> record;
            const duel::Action actions[2] = {
                {static_cast<duel::ActionKind>(k0), static_cast<uint8_t>(i0)},
                {static_cast<duel::ActionKind>(k1), static_cast<uint8_t>(i1)}};
            battle::Rng rng = makeRng();
            duel::RoundResult out = {};
            duel::resolveRound(state, actions, rng, out);
            if (record >= 0)
                duel::recordPlayerHistory(state, static_cast<uint8_t>(record));

            std::cout << "RES " << static_cast<int>(out.relation) << ' '
                      << static_cast<int>(out.damage[0]) << ' '
                      << static_cast<int>(out.damage[1]) << ' '
                      << static_cast<int>(out.hpBefore[0]) << ' '
                      << static_cast<int>(out.hpBefore[1]) << ' '
                      << static_cast<int>(out.activeId[0]) << ' '
                      << static_cast<int>(out.activeId[1]) << ' '
                      << static_cast<int>(out.previousActive[0]) << ' '
                      << static_cast<int>(out.previousActive[1]) << ' '
                      << static_cast<int>(out.kind[0]) << ' '
                      << static_cast<int>(out.kind[1]) << ' '
                      << static_cast<int>(state.outcome) << ' ';
            std::cout << static_cast<int>(state.sides[0].count);
            for (uint8_t i = 0; i < state.sides[0].count; ++i)
                std::cout << ' ' << static_cast<int>(state.sides[0].members[i].hp);
            std::cout << ' ' << static_cast<int>(state.sides[1].count);
            for (uint8_t i = 0; i < state.sides[1].count; ++i)
                std::cout << ' ' << static_cast<int>(state.sides[1].members[i].hp);
            std::cout << ' ' << static_cast<int>(state.sides[0].active) << ' '
                      << static_cast<int>(state.sides[1].active) << ' '
                      << static_cast<int>(state.historyCount);
            for (uint8_t i = 0; i < state.historyCount; ++i)
                std::cout << ' ' << static_cast<int>(state.history[i]);
            std::cout << '\n';
        } else if (cmd == "DONE") {
            std::cout << "BYE\n";
            std::cout.flush();
            return 0;
        } else {
            std::cout << "ERR " << cmd << '\n';
        }
        std::cout.flush();
    }
    return 0;
}
