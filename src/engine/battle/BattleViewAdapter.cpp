#include "BattleViewAdapter.hpp"

namespace battle {

BattleView legacyBattleView(const BattleSession &session)
{
    return session.view();
}

} // namespace battle
