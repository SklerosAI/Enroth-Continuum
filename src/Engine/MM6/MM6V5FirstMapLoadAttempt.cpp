#include "MM6V5FirstMapLoadAttempt.h"

#include <stdexcept>
#include <utility>

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Party.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/MapTable.h"

#include "MM6ChestLiveOwnerAdapter.h"
#include "MM6ChestSeedAndOwnership.h"
#include "MM6V5FirstMapLoadPreflight.h"
#include "MM6V5FullSessionPreflight.h"

namespace mm6 {
namespace {

constexpr std::string_view FIRST_MAP = "oute3.odm";
constexpr std::string_view TEST_SLOT = "candidate-v5-after-action.ec6";

} // namespace

V5FirstMapLoadAttempt::V5FirstMapLoadAttempt(
    std::unique_ptr<V5FullSessionPreflight> staged,
    std::string_view slotName, GameTitle title)
    : _staged(std::move(staged)) {
    if (title != GameTitle::MM6 || slotName != TEST_SLOT || !_staged ||
        !engine || engine->gameTitle() != title)
        throw std::runtime_error("MM6 first-map load attempt lacks exact test slot/title");
    requireV5FirstMapPrepublicationAgreement(*_staged, title);
}

V5FirstMapLoadAttempt::~V5FirstMapLoadAttempt() = default;

V5FullSessionPreflight &V5FirstMapLoadAttempt::staged() { return *_staged; }
const V5FullSessionPreflight &V5FirstMapLoadAttempt::staged() const { return *_staged; }

void V5FirstMapLoadAttempt::notePublished(GameTitle title) {
    if (title != GameTitle::MM6 || _phase != Phase::Detached || !engine ||
        !pMapTable ||
        engine->gameTitle() != title || !engine->_pendingTransition ||
        engine->_pendingTransition->map() != pMapTable->GetMapInfo(FIRST_MAP))
        throw std::runtime_error("MM6 first-map publication has another phase or map");
    _phase = Phase::Published;
}

bool V5FirstMapLoadAttempt::applyChestAtNativeMap(GameTitle title) {
    if (title != GameTitle::MM6 || _phase != Phase::Published ||
        !engine || engine->gameTitle() != title || !pMapTable || !pItemTable ||
        !pParty || !pOutdoor || uCurrentlyLoadedLevelType != LEVEL_OUTDOOR ||
        engine->_currentLoadedMapId != pMapTable->GetMapInfo(FIRST_MAP) ||
        pOutdoor->mm6DeltaBytes !=
            _staged->v4.saved.v3.raw.chest.rawActiveDelta ||
        !chestTitleState().ready ||
        chestTitleState().lcgState != _staged->v4.saved.v3.raw.chest.lcgState ||
        chestTitleState().generatedArtifactFlags !=
            _staged->v4.saved.v3.raw.chest.generatedArtifactFlags)
        throw std::runtime_error("MM6 first-map chest handoff lacks exact native owners");
    const auto owned = scanPartyOwnedChestArtifacts(title, *pParty);
    if (!owned || !applyActiveChestLiveOwnerHandoff(
            title, pMapTable->pInfos[engine->_currentLoadedMapId].mapTreasureLevel,
            *pItemTable, *owned, true))
        throw std::runtime_error("MM6 first-map chest handoff refused");
    _phase = Phase::ChestApplied;
    return true;
}

} // namespace mm6
