#include "MM6V4AfterActionCaptureForTest.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Components/Random/EngineRandomComponent.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Overlays.h"
#include "Engine/MM6/MM6ActiveActorOutdoorDeltaCapture.h"
#include "Engine/MM6/MM6ActiveActorSection.h"
#include "Engine/MM6/MM6AtomicSaveFile.h"
#include "Engine/MM6/MM6BuffSaveFields.h"
#include "Engine/MM6/MM6CoreSaveRuntime.h"
#include "Engine/MM6/MM6DurableSaveFields.h"
#include "Engine/MM6/MM6EventRuntimeSaveFields.h"
#include "Engine/MM6/MM6GameplayRngSaveFields.h"
#include "Engine/MM6/MM6GeneratedNpcSaveFields.h"
#include "Engine/MM6/MM6HirelingSaveRuntime.h"
#include "Engine/MM6/MM6LoadedMapSource.h"
#include "Engine/MM6/MM6NpcStaticBaseline.h"
#include "Engine/MM6/MM6NpcSaveFields.h"
#include "Engine/MM6/MM6OutdoorWire.h"
#include "Engine/MM6/MM6PersistentEventPublication.h"
#include "Engine/MM6/MM6RawCursorImagePlan.h"
#include "Engine/MM6/MM6RawCursorShopProcessOwner.h"
#include "Engine/MM6/MM6RawPartyInventoryStage.h"
#include "Engine/MM6/MM6RawV2Admission.h"
#include "Engine/MM6/MM6RecoverySaveRuntime.h"
#include "Engine/MM6/MM6ResistanceSaveFields.h"
#include "Engine/MM6/MM6V4FullSessionPreflight.h"
#include "Engine/MM6/MM6V4OutdoorDecorationReplay.h"
#include "Engine/MM6/MM6V4SaveFields.h"
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

namespace mm6::testing {
namespace {

std::vector<std::uint8_t> readFile(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

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
            throw std::runtime_error("unrepresented inherited commerce");
        for (const Item &item : party.standartItemsInShops[id])
            if (hasItemPayload(item))
                throw std::runtime_error("unrepresented inherited commerce");
        for (const Item &item : party.specialItemsInShops[id])
            if (hasItemPayload(item))
                throw std::runtime_error("unrepresented inherited commerce");
    }
    for (HouseId id : party.spellBooksInGuilds.indices()) {
        if (party.PartyTimes.guildNextRefreshTime[id].ticks())
            throw std::runtime_error("unrepresented inherited commerce");
        for (const Item &item : party.spellBooksInGuilds[id])
            if (hasItemPayload(item))
                throw std::runtime_error("unrepresented inherited commerce");
    }
    for (HouseId id : party.PartyTimes.bountyHuntNextGenTime.indices())
        if (party.PartyTimes.bountyHuntNextGenTime[id].ticks() ||
            party.monster_id_for_hunting[id] != MONSTER_INVALID ||
            party.monster_for_hunting_killed[id])
            throw std::runtime_error("unrepresented inherited commerce");
}

} // namespace

