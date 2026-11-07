#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/MM6/MM6BuffSaveFields.h"
#include "Engine/MM6/MM6ChestSaveFields.h"
#include "Engine/MM6/MM6ChestRuntimeProjection.h"
#include "Engine/MM6/MM6CoreSaveRuntime.h"
#include "Engine/MM6/MM6DurableSaveFields.h"
#include "Engine/MM6/MM6HirelingSaveRuntime.h"
#include "Engine/MM6/MM6NpcSaveFields.h"
#include "Engine/MM6/MM6RawCursorImagePlan.h"
#include "Engine/MM6/MM6RawCursorShopProcessOwner.h"
#include "Engine/MM6/MM6RawPartyInventoryStage.h"
#include "Engine/MM6/MM6RawSaveFields.h"
#include "Engine/MM6/MM6RawSessionStage.h"
#include "Engine/MM6/MM6RawV2Admission.h"
#include "Engine/MM6/MM6RecoverySaveRuntime.h"
#include "Engine/MM6/MM6ResistanceSaveFields.h"
#include "Engine/MM6/MM6SaveMetadata.h"
#include "Engine/SaveLoad.h"
#include "Engine/Snapshots/MM6IndoorSnapshot.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/MapTable.h"

namespace mm6::testing {

inline void requireV6IndoorRawSectionsEqualForTest(
    const RawSaveFields &actual, const RawSaveFields &expected,
    std::string_view context) {
    const auto observed = decodeSaveContainer(encodeRawSave(actual, GameTitle::MM6));
    const auto saved = decodeSaveContainer(encodeRawSave(expected, GameTitle::MM6));
    if (observed.sections.size() != saved.sections.size())
        throw std::runtime_error(std::string(context) + " section count differs");
    for (std::size_t index = 0; index < saved.sections.size(); ++index) {
        const auto &a = observed.sections[index];
        const auto &b = saved.sections[index];
        if (a.tag != b.tag || a.required != b.required || a.payload != b.payload)
            throw std::runtime_error(std::string(context) + " section " +
                                     std::string(b.tag.begin(), b.tag.end()) + " differs");
    }
}

// IART, ISRT, IDRT, IFOV, and EVRT own their separate mutable state. Every
// remaining live DLV field must still equal its retained native sidecar here;
// a changed chest, door, outline, or reset counter needs a new exact owner.
inline void requireV6IndoorNativeResidualsForTest(
    const MM6IndoorSnapshot &base) {
    if (!pIndoor || pIndoor->mm6DeltaBytes.empty())
        throw std::runtime_error("MM6 v6 indoor residual check lacks DLV sidecar");
    auto native = decodeMM6IndoorRuntimeDelta(
        Blob::view(pIndoor->mm6DeltaBytes.data(), pIndoor->mm6DeltaBytes.size()), base);
    if (pIndoor->dlv.respawnCount < 0 || pIndoor->dlv.lastRespawnDay < 0 ||
        native.respawnCount != static_cast<std::uint32_t>(pIndoor->dlv.respawnCount) ||
        native.lastRespawnDay != static_cast<std::uint32_t>(pIndoor->dlv.lastRespawnDay) ||
        native.visibleOutlines != pIndoor->_visible_outlines)
        throw std::runtime_error("MM6 v6 indoor DLV reset or outline owner changed");
    if (native.chests.size() != vChests.size() ||
        native.doors.size() != pIndoor->doors.size() ||
        native.doorsData != pIndoor->doorsData)
        throw std::runtime_error("MM6 v6 indoor DLV chest or door shape changed");
    const auto &chestBytes = native.wire.section(MM6IndoorSectionKind::Chests).bytes;
    for (std::size_t slot = 0; slot < native.chests.size(); ++slot) {
        ChestRecord record{};
        const auto *start = static_cast<const std::uint8_t *>(chestBytes.data());
        std::copy_n(start + 4 + slot * record.size(), record.size(), record.begin());
        auto projection = stageChestRuntimeProjection(
            GameTitle::MM6, record, static_cast<int>(slot));
        if (!projection)
            throw std::runtime_error("MM6 v6 indoor chest DLV cannot stage exactly");
        projection->chest = vChests[slot];
        const auto recaptured = stageChestRuntimeRecord(GameTitle::MM6, *projection);
        if (!recaptured || *recaptured != record)
            throw std::runtime_error("MM6 v6 indoor chest runtime lacks exact DLV owner");
    }
    for (std::size_t slot = 0; slot < native.doors.size(); ++slot) {
        BLVDoor_MM7 live{};
        snapshot(pIndoor->doors[slot], &live);
        const auto &source = native.doors[slot];
        // Native pointer words and padding are retained verbatim in the DLV;
        // the loader reconstructs their runtime aliases from doorsData.
        if (live.attributes != source.attributes || live.doorId != source.doorId ||
            live.timeSinceTriggered != source.timeSinceTriggered ||
            live.direction != source.direction || live.moveLength != source.moveLength ||
            live.openSpeed != source.openSpeed || live.closeSpeed != source.closeSpeed ||
            live.numVertices != source.numVertices || live.numFaces != source.numFaces ||
            live.numSectors != source.numSectors || live.numOffsets != source.numOffsets ||
            live.state != source.state)
            throw std::runtime_error("MM6 v6 indoor door runtime lacks exact DLV owner");
    }
}

// Read all v2 title owners from one settled indoor game-thread callback. The
// native DLV remains a source anchor; v6 owns actor/sprite/decoration dynamics.
inline std::unique_ptr<RawSaveFields> captureV6IndoorLiveRawOwnersForTest(
    const RawSaveFields &source, std::string_view mapName) {
    if (!engine || engine->gameTitle() != GameTitle::MM6 || !pIndoor || !pParty ||
        !pNPCStats || !pItemTable || !pMapTable || !pPartyActionQueue ||
        pPartyActionQueue->uNumActions != 0 || pMapDeltas.empty() ||
        uCurrentlyLoadedLevelType != LEVEL_INDOOR || !pIndoor->bLoaded ||
        pIndoor->filename != mapName || engine->_pendingTransition ||
        engine->_currentLoadedMapId < MAP_FIRST ||
        engine->_currentLoadedMapId > MAP_LAST ||
        pMapTable->pInfos[engine->_currentLoadedMapId].fileName != mapName ||
        source.metadata.header.mapFileName != mapName)
        throw std::runtime_error("MM6 v6 live raw capture lacks one settled indoor map");
    requireRawV2RepresentableState(*pParty, {}, GameTitle::MM6);
    (void) stageRawCursorImagePlan(
        *pParty, rawHeldCursorState(GameTitle::MM6), *pItemTable, GameTitle::MM6);
    if (pMapDeltas.size() != source.maps.deltas.size())
        throw std::runtime_error("MM6 v6 live raw map-cache count changed");

    auto result = std::make_unique<RawSaveFields>(source);
    result->core = captureCoreFields(*pParty, GameTitle::MM6);
    const auto durable = captureDurableFields(*pParty, GameTitle::MM6);
    result->bits = durable.bits;
    result->effects = durable.effects;
    result->inventory = captureRawPartyInventories(
        {*pParty, source.inventory}, *pItemTable, GameTitle::MM6);
    result->metadata = captureMetadata(
        source.metadata.header.displayName, GameTitle::MM6);
    result->npcs = captureNpcFields(*pNPCStats, GameTitle::MM6);
    result->resistances = captureResistanceFields(*pParty, GameTitle::MM6);
    result->buffs = captureBuffFields(*pParty, GameTitle::MM6);
    result->hirelings = captureHirelingFields(*pParty, GameTitle::MM6);
    result->recovery = captureRecoveryFields(*pParty, GameTitle::MM6);
    const auto held = captureRawHeldCursor(
        rawHeldCursorState(GameTitle::MM6), GameTitle::MM6);
    if (!held)
        throw std::runtime_error("MM6 v6 live cursor owner is not representable");
    result->heldItem = *held;
    result->shops = shopTitleState();

    const std::string deltaName = std::string(mapName.substr(0, mapName.size() - 4)) + ".dlv";
    result->chest = captureChestSaveFields(
        chestTitleState(), deltaName, pIndoor->mm6DeltaBytes, GameTitle::MM6);
    bool foundActive = false;
    for (auto &entry : result->maps.deltas) {
        const auto cached = pMapDeltas.find(entry.name);
        if (cached == pMapDeltas.end())
            throw std::runtime_error("MM6 v6 live raw map cache lacks source entry");
        const auto *start = static_cast<const std::uint8_t *>(cached->second.data());
        const std::vector<std::uint8_t> bytes(start, start + cached->second.size());
        if (entry.name == deltaName) {
            if (foundActive)
                throw std::runtime_error("MM6 v6 live raw map cache repeats active delta");
            foundActive = true;
            entry.bytes = pIndoor->mm6DeltaBytes;
        } else if (entry.bytes != bytes) {
            throw std::runtime_error("MM6 v6 nonactive map cache changed");
        }
    }
    if (!foundActive)
        throw std::runtime_error("MM6 v6 live raw map cache lacks active delta");
    (void) encodeRawSave(*result, GameTitle::MM6);
    return result;
}

// The raw bytes are accepted only if every represented family survives a
// detached stage and recapture. This does not assert unrepresented live state.
inline void requireV6IndoorDetachedRawRoundTripForTest(
    const RawSaveFields &saved, const StagedRawSessionFields &staged,
    const ItemTable &table, std::size_t overlayTableCount) {
    if (!staged.indoorPlan || staged.outdoorPlan ||
        staged.raw.saved.metadata.header.mapFileName != saved.metadata.header.mapFileName)
        throw std::runtime_error("MM6 v6 detached raw owner lacks indoor source");
    auto replay = std::make_unique<RawSaveFields>(saved);
    const Party &party = staged.party.party;
    replay->core = captureCoreFields(party, GameTitle::MM6);
    const auto durable = captureDurableFields(party, GameTitle::MM6);
    replay->bits = durable.bits;
    replay->effects = durable.effects;
    replay->inventory = captureRawPartyInventories(
        {party, saved.inventory}, table, GameTitle::MM6);
    replay->npcs = captureNpcFields(staged.world.world.npcs, GameTitle::MM6);
    replay->resistances = captureResistanceFields(party, GameTitle::MM6);
    replay->buffs = captureBuffFields(party, GameTitle::MM6);
    replay->hirelings = captureHirelingFields(party, GameTitle::MM6);
    replay->recovery = captureRecoveryFields(party, GameTitle::MM6);
    const auto held = captureRawHeldCursor(staged.raw.cursor, GameTitle::MM6);
    if (!held)
        throw std::runtime_error("MM6 v6 detached cursor owner is not representable");
    replay->heldItem = *held;
    replay->shops = staged.raw.shops;
    replay->chest = captureChestSaveFields(
        staged.world.chestTitle, saved.chest.activeDeltaName,
        staged.world.rawActiveDelta, GameTitle::MM6);
    const auto visuals = captureSessionVisualFields(
        staged.world.visuals.timer, staged.world.visuals.overlays,
        overlayTableCount, GameTitle::MM6);
    replay->metadata.timer = visuals.timer;
    replay->metadata.overlays = visuals.overlays;
    for (auto &entry : replay->maps.deltas) {
        const auto delta = staged.world.world.mapDeltas.find(entry.name);
        if (delta == staged.world.world.mapDeltas.end())
            throw std::runtime_error("MM6 v6 detached raw map is missing");
        const auto *start = static_cast<const std::uint8_t *>(delta->second.data());
        entry.bytes.assign(start, start + delta->second.size());
    }
    requireV6IndoorRawSectionsEqualForTest(
        *replay, saved, "MM6 v6 detached raw owner");
}

} // namespace mm6::testing
