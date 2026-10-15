#pragma once

#if defined(BATTLE_SIMULATOR) && !defined(__AVR__)

#include <iosfwd>
#include <string>

#include "BatchRunner.hpp"

namespace battle_sim {

struct FixtureProvenance {
    std::string generatorVersion;
    std::string creatureSha256;
    std::string moveSha256;
    std::string presetSha256;
};

bool readFixtureProvenance(const std::string &manifestPath,
                           FixtureProvenance &provenance,
                           std::string &error);
std::string renderMatchesCsv(const Options &options,
                             const BatchSummary &summary,
                             const FixtureProvenance &provenance);
std::string renderSummaryCsv(const Options &options,
                             const BatchSummary &summary,
                             const FixtureProvenance &provenance);
bool writeReports(const std::string &directory, const Options &options,
                  const BatchSummary &summary,
                  const FixtureProvenance &provenance,
                  std::string &error);
void writeTrace(std::ostream &out, const ReplayResult &result);

} // namespace battle_sim

#endif
