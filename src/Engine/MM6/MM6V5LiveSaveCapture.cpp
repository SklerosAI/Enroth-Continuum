#include "MM6V5LiveSaveCapture.h"

#include <algorithm>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "Engine/Components/Random/EngineRandomComponent.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Overlays.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Decoration.h"
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
#include "Library/Platform/Application/PlatformApplication.h"

#include "MM6ActiveActorOutdoorDeltaCapture.h"
#include "MM6BuffSaveFields.h"
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
#include "MM6RawCursorImagePlan.h"
#include "MM6RawCursorShopProcessOwner.h"
#include "MM6RawPartyInventoryStage.h"
#include "MM6RawV2Admission.h"
#include "MM6RecoverySaveRuntime.h"
#include "MM6ResistanceSaveFields.h"
#include "MM6V4SaveFields.h"
#include "MM6V5FullSessionPreflight.h"
#include "MM6V5SaveFields.h"

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

void requireFirstMapLiveBoundary() {
    if (!engine || engine->gameTitle() != GameTitle::MM6 ||
        !pMapTable || !pParty || !pOutdoor || !pNPCStats || !pItemTable ||
        !pDecorationTable || !pActiveOverlayList || !pOverlayTable ||
        !pTileTable || !pGames_LOD || !gameTimer ||
        !application || !engine->resources() ||
        uCurrentlyLoadedLevelType != LEVEL_OUTDOOR ||
        engine->_currentLoadedMapId != pMapTable->GetMapInfo("oute3.odm") ||
        engine->_pendingTransition || !pPartyActionQueue ||
        pPartyActionQueue->uNumActions != 0 || gameTimer->isPaused())
        throw std::runtime_error("MM6 v5 live capture requires settled first-map input");
}

std::vector<std::uint8_t> captureUnchecked(
    std::span<const std::uint8_t> sourceV4) {
    requireFirstMapLiveBoundary();
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
    std::span<const std::uint8_t> sourceV4) {
    auto candidate = captureUnchecked(sourceV4);
    requireV5FirstMapLiveSessionAgreement(candidate);
    return candidate;
}

void requireV5FirstMapLiveSessionAgreement(
    std::span<const std::uint8_t> candidateV5) {
    requireFirstMapLiveBoundary();
    auto saved = std::make_unique<V5SaveFields>(
        decodeV5Save(candidateV5, GameTitle::MM6));
    const std::vector<std::uint8_t> sourceV4 =
        encodeV4Save(saved->v4, GameTitle::MM6);
    if (captureUnchecked(sourceV4) !=
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

} // namespace mm6