void writeV4AfterActionCandidateForTest(
    const std::filesystem::path &profile,
    std::span<const std::uint8_t> sourceCandidate) {
    if (engine->gameTitle() != GameTitle::MM6 ||
        uCurrentlyLoadedLevelType != LEVEL_OUTDOOR ||
        engine->_currentLoadedMapId != pMapTable->GetMapInfo("oute3.odm") ||
        engine->_pendingTransition || pPartyActionQueue->uNumActions != 0 ||
        gameTimer->isPaused())
        throw std::runtime_error("MM6 v4 after-action capture lacks stable first-map input");
    const auto seedBefore = readFile(profile / "saves/autosave.ec6");
    const auto sourceBefore = readFile(profile / "saves/candidate-v4.ec6");
    if (seedBefore.empty() || !std::ranges::equal(sourceBefore, sourceCandidate))
        throw std::runtime_error("MM6 v4 after-action capture lacks original disposable slots");

    V4SaveFields saved = decodeV4Save(sourceCandidate, GameTitle::MM6);
    if (saved.v3.raw.metadata.header.mapFileName != "oute3.odm" ||
        saved.eventRuntime.maps.size() != 1 ||
        saved.eventRuntime.maps[0].mapName != "oute3.odm")
        throw std::runtime_error("MM6 v4 after-action capture lacks first-map source");
    requireRawV2RepresentableState(*pParty, {}, GameTitle::MM6);
    requireEmptyInheritedCommerce(*pParty);
    (void) stageRawCursorImagePlan(*pParty, rawHeldCursorState(GameTitle::MM6),
                                   *pItemTable, GameTitle::MM6);
    const LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
    const NpcStaticBaseline freshNpcs =
        loadNpcStaticBaseline(*engine->resources(), GameTitle::MM6);
    const auto verifyItem = [](std::int32_t id) {
        return id >= 1 && id <= 580 &&
            !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
    };
    if (pMapDeltas.size() != maps.expectedNames().size())
        throw std::runtime_error("MM6 v4 after-action map cache shape changed");

    V3SaveFields &fields = saved.v3;
    const auto oldPosition = fields.raw.core.party.position;
    const auto oldYaw = fields.raw.core.party.viewYaw;
    fields.raw.core = captureCoreFields(*pParty, GameTitle::MM6);
    if (fields.raw.core.party.position == oldPosition ||
        fields.raw.core.party.viewYaw == oldYaw)
        throw std::runtime_error("MM6 v4 after-action movement was not captured");
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
        throw std::runtime_error("MM6 v4 after-action cursor owner changed");
    fields.raw.heldItem = *held;
    fields.raw.shops = shopTitleState();
    const Blob delta = captureOutdoorDeltaWithActiveActors(*pOutdoor, GameTitle::MM6);
    const auto *deltaBegin = static_cast<const std::uint8_t *>(delta.data());
    const std::vector<std::uint8_t> current(deltaBegin, deltaBegin + delta.size());
    const std::string activeName = "oute3.ddm";
    fields.raw.chest = captureChestSaveFields(chestTitleState(), activeName,
                                              current, GameTitle::MM6);
    for (auto &entry : fields.raw.maps.deltas) {
        const auto it = pMapDeltas.find(entry.name);
        if (it == pMapDeltas.end())
            throw std::runtime_error("MM6 v4 after-action map cache missing");
        const auto *begin = static_cast<const std::uint8_t *>(it->second.data());
        const std::vector<std::uint8_t> cached(begin, begin + it->second.size());
        if (entry.name == activeName)
            entry.bytes = current;
        else if (entry.bytes != cached)
            throw std::runtime_error("MM6 v4 after-action nonactive map cache changed");
    }
    const std::vector<Actor> actors(pActors.begin(), pActors.end());
    fields.activeActorPayload = encodeActiveActorSection(actors, GameTitle::MM6);
    fields.generatedNpcs = captureGeneratedNpcFields(
        pActors, *pNPCStats, "oute3.odm", GameTitle::MM6);
    const auto random = application->component<EngineRandomComponent>();
    fields.gameplayRng = captureGameplayRngFields(*random, GameTitle::MM6);
    auto &event = saved.eventRuntime.maps[0];
    event.respawnCount = pOutdoor->ddm.respawnCount;
    event.lastRespawnDay = pOutdoor->ddm.lastRespawnDay;
    event.mapVars = engine->_persistentVariables.mapVars;
    event.decorVars = engine->_persistentVariables.decorVars;
    event.bindings = captureEventRuntimeDecorationBindings(pLevelDecorations,
                                                            GameTitle::MM6);
    const auto bytes = encodeV4Save(saved, GameTitle::MM6);

    const auto validate = [&](std::span<const std::uint8_t> proposed) {
        const auto checked = std::unique_ptr<V4FullSessionPreflight>(
            new V4FullSessionPreflight(stageV4FullSessionPreflight(
                proposed, freshNpcs, maps.expectedNames(),
                [&maps](std::string_view name) { return maps.readBase(name); },
                verifyItem, *pItemTable, pOverlayTable->overlays.size(),
                pTileTable->tileCount(), pActors.size(), random->type(),
                pLevelDecorations, GameTitle::MM6)));
        const auto expectedEvents = capturePersistentEventBoundary(
            checked->v3.represented.persistentEvents, GameTitle::MM6);
        const auto liveWire = decodeOutdoorDelta(pOutdoor->mm6DeltaBytes);
        if (!std::ranges::equal(expectedEvents,
                                liveWire.part(OutdoorDeltaPart::PersistentBytes)) ||
            checked->activeEventRuntime.mapVars != engine->_persistentVariables.mapVars ||
            checked->activeEventRuntime.decorVars != engine->_persistentVariables.decorVars)
            throw std::runtime_error("MM6 v4 after-action event owner changed");
        requireFirstMapOutdoorDecorationReplay(
            "oute3.odm", maps.readBase("oute3.odm"),
            checked->activeEventRuntime, *pDecorationTable,
            pLevelDecorations, GameTitle::MM6);
    };
    validate(bytes);
    writeAtomicV4SaveFileForTest(
        profile, "candidate-v4-after-action.ec6", bytes, validate);
    if (readFile(profile / "saves/candidate-v4-after-action.ec6") != bytes ||
        readFile(profile / "saves/candidate-v4.ec6") != sourceBefore ||
        readFile(profile / "saves/autosave.ec6") != seedBefore)
        throw std::runtime_error("MM6 v4 after-action atomic write changed another slot");
}

} // namespace mm6::testing
