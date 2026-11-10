#include "MM6V5LiveSaveCapture.h"

#include <algorithm>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Engine/Components/Random/EngineRandomComponent.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Overlays.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Party.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/DecorationTable.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"
#include "Engine/Tables/OverlayTable.h"
#include "Engine/Tables/TileTable.h"
#include "Engine/Timer.h"
#include "GUI/GUIMessageQueue.h"
#include "GUI/GUIWindow.h"
#include "Io/Mouse.h"
#include "Library/Platform/Application/PlatformApplication.h"

#include "MM6ActiveActorOutdoorDeltaCapture.h"
#include "MM6BuffSaveFields.h"
#include "MM6ChestLiveOwnerAdapter.h"
#include "MM6ChestRuntimeProjection.h"
#include "MM6CoreSaveRuntime.h"
#include "MM6DurableSaveFields.h"
#include "MM6EventRuntimeSaveFields.h"
#include "MM6GameplayRngSaveFields.h"
#include "MM6GeneratedNpcSaveFields.h"
#include "MM6HirelingSaveRuntime.h"
#include "MM6LoadedMapSource.h"
#include "MM6NpcStaticBaseline.h"
#include "MM6NpcSaveFields.h"
#include "MM6OutdoorWire.h"
#include "MM6PersistentEventPublication.h"
#include "MM6PostRebuildPidSlots.h"
#include "MM6RawCursorImagePlan.h"
#include "MM6RawCursorShopProcessOwner.h"
#include "MM6RawPartyInventoryStage.h"
#include "MM6RawV2Admission.h"
#include "MM6RecoverySaveRuntime.h"
#include "MM6ResistanceSaveFields.h"
#include "MM6SavedActorPidPolicy.h"
#include "MM6V4SaveFields.h"
#include "MM6V5FullSessionPreflight.h"
#include "MM6V5SaveFields.h"
#include "MM6V5ActorRuntimeFields.h"
#include "MM6V5DecorationRuntimeFields.h"
#include "MM6V5SpriteRuntimeFields.h"

