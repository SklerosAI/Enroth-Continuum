#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Components/Random/EngineRandomComponent.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Overlays.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6ActiveActorOutdoorDeltaCapture.h"
#include "Engine/MM6/MM6ActiveActorSection.h"
#include "Engine/MM6/MM6BuffSaveFields.h"
#include "Engine/MM6/MM6CoreSaveRuntime.h"
#include "Engine/MM6/MM6DurableSaveFields.h"
#include "Engine/MM6/MM6GameplayRngSaveFields.h"
#include "Engine/MM6/MM6GeneratedNpcSaveFields.h"
#include "Engine/MM6/MM6HirelingSaveRuntime.h"
#include "Engine/MM6/MM6LoadedMapSource.h"
#include "Engine/MM6/MM6NpcStaticBaseline.h"
#include "Engine/MM6/MM6NpcSaveFields.h"
#include "Engine/MM6/MM6PersistentEventPublication.h"
#include "Engine/MM6/MM6PostWorldAdmissionForTest.h"
#include "Engine/MM6/MM6PostWorldOwnerValidator.h"
#include "Engine/MM6/MM6RawChildPublicationForTest.h"
#include "Engine/MM6/MM6RawCursorImagePlan.h"
#include "Engine/MM6/MM6RawCursorShopProcessOwner.h"
#include "Engine/MM6/MM6RawPartyInventoryStage.h"
#include "Engine/MM6/MM6RawSessionStage.h"
#include "Engine/MM6/MM6RawV2Admission.h"
#include "Engine/MM6/MM6RecoverySaveRuntime.h"
#include "Engine/MM6/MM6ResistanceSaveFields.h"
#include "Engine/MM6/MM6ScopedNewGameSave.h"
#include "Engine/MM6/MM6V3FullSessionPreflight.h"
#include "Engine/MM6/MM6V3SaveFields.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Party.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"
#include "Engine/Tables/OverlayTable.h"
#include "Engine/Tables/TileTable.h"
#include "Engine/Timer.h"
#include "Library/Platform/Application/PlatformApplication.h"
#include "GUI/GUIMessageQueue.h"
#include "Io/Mouse.h"

