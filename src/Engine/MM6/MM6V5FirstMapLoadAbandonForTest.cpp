#include "MM6V5FirstMapLoadAbandonForTest.h"

#include <stdexcept>

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/EngineEnums.h"
#include "Engine/Party.h"
#include "Engine/Timer.h"

#include "MM6V5FirstMapLoadPreflight.h"

namespace mm6::testing {
namespace {

thread_local ScopedV5FirstMapLoadAbandonForTest *active = nullptr;

} // namespace

ScopedV5FirstMapLoadAbandonForTest::ScopedV5FirstMapLoadAbandonForTest(
    const V5FullSessionPreflight &staged, GameTitle title) {
    if (title != GameTitle::MM6 || !engine || engine->gameTitle() != title || active)
        throw std::runtime_error("MM6 first-map load abandon needs a sole title-local transaction");
    requireV5FirstMapPrepublicationAgreement(staged, title);
    active = this;
}

ScopedV5FirstMapLoadAbandonForTest::~ScopedV5FirstMapLoadAbandonForTest() {
    if (active == this)
        active = nullptr;
}

bool abandonV5FirstMapLoadFailureForTest() noexcept {
    if (!active || active->_abandoned || !engine ||
        engine->gameTitle() != GameTitle::MM6)
        return false;
    active->_abandoned = true;
    // No rollback is claimed. Remove only the route that could resume a
    // partially published world, then let Game::loop return to Main Menu.
    engine->_pendingTransition.reset();
    engineFlags &= ~(ENGINE_LOADING_SAVEGAME | ENGINE_SKIP_NEXT_WORLD_UPDATE |
                     ENGINE_SKIP_NEXT_USER_INPUT);
    if (gameTimer)
        gameTimer->setPaused(true);
    if (pPartyActionQueue)
        pPartyActionQueue->uNumActions = 0;
    uGameState = GAME_STATE_GAME_QUITTING_TO_MAIN_MENU;
    return true;
}

} // namespace mm6::testing