namespace mm6 {
namespace {

bool hasItemPayload(const Item &item) {
    return item.itemId != ITEM_NULL || item.potionPower || item.goldAmount ||
           item.standardEnchantment || item.standardEnchantmentStrength ||
           item.specialEnchantment != ITEM_ENCHANTMENT_NULL || item.numCharges ||
           item.maxCharges || item.flags || item.lichJarCharacterIndex != -1 ||
           item.enchantmentExpirationTime.ticks() || item.mm6RawRecord;
}

void requireEmptyInheritedCommerce(const Party &party) {
    for (HouseId id : party.standartItemsInShops.indices()) {
        if (party.PartyTimes.shopNextRefreshTime[id].ticks() ||
            party.PartyTimes.shopBanTimes[id].ticks() || party.InTheShopFlags[id])
            throw std::runtime_error("MM6 v5 inherited commerce is not represented");
        for (const Item &item : party.standartItemsInShops[id])
            if (hasItemPayload(item))
                throw std::runtime_error("MM6 v5 inherited commerce is not represented");
        for (const Item &item : party.specialItemsInShops[id])
            if (hasItemPayload(item))
                throw std::runtime_error("MM6 v5 inherited commerce is not represented");
    }
    for (HouseId id : party.spellBooksInGuilds.indices()) {
        if (party.PartyTimes.guildNextRefreshTime[id].ticks())
            throw std::runtime_error("MM6 v5 inherited commerce is not represented");
        for (const Item &item : party.spellBooksInGuilds[id])
            if (hasItemPayload(item))
                throw std::runtime_error("MM6 v5 inherited commerce is not represented");
    }
    for (HouseId id : party.PartyTimes.bountyHuntNextGenTime.indices())
        if (party.PartyTimes.bountyHuntNextGenTime[id].ticks() ||
            party.monster_id_for_hunting[id] != MONSTER_INVALID ||
            party.monster_for_hunting_killed[id])
            throw std::runtime_error("MM6 v5 inherited commerce is not represented");
}

void requireFirstMapLiveBoundary(V5FirstMapCaptureBoundary boundary) {
    if (!engine || engine->gameTitle() != GameTitle::MM6 ||
        !pMapTable || !pParty || !pOutdoor || !pNPCStats || !pItemTable ||
        !pDecorationTable || !pActiveOverlayList || !pOverlayTable ||
        !pTileTable || !pGames_LOD || !gameTimer ||
        !application || !engine->resources() ||
        uCurrentlyLoadedLevelType != LEVEL_OUTDOOR ||
        engine->_currentLoadedMapId != pMapTable->GetMapInfo("oute3.odm") ||
        engine->_pendingTransition || !pPartyActionQueue ||
        pPartyActionQueue->uNumActions != 0)
        throw std::runtime_error("MM6 v5 live capture requires settled first-map owners");
    if (boundary == V5FirstMapCaptureBoundary::SettledInput) {
        if (gameTimer->isPaused())
            throw std::runtime_error("MM6 v5 live capture requires settled first-map input");
    } else if (boundary == V5FirstMapCaptureBoundary::PausedSaveMenu) {
        if (!gameTimer->isPaused() || current_screen_type != SCREEN_SAVEGAME ||
            !pGUIWindow_CurrentMenu ||
            pGUIWindow_CurrentMenu->eWindowType != WINDOW_SAVE ||
            pGUIWindow_CurrentMenu->keyboard_input_status != WINDOW_INPUT_NONE ||
            !engine->_messageQueue || engine->_messageQueue->hasPendingForSave())
            throw std::runtime_error("MM6 v5 live capture requires idle paused Save menu");
    } else {
        throw std::runtime_error("MM6 v5 live capture has unknown boundary");
    }
}

std::vector<std::uint8_t> captureUnchecked(
    std::span<const std::uint8_t> sourceV4,
    V5FirstMapCaptureBoundary boundary) {
    requireFirstMapLiveBoundary(boundary);
    V4SaveFields saved = decodeV4Save(sourceV4, GameTitle::MM6);
    if (saved.v3.raw.metadata.header.mapFileName != "oute3.odm" ||
        saved.eventRuntime.maps.size() != 1 ||
        saved.eventRuntime.maps[0].mapName != "oute3.odm")
        throw std::runtime_error("MM6 v5 live capture lacks first-map v4 source");
    requireRawV2RepresentableState(*pParty, {}, GameTitle::MM6);
    requireEmptyInheritedCommerce(*pParty);
    (void) stageRawCursorImagePlan(*pParty, rawHeldCursorState(GameTitle::MM6),
                                   *pItemTable, GameTitle::MM6);
    const LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
    if (pMapDeltas.size() != maps.expectedNames().size())
        throw std::runtime_error("MM6 v5 live map cache shape changed");

    V3SaveFields &fields = saved.v3;
    fields.raw.core = captureCoreFields(*pParty, GameTitle::MM6);
    const auto durable = captureDurableFields(*pParty, GameTitle::MM6);
    fields.raw.bits = durable.bits;
    fields.raw.effects = durable.effects;
    fields.raw.inventory = captureRawPartyInventories(
        {*pParty, fields.raw.inventory}, *pItemTable, GameTitle::MM6);
    fields.raw.metadata = captureMetadata(
        fields.raw.metadata.header.displayName, GameTitle::MM6);
    fields.raw.npcs = captureNpcFields(*pNPCStats, GameTitle::MM6);
    fields.raw.resistances = captureResistanceFields(*pParty, GameTitle::MM6);
    fields.raw.buffs = captureBuffFields(*pParty, GameTitle::MM6);
    fields.raw.hirelings = captureHirelingFields(*pParty, GameTitle::MM6);
    fields.raw.recovery = captureRecoveryFields(*pParty, GameTitle::MM6);
    const auto held = captureRawHeldCursor(rawHeldCursorState(GameTitle::MM6),
                                           GameTitle::MM6);
    if (!held)
        throw std::runtime_error("MM6 v5 live cursor owner is not representable");
    fields.raw.heldItem = *held;
    fields.raw.shops = shopTitleState();

    const Blob delta = captureOutdoorDeltaWithNativeSpriteAnchors(
        *pOutdoor, "oute3.odm", GameTitle::MM6);
    const auto *deltaBegin = static_cast<const std::uint8_t *>(delta.data());
    const std::vector<std::uint8_t> current(deltaBegin, deltaBegin + delta.size());
    const std::string activeName = "oute3.ddm";
    fields.raw.chest = captureChestSaveFields(chestTitleState(), activeName,
                                              current, GameTitle::MM6);
    for (auto &entry : fields.raw.maps.deltas) {
        const auto it = pMapDeltas.find(entry.name);
        if (it == pMapDeltas.end())
            throw std::runtime_error("MM6 v5 live map cache lacks source entry");
        const auto *begin = static_cast<const std::uint8_t *>(it->second.data());
        const std::vector<std::uint8_t> cached(begin, begin + it->second.size());
        if (entry.name == activeName)
            entry.bytes = current;
        else if (entry.bytes != cached)
            throw std::runtime_error("MM6 v5 nonactive map cache changed");
    }
    const std::vector<Actor> actors(pActors.begin(), pActors.end());
    // ACTR and EVRT source bindings remain v4 anchors. Their dynamic values
    // are owned by separate ARTR and DECR runtime sections.
    fields.generatedNpcs = captureGeneratedNpcFields(
        pActors, *pNPCStats, "oute3.odm", GameTitle::MM6);
    const auto random = application->component<EngineRandomComponent>();
    fields.gameplayRng = captureGameplayRngFields(*random, GameTitle::MM6);
    auto &event = saved.eventRuntime.maps[0];
    event.respawnCount = pOutdoor->ddm.respawnCount;
    event.lastRespawnDay = pOutdoor->ddm.lastRespawnDay;
    event.mapVars = engine->_persistentVariables.mapVars;
    event.decorVars = engine->_persistentVariables.decorVars;

    auto sprites = captureSpriteRuntimeFields(
        "oute3.odm", current, pSpriteObjects, GameTitle::MM6);
    auto actorRuntime = captureActorRuntimeFields(
        "oute3.odm", fields.activeActorPayload, actors, GameTitle::MM6);
    auto decorations = captureDecorationRuntimeFields(
        "oute3.odm", maps.readBase("oute3.odm"), event,
        *pDecorationTable, pLevelDecorations, GameTitle::MM6);
    return encodeV5Save({std::move(saved), std::move(sprites),
                         std::move(actorRuntime), std::move(decorations)},
                        GameTitle::MM6);
}

} // namespace

std::vector<std::uint8_t> captureV5FirstMapLiveSession(
    std::span<const std::uint8_t> sourceV4,
    V5FirstMapCaptureBoundary boundary) {
    auto candidate = captureUnchecked(sourceV4, boundary);
    requireV5FirstMapLiveSessionAgreement(candidate, boundary);
    return candidate;
}

void requireV5FirstMapLiveSessionAgreement(
    std::span<const std::uint8_t> candidateV5,
    V5FirstMapCaptureBoundary boundary) {
    requireFirstMapLiveBoundary(boundary);
    auto saved = std::make_unique<V5SaveFields>(
        decodeV5Save(candidateV5, GameTitle::MM6));
    const std::vector<std::uint8_t> sourceV4 =
        encodeV4Save(saved->v4, GameTitle::MM6);
    if (captureUnchecked(sourceV4, boundary) !=
        std::vector<std::uint8_t>(candidateV5.begin(), candidateV5.end()))
        throw std::runtime_error("MM6 v5 candidate differs from exact live owners");

    const LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
    const NpcStaticBaseline freshNpcs =
        loadNpcStaticBaseline(*engine->resources(), GameTitle::MM6);
    const auto verifyItem = [](std::int32_t id) {
        return id >= 1 && id <= 580 &&
            !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
    };
    auto staged = stageV5FullSessionPreflightOnHeap(
        candidateV5, freshNpcs, maps.expectedNames(),
        [&maps](std::string_view name) { return maps.readBase(name); },
        verifyItem, *pItemTable, pOverlayTable->overlays.size(),
        pTileTable->tileCount(), pActors.size(),
        application->component<EngineRandomComponent>()->type(),
        pLevelDecorations, *pDecorationTable, GameTitle::MM6);
    const auto expectedEvents = capturePersistentEventBoundary(
        staged->v4.v3.represented.persistentEvents, GameTitle::MM6);
    const auto liveWire = decodeOutdoorDelta(pOutdoor->mm6DeltaBytes);
    if (!std::ranges::equal(expectedEvents,
                            liveWire.part(OutdoorDeltaPart::PersistentBytes)) ||
        staged->v4.activeEventRuntime.mapVars != engine->_persistentVariables.mapVars ||
        staged->v4.activeEventRuntime.decorVars != engine->_persistentVariables.decorVars)
        throw std::runtime_error("MM6 v5 live event owner changed");
}

void requireV5FirstMapPostWorldAgreement(
    const V5FullSessionPreflight &staged, std::size_t expectedNativeActors) {
    constexpr std::string_view mapName = "oute3.odm";
    if (!engine || engine->gameTitle() != GameTitle::MM6 || !pMapTable ||
        !pParty || !pOutdoor || !pNPCStats || !pItemTable ||
        !pDecorationTable || !mouse || !gameTimer ||
        !pPartyActionQueue || !pGames_LOD)
        throw std::runtime_error("MM6 v5 post-world resources unavailable");
    if (engine->_currentLoadedMapId != pMapTable->GetMapInfo(mapName) ||
        uCurrentlyLoadedLevelType != LEVEL_OUTDOOR)
        throw std::runtime_error("MM6 v5 post-world loaded map differs");
    if (!gameTimer->isPaused() || engine->_pendingTransition ||
        pPartyActionQueue->uNumActions != 0)
        throw std::runtime_error("MM6 v5 post-world input boundary differs");
    if (staged.v4.saved.v3.raw.metadata.header.mapFileName != mapName)
        throw std::runtime_error("MM6 v5 post-world staged map differs");

    // The test-only pre-event replay scope already owns the moved actor-map
    // baseline. Its native count was captured before that move.
    const std::size_t nativeCount = expectedNativeActors;
    const std::size_t activeCount = staged.actors.actors.size();
    if (activeCount < nativeCount || pActors.size() != activeCount)
        throw std::runtime_error("MM6 v5 post-world actor count differs");
    const auto &expectedGenerated = staged.v4.saved.v3.generatedNpcs;
    requireGeneratedNpcActorAgreement(expectedGenerated, pActors, mapName,
                                      GameTitle::MM6);
    const auto observedGenerated = captureGeneratedNpcFields(
        pActors, *pNPCStats, mapName, GameTitle::MM6);
    if (encodeGeneratedNpcSection(observedGenerated, GameTitle::MM6) !=
        encodeGeneratedNpcSection(expectedGenerated, GameTitle::MM6))
        throw std::runtime_error("MM6 v5 generated NPC rows changed after map events");

    const std::vector<Actor> finalActors(pActors.begin(), pActors.end());
    const auto finalActorRuntime = captureActorRuntimeFields(
        mapName, staged.v4.saved.v3.activeActorPayload, finalActors,
        GameTitle::MM6);
    if (encodeActorRuntimeSection(finalActorRuntime, GameTitle::MM6) !=
        encodeActorRuntimeSection(staged.actors, GameTitle::MM6))
        throw std::runtime_error("MM6 v5 ARTR changed after map events");
    const auto finalSprites = captureSpriteRuntimeFields(
        mapName, pOutdoor->mm6DeltaBytes, pSpriteObjects, GameTitle::MM6);
    if (encodeSpriteRuntimeSection(finalSprites, GameTitle::MM6) !=
        encodeSpriteRuntimeSection(staged.sprites, GameTitle::MM6))
        throw std::runtime_error("MM6 v5 SPRT changed after map events");

    if (engine->_persistentVariables.mapVars !=
            staged.v4.activeEventRuntime.mapVars ||
        engine->_persistentVariables.decorVars !=
            staged.v4.activeEventRuntime.decorVars)
        throw std::runtime_error("MM6 v5 EVRT arrays changed after map events");
    const LoadedMapSource loadedMaps(*pGames_LOD, *pMapTable, GameTitle::MM6);
    const auto finalDecorations = captureDecorationRuntimeFields(
        mapName, loadedMaps.readBase(mapName), staged.v4.activeEventRuntime,
        *pDecorationTable, pLevelDecorations, GameTitle::MM6);
    if (encodeDecorationRuntimeSection(finalDecorations, GameTitle::MM6) !=
        encodeDecorationRuntimeSection(staged.decorations, GameTitle::MM6))
        throw std::runtime_error("MM6 v5 DECR changed after map events");

    const auto outdoorWire = decodeOutdoorDelta(pOutdoor->mm6DeltaBytes);
    const auto nativeBytes = outdoorWire.part(OutdoorDeltaPart::PersistentBytes);
    const auto expectedNative = capturePersistentEventBoundary(
        staged.v4.v3.represented.persistentEvents, GameTitle::MM6);
    if (nativeBytes.size() != expectedNative.size() ||
        !std::equal(nativeBytes.begin(), nativeBytes.end(),
                    expectedNative.begin(), expectedNative.end()))
        throw std::runtime_error("MM6 v5 native event bytes changed after map events");

    const auto &savedRaw = staged.v4.saved.v3.raw;
    if (!staged.v4.v3.represented.outdoorPlan ||
        staged.v4.v3.represented.outdoorPlan->mapName != mapName)
        throw std::runtime_error("MM6 v5 final map differs from HEAD");
    requireNoActiveSavedActorOverlays(savedRaw.metadata.overlays, GameTitle::MM6);
    requirePostRebuildPidSlotAgreement(
        *staged.v4.v3.represented.outdoorPlan, mapName,
        std::span<const Actor>(finalActors).first(nativeCount), pSpriteObjects,
        GameTitle::MM6);

    const auto &chest = chestTitleState();
    if (!chest.ready || chest.lcgState != savedRaw.chest.lcgState ||
        chest.generatedArtifactFlags != savedRaw.chest.generatedArtifactFlags ||
        pOutdoor->mm6DeltaBytes != savedRaw.chest.rawActiveDelta ||
        vChests.size() != 20)
        throw std::runtime_error("MM6 v5 final CHST owner changed");
    ChestRecordSet chestRecords{};
    if (!copyChestRecords(outdoorWire.part(OutdoorDeltaPart::Chests),
                          &chestRecords))
        throw std::runtime_error("MM6 v5 final chest section is incomplete");
    for (int index = 0; index < 20; ++index) {
        ChestRuntimeProjection projection{vChests[index], chestRecords[index]};
        const auto replayed = stageChestRuntimeRecord(GameTitle::MM6, projection);
        if (!replayed || *replayed != chestRecords[index])
            throw std::runtime_error("MM6 v5 final chest projection differed");
    }

    if (captureCoreFields(*pParty, GameTitle::MM6) != savedRaw.core)
        throw std::runtime_error("MM6 v5 PART/CHAR owner changed after map events");
    const auto durable = captureDurableFields(*pParty, GameTitle::MM6);
    if (durable.bits != savedRaw.bits || durable.effects != savedRaw.effects)
        throw std::runtime_error("MM6 v5 QEST/EFFT owner changed after map events");
    if (captureResistanceFields(*pParty, GameTitle::MM6) !=
            savedRaw.resistances ||
        captureBuffFields(*pParty, GameTitle::MM6) != savedRaw.buffs ||
        captureRecoveryFields(*pParty, GameTitle::MM6) != savedRaw.recovery)
        throw std::runtime_error("MM6 v5 Party resistance, buff or recovery owner changed");
    if (encodeHirelingSection(captureHirelingFields(*pParty, GameTitle::MM6)) !=
        encodeHirelingSection(savedRaw.hirelings))
        throw std::runtime_error("MM6 v5 HIRE owner changed after map events");
    StagedRawPartyInventories liveInventories{*pParty, savedRaw.inventory};
    if (captureRawPartyInventories(liveInventories, *pItemTable,
                                   GameTitle::MM6) != savedRaw.inventory)
        throw std::runtime_error("MM6 v5 INVR owner changed after map events");
    requirePostWorldEmptyCursorReady(
        staged.v4.v3.cursorImage, *pParty, *mouse,
        engine->config->graphics.AlwaysCustomCursor.value(), GameTitle::MM6);
    requirePostWorldArmageddonResumeAgreement(
        *pParty, staged.v4.v3.armageddon, LEVEL_OUTDOOR, GameTitle::MM6);
}

} // namespace mm6
