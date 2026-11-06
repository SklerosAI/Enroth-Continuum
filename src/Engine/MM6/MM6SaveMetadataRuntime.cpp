#include "MM6SaveMetadata.h"

#include <stdexcept>
#include <string>
#include <utility>

#include "Engine/Engine.h"
#include "Engine/Graphics/Overlays.h"
#include "Engine/Party.h"
#include "Engine/Resources/LOD.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/OverlayTable.h"
#include "Engine/Timer.h"

namespace mm6 {
namespace {

void requireMM6(GameTitle title) {
    if (title != GameTitle::MM6 || !engine || engine->gameTitle() != GameTitle::MM6)
        throw std::runtime_error("MM6 save metadata cannot apply to another title");
}

} // namespace

SaveMetadataFields captureMetadata(const std::string &displayName, GameTitle title) {
    requireMM6(title);
    if (!pMapTable || !pParty || !gameTimer || !pActiveOverlayList ||
        engine->_currentLoadedMapId == MAP_INVALID)
        throw std::runtime_error("MM6 metadata capture requires a loaded map and runtime");

    SaveMetadataFields fields;
    fields.header.displayName = displayName;
    fields.header.mapFileName = pMapTable->pInfos[engine->_currentLoadedMapId].fileName;
    fields.header.playingTicks = pParty->GetPlayingTime().ticks();
    fields.timer.elapsedTicks = gameTimer->time().ticks();
    fields.ui.activeCharacter = pParty->hasActiveCharacter() ? pParty->activeCharacterIndex() : -1;
    for (size_t index = 0; index < fields.overlays.size(); ++index) {
        const ActiveOverlay &source = pActiveOverlayList->pOverlays[index];
        SaveOverlayField &target = fields.overlays[index];
        target.tableIndex = source.indexToOverlayTable;
        target.frameTime = source.spriteFrameTime;
        target.animationLength = source.animLength;
        target.x = source.screenSpaceX;
        target.y = source.screenSpaceY;
        target.packedPid = source.pid.packed();
        target.projectedSize = source.projSize;
        target.damageModifier = source.fpDamageMod;
    }
    (void) encodeMetadataSections(fields);
    (void) stageHeader(fields.header, title);
    (void) stageOverlays(*pActiveOverlayList, fields.overlays, title);
    return fields;
}

SaveHeaderFields stageHeader(const SaveHeaderFields &saved, GameTitle title) {
    requireMM6(title);
    if (!pMapTable || !pGames_LOD)
        throw std::runtime_error("MM6 map table and archive must be loaded before header staging");

    SaveMetadataFields probe;
    probe.header = saved;
    (void) encodeMetadataSections(probe);
    bool knownMap = false;
    for (MapId id : pMapTable->pInfos.indices())
        if (pMapTable->pInfos[id].fileName == saved.mapFileName) {
            knownMap = true;
            break;
        }
    if (!knownMap || !pGames_LOD->exists(saved.mapFileName))
        throw std::runtime_error("MM6 save refers to an unavailable map");
    return saved;
}

Timer stageEventTimer(const Timer &live, const SaveTimerFields &saved, GameTitle title) {
    requireMM6(title);
    if (saved.elapsedTicks < 0)
        throw std::runtime_error("negative MM6 simulation time");
    Timer staged = live;
    staged.restorePausedSimulationTime(Duration::fromTicks(saved.elapsedTicks));
    return staged;
}

ActiveOverlayList stageOverlays(const ActiveOverlayList &live,
                                const std::array<SaveOverlayField, 50> &saved, GameTitle title) {
    requireMM6(title);
    if (!pOverlayTable)
        throw std::runtime_error("MM6 overlay table must be loaded before overlay staging");

    ActiveOverlayList staged = live;
    for (size_t index = 0; index < saved.size(); ++index) {
        const SaveOverlayField &source = saved[index];
        if (source.animationLength > 0 &&
            (source.tableIndex < 0 || source.tableIndex >= std::ssize(pOverlayTable->overlays)))
            throw std::runtime_error("MM6 active overlay table index exceeds loaded table");
        ActiveOverlay &target = staged.pOverlays[index];
        target.indexToOverlayTable = source.tableIndex;
        target.spriteFrameTime = source.frameTime;
        target.animLength = source.animationLength;
        target.screenSpaceX = source.x;
        target.screenSpaceY = source.y;
        target.pid = Pid::fromPacked(source.packedPid);
        target.projSize = source.projectedSize;
        target.fpDamageMod = source.damageModifier;
    }
    return staged;
}

} // namespace mm6