namespace {

std::filesystem::path disposableProfile() {
    const char *value = std::getenv("ENROTH_MM6_RESTART_PROFILE");
    if (!value || !*value)
        return {};
    const std::filesystem::path supplied(value);
    if (!supplied.is_absolute() || !std::filesystem::is_directory(supplied))
        throw std::runtime_error("MM6 v2 world probe needs an absolute disposable profile");
    const std::filesystem::path root = std::filesystem::canonical(supplied);
    if (root.parent_path() != std::filesystem::canonical(std::filesystem::temp_directory_path()) ||
        !root.filename().string().starts_with("mm6-restart-") ||
        (std::filesystem::status(root).permissions() &
         (std::filesystem::perms::group_all | std::filesystem::perms::others_all)) !=
            std::filesystem::perms::none)
        throw std::runtime_error("MM6 v2 world probe needs an owner-only system-temp profile");
    return root;
}

std::vector<std::uint8_t> slotBytes(const std::filesystem::path &path) {
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
    // The physical SHOP/SCLK title owner is captured separately. The older
    // MM7-indexed Party arrays cannot be silently interpreted as MM6 houses.
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

// Candidate-only probe in the disposable v2 restart child. It captures the
// real post-entry first map and runs the complete detached v3 semantic stage.
// No v3 file is written and production SaveLoad remains guarded.
GAME_TEST(MM6V3PostEntryCandidateHeadless, DISABLED_FirstMapFullSessionPreflight) {
    const std::filesystem::path profile = disposableProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to an owner-only v2 profile";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const auto before = slotBytes(profile / "saves" / "autosave.ec6");
    ASSERT_FALSE(before.empty());

    bool entered = false;
    bool preflightPassed = false;
    std::string refusalClass = "not-entered";
    std::string stage = "capture";
    std::unique_ptr<mm6::StagedRawSessionFields> staged;
    std::unique_ptr<mm6::ScopedChestMapLoadOptIn> chestOptIn;
    std::unique_ptr<mm6::testing::ScopedPostWorldAdmissionForTest> admission;
    game.runGameRoutine([&] {
        mm6::testing::setLoadGameOverride([&](std::string_view slotName) {
            if (slotName != mm6::NEW_GAME_AUTOSAVE_SLOT)
                throw std::runtime_error("MM6 v3 probe selected another slot");
            const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
            const mm6::NpcStaticBaseline freshNpcs =
                mm6::loadNpcStaticBaseline(*engine->resources(), GameTitle::MM6);
            const auto verifyItem = [](std::int32_t id) {
                return id >= 1 && id <= 580 &&
                    !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
            };
            auto candidate = mm6::preflightStagedRawSessionFields(
                profile, slotName, freshNpcs, maps.expectedNames(),
                [&maps](std::string_view name) { return maps.readBase(name); },
                verifyItem, *pItemTable, pOverlayTable->overlays.size(),
                pTileTable->tileCount(), GameTitle::MM6);
            if (!candidate || candidate->needsRecovery || !candidate->outdoorPlan ||
                candidate->indoorPlan || candidate->outdoorPlan->mapName != "oute3.odm" ||
                mm6::encodeRawSave(candidate->raw.saved, GameTitle::MM6) != before)
                throw std::runtime_error("MM6 v3 probe requires the exact first-map v2 primary");
            staged = std::make_unique<mm6::StagedRawSessionFields>(std::move(*candidate));
            *pParty = staged->party.party;
            *pNPCStats = staged->world.world.npcs;
            *gameTimer = staged->world.visuals.timer;
            *pActiveOverlayList = staged->world.visuals.overlays;
            pMapDeltas = std::move(staged->world.world.mapDeltas);
            mm6::chestTitleState() = staged->world.chestTitle;
            mm6::publishFreshChildPersistentEventZeros(
                &engine->_persistentVariables, staged->persistentEvents, GameTitle::MM6);
            mm6::publishRawCursorShopOwners(
                &mm6::rawHeldCursorState(GameTitle::MM6), &mm6::shopTitleState(),
                staged->raw.saved,
                {std::move(staged->raw.cursor), std::move(staged->raw.shops)}, GameTitle::MM6);
            engine->_pendingTransition = MapDestination(
                pMapTable->GetMapInfo("oute3.odm"), MAP_START_POINT_PARTY);
            engineFlags |= ENGINE_LOADING_SAVEGAME | ENGINE_SKIP_NEXT_WORLD_UPDATE;
            chestOptIn = std::make_unique<mm6::ScopedChestMapLoadOptIn>();
            admission = std::make_unique<mm6::testing::ScopedPostWorldAdmissionForTest>(
                GameTitle::MM6, [&] {
                    entered = true;
                    try {
                        // Reopen read-only sources inside this later callback;
                        // the load override's local readers have expired.
                        const mm6::LoadedMapSource maps(*pGames_LOD, *pMapTable, GameTitle::MM6);
                        const mm6::NpcStaticBaseline freshNpcs =
                            mm6::loadNpcStaticBaseline(*engine->resources(), GameTitle::MM6);
                        const auto verifyItem = [](std::int32_t id) {
                            return id >= 1 && id <= 580 &&
                                !pItemTable->items[static_cast<ItemId>(id)].iconName.empty();
                        };
                        mm6::requireRawV2RepresentableState(*pParty, {}, GameTitle::MM6);
                        requireEmptyInheritedCommerce(*pParty);
                        (void) mm6::stageRawCursorImagePlan(
                            *pParty, mm6::rawHeldCursorState(GameTitle::MM6),
                            *pItemTable, GameTitle::MM6);
                        if (uCurrentlyLoadedLevelType != LEVEL_OUTDOOR ||
                            engine->_currentLoadedMapId != pMapTable->GetMapInfo("oute3.odm") ||
                            pMapDeltas.size() != maps.expectedNames().size())
                            throw std::runtime_error("map identity");
                        mm6::V3SaveFields fields;
                        fields.raw = staged->raw.saved;
                        fields.raw.core = mm6::captureCoreFields(*pParty, GameTitle::MM6);
                        const auto durable = mm6::captureDurableFields(*pParty, GameTitle::MM6);
                        fields.raw.bits = durable.bits;
                        fields.raw.effects = durable.effects;
                        fields.raw.inventory = mm6::captureRawPartyInventories(
                            {*pParty, fields.raw.inventory}, *pItemTable, GameTitle::MM6);
                        fields.raw.metadata = mm6::captureMetadata(
                            fields.raw.metadata.header.displayName, GameTitle::MM6);
                        fields.raw.npcs = mm6::captureNpcFields(*pNPCStats, GameTitle::MM6);
                        fields.raw.resistances = mm6::captureResistanceFields(*pParty, GameTitle::MM6);
                        fields.raw.buffs = mm6::captureBuffFields(*pParty, GameTitle::MM6);
                        fields.raw.hirelings = mm6::captureHirelingFields(*pParty, GameTitle::MM6);
                        fields.raw.recovery = mm6::captureRecoveryFields(*pParty, GameTitle::MM6);
                        const auto held = mm6::captureRawHeldCursor(
                            mm6::rawHeldCursorState(GameTitle::MM6), GameTitle::MM6);
                        if (!held)
                            throw std::runtime_error("cursor owner");
                        fields.raw.heldItem = *held;
                        fields.raw.shops = mm6::shopTitleState();
                        const Blob delta = mm6::captureOutdoorDeltaWithActiveActors(
                            *pOutdoor, GameTitle::MM6);
                        const std::string activeName = "oute3.ddm";
                        const auto *deltaBegin = static_cast<const std::uint8_t *>(delta.data());
                        const std::vector<std::uint8_t> current(deltaBegin, deltaBegin + delta.size());
                        fields.raw.chest = mm6::captureChestSaveFields(
                            mm6::chestTitleState(), activeName, current, GameTitle::MM6);
                        for (auto &entry : fields.raw.maps.deltas) {
                            const auto it = pMapDeltas.find(entry.name);
                            if (it == pMapDeltas.end())
                                throw std::runtime_error("map cache");
                            const auto *begin = static_cast<const std::uint8_t *>(it->second.data());
                            const std::vector<std::uint8_t> cached(begin, begin + it->second.size());
                            if (entry.name == activeName)
                                entry.bytes = current;
                            else if (entry.bytes != cached)
                                throw std::runtime_error("nonactive map cache changed");
                        }
                        const std::vector<Actor> actors(pActors.begin(), pActors.end());
                        fields.activeActorPayload = mm6::encodeActiveActorSection(
                            actors, GameTitle::MM6);
                        fields.generatedNpcs = mm6::captureGeneratedNpcFields(
                            pActors, *pNPCStats, "oute3.odm", GameTitle::MM6);
                        const auto random = application->component<EngineRandomComponent>();
                        fields.gameplayRng = mm6::captureGameplayRngFields(*random, GameTitle::MM6);
                        stage = "encode";
                        const auto bytes = mm6::encodeV3Save(fields, GameTitle::MM6);
                        stage = "full-session-preflight";
                        const auto preflight = mm6::stageV3FullSessionPreflight(
                            bytes, freshNpcs, maps.expectedNames(),
                            [&maps](std::string_view name) { return maps.readBase(name); },
                            verifyItem, *pItemTable, pOverlayTable->overlays.size(),
                            pTileTable->tileCount(), pActors.size(), random->type(),
                            GameTitle::MM6);
                        stage = "persistent-event-join";
                        mm6::requirePostMapPersistentEvents(
                            preflight.represented.persistentEvents, pOutdoor->mm6DeltaBytes,
                            engine->_persistentVariables, GameTitle::MM6);
                        preflightPassed = true;
                        refusalClass = "pass";
                    } catch (const std::exception &error) {
                        const std::string_view message = error.what();
                        if (stage == "persistent-event-join")
                            refusalClass = "persistent-event-join";
                        else if (message.find("generated NPC") != std::string_view::npos ||
                            message.find("GNPC") != std::string_view::npos)
                            refusalClass = "generated-npc";
                        else if (message.find("inventory") != std::string_view::npos ||
                                 message.find("INVR") != std::string_view::npos)
                            refusalClass = "inventory";
                        else if (message.find("ground item") != std::string_view::npos)
                            refusalClass = "ground-item";
                        else if (message.find("commerce") != std::string_view::npos)
                            refusalClass = "commerce";
                        else if (message.find("map identity") != std::string_view::npos)
                            refusalClass = "map-identity";
                        else if (message.find("nonactive map cache changed") != std::string_view::npos)
                            refusalClass = "nonactive-cache";
                        else if (message.find("map cache") != std::string_view::npos)
                            refusalClass = "map-cache-missing";
                        else if (message.find("active DDM") != std::string_view::npos ||
                                 message.find("DDM") != std::string_view::npos)
                            refusalClass = "active-ddm";
                        else if (message.find("actor map") != std::string_view::npos)
                            refusalClass = "actor-map";
                        else if (message.find("represented world differs") != std::string_view::npos)
                            refusalClass = "represented-map-join";
                        else if (message.find("invalid item template") != std::string_view::npos)
                            refusalClass = "map-item-template";
                        else if (message.find("missing a base map") != std::string_view::npos)
                            refusalClass = "map-base";
                        else if (message.find("record shape") != std::string_view::npos)
                            refusalClass = "map-record-shape";
                        else if (message.find("saved delta") != std::string_view::npos)
                            refusalClass = "map-delta-set";
                        else if (message.find("MAPS") != std::string_view::npos)
                            refusalClass = "maps-section";
                        else if (message.find("map") != std::string_view::npos)
                            refusalClass = "map-preflight";
                        else if (message.find("Party") != std::string_view::npos ||
                                 message.find("core") != std::string_view::npos)
                            refusalClass = "party";
                        else
                            refusalClass = "other-refusal";
                    }
                }, mm6::testing::PostWorldAdmissionResult::AbortToMainMenu);
        });
        mm6::testing::setMainMenuAutosaveQuickLoadOverride(true);
        engine->_messageQueue->addMessageCurrentFrame(UIMSG_QuickLoad);
    });
    game.skipLoadingScreen();
    game.tick(2);
    game.runGameRoutine([&] {
        admission.reset();
        chestOptIn.reset();
        mm6::testing::setMainMenuAutosaveQuickLoadOverride(false);
        mm6::testing::setLoadGameOverride({});
    });
    EXPECT_TRUE(entered);
    EXPECT_TRUE(preflightPassed) << refusalClass;
    EXPECT_EQ(slotBytes(profile / "saves" / "autosave.ec6"), before);
    EXPECT_EQ(window->nativeHandle(), nullptr);
}

