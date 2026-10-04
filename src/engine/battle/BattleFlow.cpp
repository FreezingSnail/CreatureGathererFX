#include "BattleFlow.hpp"

#include "BattleSession.hpp"
#include "../ModeState.hpp"
#include "../menu/MenuV2.hpp"
#include "../../GameState.hpp"
#include "../../globals.hpp"

namespace BattleFlow {
namespace {

bool menuAt(MenuEnum type) {
    return menu.menuPointer >= 0 && menu.stack[menu.menuPointer] == type;
}

void openChoice() {
    if (menuAt(BATTLE_OPTIONS) || menuAt(BATTLE_MOVE_SELECT) ||
        menuAt(BATTLE_CREATURE_SELECT)) return;
    menu.clear();
    menu.push(BATTLE_OPTIONS);
}

void openReplacement(battle::BattleSession &session) {
    if (menuAt(BATTLE_CREATURE_SELECT)) return;
    menu.clear();
    menu.openMenu(BATTLE_CREATURE_SELECT, session.view());
    menu.setPartySnapshot(session.partyChoices());
}

void beginResult(battle::BattleSession &session,
                 battle::BattlePresenter &presenter) {
    if (session.advance()) presenter.begin(session.result());
}

} // namespace

bool update(uint8_t edgeButtons) {
    battle::BattleSession &session = battleSession();
    battle::BattlePresenter &presenter = battlePresenter();

    // A result exclusively owns its completion edge. The next action/menu
    // cannot consume that same edge after a phase transition.
    if (presenter.stage() != battle::PresenterStage::Idle) {
        if (!presenter.done()) {
            presenter.update((edgeButtons & MENU_EDGE_A) != 0);
        } else {
            session.finishPresentation();
            presenter.reset();
            if (session.exitReady()) {
                menu.clear();
                dialogMenu.clear();
                exitBattle();
                gameState.state = GameState_t::WORLD;
                return true;
            }
        }
        return false;
    }

    if (!session.isActive()) return false;
    if (dialogMenu.peek()) dialogMenu.clear();

    if (session.awaitingReplacement() || session.awaitingPlayer()) {
        if (session.awaitingReplacement()) openReplacement(session);
        else openChoice();
        const MenuIntent intent = menu.run(session, edgeButtons);
        if (intent.kind != MenuIntentKind::None && session.submitIntent(intent)) {
            menu.clear();
            beginResult(session, presenter);
        }
        return false;
    }

    beginResult(session, presenter);
    return false;
}

} // namespace BattleFlow
