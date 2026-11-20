#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtInterpreter.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Localization.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6FirstMapManualSave.h"
#include "Engine/MM6/MM6ChestTitleState.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/CombinedSkillValue.h"
#include "Engine/Objects/SpriteEnumFunctions.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Party.h"
#include "Engine/PartyPlacement.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Snapshots/CompositeSnapshots.h"
#include "Engine/Tables/ChestTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Timer.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIChest.h"
#include "GUI/UI/UIRest.h"
#include "GUI/UI/UISaveLoad.h"

#include "Utility/Memory/Blob.h"

#include "MM6CampaignChain.h"

static std::filesystem::path ownerOnlyProfile() {
    const char *value = std::getenv("ENROTH_MM6_RESTART_PROFILE");
    if (!value || !*value)
        return {};
    const std::filesystem::path root = std::filesystem::canonical(std::filesystem::path(value));
    if (root.parent_path() != std::filesystem::canonical(std::filesystem::temp_directory_path()) ||
        !root.filename().string().starts_with("mm6-restart-") ||
        (std::filesystem::status(root).permissions() &
         (std::filesystem::perms::group_all | std::filesystem::perms::others_all)) != std::filesystem::perms::none)
        throw std::runtime_error("MM6 chest save test needs an owner-only system-temp profile");
    return root;
}

static bool enabled(std::string_view value) {
    const char *flag = std::getenv("ENROTH_MM6_SAVE_ANYWHERE_ENABLE");
    return flag && std::string_view(flag) == value;
}

static std::string fileText(const std::filesystem::path &path) {
    std::ifstream input(path);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

static std::string chestPagesText(int chestId) {
    // Every item on each of the chest's pages, with its top left cell.
    std::string result;
    for (InventoryEntry entry : vChests[chestId].inventory.entries())
        if (entry.zone() == INVENTORY_ZONE_GRID)
            result += fmt::format(" {}@{},{}", std::to_underlying(entry->itemId), entry.geometry().x, entry.geometry().y);
    for (const mm6::ChestPageItem &item : vChests[chestId].mm6Pages)
        result += fmt::format(" p{}:{}@{},{}", item.page + 1, std::to_underlying(vChests[chestId].inventory.entry(item.slot)->itemId),
                              item.cell % 9, item.cell / 9);
    return result;
}

static void startNewGameThroughInput(EngineController &game) {
    // Each character takes its first three optional skills and up to three points in every attribute, then the party
    // enters the first map.
    constexpr PlatformKey characterKeys[] = {PlatformKey::KEY_DIGIT_1, PlatformKey::KEY_DIGIT_2, PlatformKey::KEY_DIGIT_3,
                                             PlatformKey::KEY_DIGIT_4};
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    for (PlatformKey key : characterKeys) {
        game.pressAndReleaseKey(key);
        game.tick();
        for (int skill = 0; skill < 3; skill++) {
            game.pressGuiButton(fmt::format("PartyCreation_Skill{}", skill));
            game.tick();
        }
        for (int attribute = 0; attribute < 7; attribute++) {
            for (int point = 0; point < 3; point++) {
                game.pressAndReleaseKey(PlatformKey::KEY_ADD);
                game.tick();
            }
            game.pressAndReleaseKey(PlatformKey::KEY_DOWN);
            game.tick();
        }
    }
    game.pressGuiButton("PartyCreation_OK");
    game.skipLoadingScreen();
    game.tick();
}

static void walkOverEdge(EngineController &game, MapId map, Vec3f pos, int yaw) {
    game.teleportTo(map, pos, yaw); // Next to the edge, facing it.
    game.pressKey(PlatformKey::KEY_UP);
    game.tick(2); // The party walks off the edge and the travel dialogue opens.
    game.releaseKey(PlatformKey::KEY_UP);
    game.pressGuiButton("Transition_Yes");
    game.tick();
    game.skipLoadingScreen();
    game.tick();
}

static void enterOutd3(EngineController &game) {
    startNewGameThroughInput(game);
    walkOverEdge(game, pMapTable->GetMapInfo("oute3.odm"), Vec3f(-22450, -10000, 100), 180); // The west edge.
}

static void clickOutd3Chest4(EngineController &game) {
    // Every chest on outd3 is trapped. The party clicks the near side of the crate that holds chest 4.
    game.teleportTo(pMapTable->GetMapInfo("outd3.odm"), Vec3f(21632, -17200, 0), 90); // South of the crate, facing it.
    game.pointMouseAtFace(2308);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
}

static std::vector<int> chestContents(int chestId) {
    // The chest's flags, then every item with its grid cell.
    std::vector<int> result = {std::to_underlying(vChests[chestId].flags)};
    for (InventoryEntry entry : vChests[chestId].inventory.entries())
        result.insert(result.end(), {std::to_underlying(entry->itemId), entry.geometry().x, entry.geometry().y});
    return result;
}

static void walkIntoAbandonedTemple(EngineController &game, std::uint32_t chestRoll) {
    // The party walks into the Abandoned Temple, whose chests roll their items as it loads, from the given chest random
    // state. Chest 1 is trapped, the active character disarms it.
    pParty->activeCharacter().setSkillValue(SKILL_TRAP_DISARM, CombinedSkillValue(10, MASTERY_MASTER));
    game.teleportTo(pMapTable->GetMapInfo("oute3.odm"), Vec3f(-21456, -286, 193), 90); // In front of the temple door, facing it.
    mm6::chestTitleState().lcgState = chestRoll;
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    game.pressGuiButton("Transition_Yes");
    game.tick();
    game.skipLoadingScreen();
    game.tick();
}

static void enterAbandonedTempleWithChestRoll(EngineController &game, std::uint32_t chestRoll) {
    startNewGameThroughInput(game);
    walkIntoAbandonedTemple(game, chestRoll);
}

static void hireNoreenTheBanker(EngineController &game) {
    // The party stands next to Noreen, facing east towards her, clicks on her and hires her.
    game.teleportTo(engine->_currentLoadedMapId, pActors[15].pos - Vec3f(150, 0, 0), 0);
    game.pointMouseAtActor(15);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
    for (std::string_view label : {"Join", "Hire"}) {
        const auto option = std::ranges::find_if(pDialogueWindow->vButtons, [&](GUIButton *button) {
            return button->id.starts_with("NpcDialogue_Option") && button->label == label;
        });
        ASSERT_NE(option, pDialogueWindow->vButtons.end()) << "No conversation topic " << label;
        game.pressAndReleaseButton(BUTTON_LEFT, (*option)->rect.center());
        game.tick();
    }
}

static int chestGoldPiles(int chestId) {
    return std::ranges::count_if(vChests[chestId].inventory.entries(), [](InventoryEntry entry) { return entry->isGold(); });
}

static std::vector<int> hiddenChestItems(int chestId) {
    // Items the chest holds without a cell in its grid.
    std::vector<int> result;
    for (InventoryEntry entry : vChests[chestId].inventory.entries())
        if (entry.zone() == INVENTORY_ZONE_STASH)
            result.push_back(std::to_underlying(entry->itemId));
    return result;
}

static GUIButton *chestPageButton() {
    if (!pGUIWindow_CurrentChest)
        return nullptr;
    const auto button = std::ranges::find(pGUIWindow_CurrentChest->vButtons, "Chest_NextPage", &GUIButton::id);
    return button == pGUIWindow_CurrentChest->vButtons.end() ? nullptr : *button;
}

static std::vector<std::pair<ItemId, Pointi>> chestPages(int chestId) {
    // Items on the chest's later pages, with their top left cells.
    std::vector<std::pair<ItemId, Pointi>> result;
    for (const mm6::ChestPageItem &item : vChests[chestId].mm6Pages)
        result.push_back({vChests[chestId].inventory.entry(item.slot)->itemId, Pointi(item.cell % 9, item.cell / 9)});
    return result;
}

static constexpr std::uint32_t secondPageRoll = 9; // Chest luck that puts two items on the second page of the Abandoned Temple's chest 1.

static int drawnChestItems(const AccessibleVector<DrawnQuad> &quads, int chestId) {
    // Icons of the chest's items on the chest screen.
    std::set<std::string> icons;
    for (InventoryEntry entry : vChests[chestId].inventory.entries())
        icons.insert(entry->GetIconName());
    return quads.count([&](const DrawnQuad &quad) { return icons.contains(quad.texture); });
}

static int trapSprites() {
    return std::ranges::count_if(pSpriteObjects, [](const SpriteObject &object) {
        return object.uObjectDescID && object.spriteId >= SPRITE_TRAP_FIRE && object.spriteId <= SPRITE_TRAP_BODY;
    });
}

static std::vector<int> chestArtifacts(int chestId) {
    std::vector<int> result;
    for (InventoryEntry entry : vChests[chestId].inventory.entries())
        if (std::to_underlying(entry->itemId) >= 400 && std::to_underlying(entry->itemId) <= 429)
            result.push_back(std::to_underlying(entry->itemId));
    return result;
}

static void restDays(EngineController &game, int days) {
    // Rests and heals once a day in the rest screen, then waits there for the next dawn, that many days.
    for (int day = 0; day < days; day++) {
        for (std::string_view button : {"Rest_RestAndHeal", "Rest_WaitTillDawn"}) {
            game.pressAndReleaseKey(PlatformKey::KEY_R);
            game.tick();
            game.pressGuiButton(button);
            game.tick();
            while (currentRestType != REST_NONE)
                game.tick(); // The hours pass.
            if (current_screen_type == SCREEN_REST) {
                game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
                game.tick();
            }
        }
    }
}

static constexpr std::uint32_t hiveArtifactLuck = 4; // Chest luck that rolls artifact 415 into the hive's chest 4, and no other artifact.

static void reopenHiveChest4AfterRespawn(EngineController &game) {
    // The party leaves the hive, rests a week in the Oracle for the hive to refill, comes back with the same chest luck and
    // opens chest 4.
    game.teleportTo(pMapTable->GetMapInfo("oracle.blv"), Vec3f(-1408, 900, -512), 90); // No random encounters here.
    pParty->SetFood(14); // Seven rests at two food each, a new party has seven.
    restDays(game, 7);
    mm6::chestTitleState().lcgState = hiveArtifactLuck;
    game.teleportTo(pMapTable->GetMapInfo("hive.blv"), Vec3f(-3000, 4182, -1777), 0, -20); // West of chest 4, looking down at it.
    game.pointMouseAtFace(2960);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
}

static EvtInstruction parsedMM6Instruction(EvtOpcode opcode, std::string payload) {
    std::string record;
    record.push_back(static_cast<char>(4 + payload.size()));
    record.push_back(1); // Event ID, little endian.
    record.push_back(0);
    record.push_back(0); // Step.
    record.push_back(static_cast<char>(opcode));
    record += payload;
    return EvtProgram::load(Blob::fromString(record), GameTitle::MM6).instruction(1, 0);
}

// This is intentionally separate from GameTests_MM6.cpp, which owns the
// broader first-map preflight. Run only with the MM6 headless GameTest profile.
GAME_TEST(MM6ZChestHeadless, ScopedFirstOutdoorMapAndRollback) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    ASSERT_FALSE(mm6::chestMapLoadOptedIn());

    const MapId firstMap = pMapTable->GetMapInfo("oute3.odm");
    ASSERT_NE(firstMap, MAP_INVALID);

    // The headless profile has already mounted its private Games archive.
    // Seed only decoded archive defaults into the test's in-memory map owner.
    // No game installation or save path is written by this test.
    pMapDeltas.clear();
    for (const std::string &name : pGames_LOD->ls()) {
        if (std::string_view(name).ends_with(".dlv") ||
            std::string_view(name).ends_with(".ddm"))
            pMapDeltas.emplace(name, Blob::copy(pGames_LOD->read(name)));
    }
    ASSERT_EQ(pMapDeltas.size(), 67u);
    ASSERT_TRUE(pMapDeltas.contains("oute3.ddm"));

    mm6::resetChestTitleState(&mm6::chestTitleState());
    ASSERT_TRUE(mm6::restoreChestTitleState(GameTitle::MM6, &mm6::chestTitleState(),
                                            0x12345678u, {}));

    // Exercise actual first-map archive/delta reconstruction and the live
    // chest route. The outer outdoor load also spawns encounters; that path
    // has an independent MM6 monster-table blocker.
    bool committed = false;
    game.runGameRoutine([firstMap, &committed] {
        pParty->Reset();
        engine->_currentLoadedMapId = firstMap;
        uCurrentlyLoadedLevelType = LEVEL_OUTDOOR;
        bool respawned = false;
        pOutdoor->Load("oute3.odm", 1, 0, &respawned);
        mm6::ScopedChestMapLoadOptIn optIn;
        committed = mm6::tryActiveChestMapLoad(GameTitle::MM6, firstMap, pMapTable, pItemTable, pParty);
    });
    ASSERT_TRUE(committed);
    ASSERT_FALSE(mm6::chestMapLoadOptedIn());
    ASSERT_EQ(engine->_currentLoadedMapId, firstMap);
    ASSERT_EQ(uCurrentlyLoadedLevelType, LEVEL_OUTDOOR);
    ASSERT_TRUE(mm6::chestTitleState().ready);
    ASSERT_EQ(vChests.size(), 20u);

    const auto wire = mm6::decodeOutdoorDelta(pOutdoor->mm6DeltaBytes);
    ASSERT_EQ(wire.chestCount, 20u);
    mm6::ChestRecordSet records{};
    ASSERT_TRUE(mm6::copyChestRecords(wire.part(mm6::OutdoorDeltaPart::Chests), &records));
    for (int index = 0; index < 20; ++index) {
        Chest_MM7 live{};
        snapshot(vChests[index], &live);
        EXPECT_EQ(live.chestTypeId, mm6::readChest16(records[index], 0));
        EXPECT_EQ(live.flags, mm6::readChest16(records[index], 2));
        for (int slot = 0; slot < 140; ++slot)
            EXPECT_EQ(std::memcmp(&live.items[slot], records[index].data() + 4 + slot * 28, 24), 0)
                << "chest " << index << ", slot " << slot;
        const auto grid = mm6::decodeChestGridWire(GameTitle::MM6, records[index]);
        ASSERT_TRUE(grid);
        EXPECT_EQ(live.inventoryMatrix, *grid);
    }

    // A late malformed sidecar must not publish another chest transaction.
    // Restore the valid sidecar before returning to the shared fixture.
    const auto savedSidecar = pOutdoor->mm6DeltaBytes;
    const auto savedState = mm6::chestTitleState();
    const Chest *savedChests = vChests.data();
    pOutdoor->mm6DeltaBytes.pop_back();
    const auto malformed = pOutdoor->mm6DeltaBytes;
    bool accepted = false;
    {
        mm6::ScopedChestMapLoadOptIn optIn;
        accepted = mm6::tryActiveChestMapLoad(GameTitle::MM6, firstMap, pMapTable, pItemTable, pParty);
    }
    const bool rawUntouched = pOutdoor->mm6DeltaBytes == malformed;
    pOutdoor->mm6DeltaBytes = savedSidecar;
    EXPECT_FALSE(accepted);
    EXPECT_TRUE(rawUntouched);
    EXPECT_EQ(vChests.data(), savedChests);
    EXPECT_EQ(mm6::chestTitleState().lcgState, savedState.lcgState);
    EXPECT_EQ(mm6::chestTitleState().generatedArtifactFlags, savedState.generatedArtifactFlags);
    EXPECT_EQ(window->nativeHandle(), nullptr);

    // Open one real untrapped first-map chest through the public entry point.
    // This must publish the MM6 raw sidecar and live chest together, without
    // entering the inherited MM7 placement path.
    int openIndex = -1;
    for (int index = 0; index < static_cast<int>(vChests.size()); ++index) {
        if (!vChests[index].Trapped() && !vChests[index].Initialized()) {
            openIndex = index;
            break;
        }
    }
    ASSERT_GE(openIndex, 0);
    const auto beforeOpenSidecar = pOutdoor->mm6DeltaBytes;
    bool opened = false;
    game.runGameRoutine([openIndex, &opened] {
        pParty->setActiveCharacterIndex(0);
        opened = Chest::open(openIndex, Pid::dummy());
        if (opened && pGUIWindow_CurrentChest)
            pGUIWindow_CurrentChest->Update();
    });
    EXPECT_TRUE(opened);
    EXPECT_NE(pGUIWindow_CurrentChest, nullptr);
    if (pGUIWindow_CurrentChest)
        EXPECT_EQ(pGUIWindow_CurrentChest->chestId(), openIndex);
    EXPECT_EQ(current_screen_type, SCREEN_CHEST);
    EXPECT_TRUE(gameTimer->isPaused());
    EXPECT_TRUE(vChests[openIndex].Initialized());
    EXPECT_NE(pOutdoor->mm6DeltaBytes, beforeOpenSidecar);
    EXPECT_EQ(window->nativeHandle(), nullptr);
    game.runGameRoutine([] {
        pGUIWindow_CurrentChest.reset();
        current_screen_type = SCREEN_GAME;
        gameTimer->setPaused(false);
    });
}

GAME_TEST(MM6ChestTrap, FailedDisarmSetsOffTrap) {
    // Trapped MM6 chests never opened. A party that fails the disarm sets the trap off with the first click, and the
    // second click opens the chest.
    enterOutd3(game);
    for (Character &character : pParty->pCharacters)
        character.health = 200; // A new party dies to the trap.
    auto hpTape = charTapes.hps();
    auto chestTape = tapes.custom([] { return chestContents(4); });
    auto trapTape = tapes.custom([] { return trapSprites(); });
    auto screenTape = tapes.screen();
    auto soundsTape = tapes.sounds();
    test.startTaping();

    clickOutd3Chest4(game);
    game.tick(7); // The trap's flash burns out and hits the party.
    game.pointMouseAtFace(2308);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();

    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_CHEST));
    EXPECT_EQ(trapTape, tape(0, 1, 0));
    ASSERT_EQ(hpTape.size(), 2);
    for (int hp : hpTape.back())
        EXPECT_LT(hp, 200);
    ASSERT_EQ(chestTape.size(), 2);
    EXPECT_EQ(chestTape.front().front(), std::to_underlying(CHEST_TRAPPED));
    EXPECT_EQ(chestTape.back().front(), std::to_underlying(CHEST_ITEMS_PLACED)); // The trap is gone.
    EXPECT_GT(chestTape.back().size(), 1);
    EXPECT_CONTAINS(soundsTape.flatten(), SOUND_fireBall);
    EXPECT_CONTAINS(soundsTape.flatten(), SOUND_openchest0101);
}

GAME_TEST(MM6ChestTrap, DisarmedTrapOpensChest) {
    // Trapped MM6 chests never opened. A character good enough at disarming opens one with the first click and no harm.
    enterOutd3(game);
    pParty->activeCharacter().setSkillValue(SKILL_TRAP_DISARM, CombinedSkillValue(6, MASTERY_NOVICE)); // Beats outd3's lock even on the worst roll.
    auto hpTape = charTapes.hps();
    auto chestTape = tapes.custom([] { return chestContents(4); });
    auto trapTape = tapes.custom([] { return trapSprites(); });
    auto screenTape = tapes.screen();
    auto soundsTape = tapes.sounds();
    test.startTaping();

    clickOutd3Chest4(game);

    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_CHEST));
    EXPECT_EQ(trapTape, tape(0));
    EXPECT_EQ(hpTape.size(), 1);
    ASSERT_EQ(chestTape.size(), 2);
    EXPECT_EQ(chestTape.front().front(), std::to_underlying(CHEST_TRAPPED));
    EXPECT_EQ(chestTape.back().front(), std::to_underlying(CHEST_ITEMS_PLACED));
    EXPECT_GT(chestTape.back().size(), 1);
    EXPECT_MISSES(soundsTape.flatten(), SOUND_fireBall);
    EXPECT_CONTAINS(soundsTape.flatten(), SOUND_openchest0101);
}

GAME_TEST(MM6ChestTrap, SprungTrapStaysGoneAfterReturn) {
    // A chest whose trap went off stays untrapped with its items when the party leaves outd3 and comes back.
    enterOutd3(game);
    for (Character &character : pParty->pCharacters)
        character.health = 200; // A new party dies to the trap.
    const MapId outd3 = pMapTable->GetMapInfo("outd3.odm");
    auto mapTape = tapes.map();
    auto chestTape = tapes.custom([=] { return engine->_currentLoadedMapId == outd3 ? std::optional(chestContents(4)) : std::nullopt; });
    auto trapTape = tapes.custom([] { return trapSprites(); });
    auto screenTape = tapes.screen();
    test.startTaping();

    clickOutd3Chest4(game);
    game.tick(7); // The trap's flash burns out and hits the party.
    walkOverEdge(game, outd3, Vec3f(22450, -10000, 164), 0); // The east edge.
    walkOverEdge(game, pMapTable->GetMapInfo("oute3.odm"), Vec3f(-22450, -10000, 100), 180); // The west edge.
    clickOutd3Chest4(game);

    EXPECT_EQ(mapTape, tape(outd3, pMapTable->GetMapInfo("oute3.odm"), outd3));
    EXPECT_EQ(screenTape.back(), SCREEN_CHEST);
    EXPECT_EQ(trapTape.max(), 1); // Only the first click sets a trap off.
    ASSERT_EQ(chestTape.size(), 4);
    EXPECT_EQ(chestTape[0]->front(), std::to_underlying(CHEST_TRAPPED));
    EXPECT_EQ(chestTape[1]->front(), std::to_underlying(CHEST_ITEMS_PLACED));
    EXPECT_EQ(chestTape[2], std::nullopt);
    EXPECT_EQ(chestTape[3], chestTape[1]);
}

GAME_TEST(MM6ChestArtifact, UnopenedChestArtifactComesBackAfterRespawn) {
    // An artifact generated in a chest the party never opened was lost for good once the map respawned. It stayed on the
    // found list, so the respawned chest rolled something else with the same luck, and no chest could ever roll it again.
    startNewGameThroughInput(game);
    const MapId hive = pMapTable->GetMapInfo("hive.blv");
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId oracle = pMapTable->GetMapInfo("oracle.blv");
    auto mapTape = tapes.map();
    auto screenTape = tapes.screen();
    auto respawnTape = tapes.custom([=] { return engine->_currentLoadedMapId == hive ? std::optional(pIndoor->dlv.respawnCount) : std::nullopt; });
    auto artifactTape = tapes.custom([=] { return engine->_currentLoadedMapId == hive ? std::optional(chestArtifacts(4)) : std::nullopt; });
    auto chestTape = tapes.custom([=] { return engine->_currentLoadedMapId == hive ? std::optional(chestContents(4)) : std::nullopt; });
    test.startTaping();

    mm6::chestTitleState().lcgState = hiveArtifactLuck;
    game.teleportTo(hive, Vec3f(435, 3707, 1), 90);
    reopenHiveChest4AfterRespawn(game);

    EXPECT_EQ(mapTape, tape(oute3, hive, oracle, hive));
    EXPECT_EQ(screenTape.back(), SCREEN_CHEST);
    EXPECT_EQ(respawnTape, tape(std::optional<int>(), 0, std::nullopt, 1));
    // Chest 4 holds no artifact while the hive loads, before its chests are generated.
    EXPECT_EQ(artifactTape, tape(std::optional<std::vector<int>>(), std::vector<int>(), std::vector{415}, std::nullopt, std::vector<int>(), std::vector{415}));
    ASSERT_TRUE(chestTape.back());
    EXPECT_EQ(chestTape.back()->front(), std::to_underlying(CHEST_ITEMS_PLACED));
    EXPECT_TRUE(std::ranges::contains(*chestTape.back(), 415));
}

GAME_TEST(MM6ChestArtifact, RefundCommandReturnsAnUnopenedChestArtifact) {
    // GrayFace's patched MM6 scripts run event command 69 for a chest the party can't reach, so the artifacts it rolled
    // can be found elsewhere. The command alone brings the artifact back in the respawned chest.
    startNewGameThroughInput(game);
    engine->config->gameplay.FixUnopenedChestArtifacts.setValue(false);
    const MapId hive = pMapTable->GetMapInfo("hive.blv");
    auto artifactTape = tapes.custom([=] { return engine->_currentLoadedMapId == hive ? std::optional(chestArtifacts(4)) : std::nullopt; });
    auto chestTape = tapes.custom([=] { return engine->_currentLoadedMapId == hive ? std::optional(chestContents(4)) : std::nullopt; });
    auto respawnTape = tapes.custom([=] { return engine->_currentLoadedMapId == hive ? std::optional(pIndoor->dlv.respawnCount) : std::nullopt; });
    test.startTaping();

    mm6::chestTitleState().lcgState = hiveArtifactLuck;
    game.teleportTo(hive, Vec3f(435, 3707, 1), 90);
    (void) EvtInterpreter().executeInstruction(parsedMM6Instruction(EVENT_RefundChestArtifacts, std::string("\x04", 1))); // Chest 4.
    reopenHiveChest4AfterRespawn(game);

    EXPECT_EQ(respawnTape, tape(std::optional<int>(), 0, std::nullopt, 1));
    EXPECT_EQ(artifactTape, tape(std::optional<std::vector<int>>(), std::vector<int>(), std::vector{415}, std::nullopt, std::vector<int>(), std::vector{415}));
    ASSERT_TRUE(chestTape.back());
    EXPECT_EQ(chestTape.back()->front(), std::to_underlying(CHEST_ITEMS_PLACED));
    EXPECT_TRUE(std::ranges::contains(*chestTape.back(), 415));
}

GAME_TEST(MM6ChestScreen, ExitButton) {
    // An open chest drew MM7's exit button art, which MM6 doesn't have, below the 3D view. MM6 draws its exit button in
    // the 3D view's corner over the chest, with the view's side borders, and the button closes the chest.
    startNewGameThroughInput(game);
    auto quadsTape = tapes.hudQuads();
    auto screenTape = tapes.screen();
    test.startTaping();

    game.teleportTo(engine->_currentLoadedMapId, Vec3f(-15950, -1680, 0), 180, -20); // East of the box that holds chest 1, looking down at it.
    game.pointMouseAtFace(866);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
    const auto chestFrame = quadsTape.back();
    game.pressAndReleaseButton(BUTTON_LEFT, 429, 334); // The middle of the exit button.
    game.tick();

    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_CHEST, SCREEN_GAME));
    auto frameArt = chestFrame.filter([](const DrawnQuad &quad) {
        return quad.texture.starts_with("chest") || quad.texture.starts_with("border5") || quad.texture.starts_with("border6") ||
               quad.texture.starts_with("buttesc") || quad.texture.starts_with("ib-");
    });
    EXPECT_EQ(frameArt, tape(DrawnQuad{"chest01", {8, 8}}, DrawnQuad{"border5", {7, 8}}, DrawnQuad{"border6", {461, 8}}, DrawnQuad{"buttesc1", {392, 318}}));
}

GAME_TEST(MM6Chest, GoldPileGoesToTheParty) {
    // Clicking a gold pile in a chest did nothing, the pile stayed where it was. The party takes the 983 gold pile in chest
    // 1 of the Abandoned Temple, once alone and once with Noreen the Banker hired, who adds a fifth and takes her share.
    constexpr ItemId mediumGold = static_cast<ItemId>(198);
    for (bool banker : {false, true}) {
        SCOPED_TRACE(fmt::format("banker={}", banker));
        test.prepareForNextTest();
        startNewGameThroughInput(game);
        if (banker) {
            game.runGameRoutine([] { pParty->SetGold(1000, true); }); // Her fee.
            hireNoreenTheBanker(game);
        }
        walkIntoAbandonedTemple(game, 3041712726u);
        auto goldTape = tapes.gold();
        auto pilesTape = tapes.custom([] { return chestGoldPiles(1); });
        auto heldTape = tapes.custom([] { return pParty->pPickedItem.itemId; });
        auto statusTape = tapes.statusBar();
        auto soundsTape = tapes.sounds();
        test.startTaping();

        useMM6CampaignChainIndoorEventFace(game, 12); // Opens chest 1.
        ASSERT_EQ(vChests[1].inventory.entry(Pointi(0, 0))->itemId, mediumGold);
        ASSERT_EQ(vChests[1].inventory.entry(Pointi(0, 0))->goldAmount, 983);
        game.pressAndReleaseButton(BUTTON_LEFT, chestTable[vChests[1].chestTypeId].inventoryOffset + Pointi(16, 16)); // The pile in the top left cell.
        game.tick();

        EXPECT_EQ(pilesTape, tape(4, 3));
        EXPECT_EQ(heldTape, tape(ITEM_NULL));
        EXPECT_CONTAINS(soundsTape.flatten(), SOUND_gold01);
        if (banker) {
            EXPECT_EQ(goldTape, tape(0, 1062)); // 983 and a fifth make 1179, the banker takes a tenth of it.
            EXPECT_EQ(statusTape.back(), localization->format(LSTR_YOU_FOUND_LU_GOLD_FOLLOWERS_TAKE_LU, 1179, 117));
        } else {
            EXPECT_EQ(goldTape, tape(200, 1183));
            EXPECT_EQ(statusTape.back(), localization->format(LSTR_YOU_FOUND_LU_GOLD, 983));
        }
    }
}

GAME_TEST(MM6Chest, OverflowShowsMostValuableItems) {
    // Chest items that didn't fit the grid on first opening were never shown. This roll puts twelve items into chest 1
    // of the Abandoned Temple. MM6 showed eight, four gold piles among them, and hid four weapons. Placed most valuable
    // first and packed into rows, only the Emerald Staff, nine cells tall, has no room on the first page.
    constexpr int ornateTrident = 40, longbow = 42, crossbow = 47, emeraldStaff = 62;
    for (bool fix : {false, true}) {
        SCOPED_TRACE(fmt::format("fix={}", fix));
        test.prepareForNextTest();
        engine->config->gameplay.FixChestOverflow.setValue(fix);
        enterAbandonedTempleWithChestRoll(game, 3041712726u);
        auto hiddenTape = tapes.custom([] { return hiddenChestItems(1); });
        auto quadsTape = tapes.hudQuads();
        auto screenTape = tapes.screen();
        test.startTaping();

        useMM6CampaignChainIndoorEventFace(game, 12); // Opens chest 1.

        EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_CHEST));
        if (fix) {
            EXPECT_EQ(hiddenTape.back(), std::vector<int>{emeraldStaff});
            EXPECT_EQ(drawnChestItems(quadsTape.back(), 1), 11);
        } else {
            EXPECT_EQ(hiddenTape.back(), (std::vector<int>{ornateTrident, crossbow, emeraldStaff, longbow}));
            EXPECT_EQ(drawnChestItems(quadsTape.back(), 1), 8);
        }
        EXPECT_EQ(hiddenTape.front().size(), 12); // Nothing has a cell before the first opening.
    }
}

GAME_TEST(MM6Chest, OverflowItemIsOnTheSecondPage) {
    // A chest item that had no room on the chest's grid was never shown. With this roll the Emerald Staff has no room in
    // chest 1 of the Abandoned Temple. It is on the chest's second page, which the page button turns to, and the party
    // takes it from there. Then the chest has one page and no button. Without the fix there is no button and no staff.
    constexpr ItemId emeraldStaff = static_cast<ItemId>(62);
    const std::string staffIcon = pItemTable->items[emeraldStaff].iconName;
    for (bool fix : {false, true}) {
        SCOPED_TRACE(fmt::format("fix={}", fix));
        test.prepareForNextTest();
        engine->config->gameplay.FixChestOverflow.setValue(fix);
        enterAbandonedTempleWithChestRoll(game, 3041712726u);
        auto pageButtonTape = tapes.custom([] { return chestPageButton() != nullptr; });
        auto heldTape = tapes.custom([] { return pParty->pPickedItem.itemId; });
        auto quadsTape = tapes.hudQuads();
        auto screenTape = tapes.screen();
        test.startTaping();

        useMM6CampaignChainIndoorEventFace(game, 12); // Opens chest 1.
        const auto firstPage = quadsTape.back();
        const int firstPageItems = drawnChestItems(firstPage, 1);
        if (!fix) {
            EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_CHEST));
            EXPECT_EQ(pageButtonTape, tape(false));
            EXPECT_FALSE(firstPage.contains([&](const DrawnQuad &quad) { return quad.texture == staffIcon; }));
            continue;
        }
        game.pressGuiButton("Chest_NextPage");
        game.tick();
        const auto secondPage = quadsTape.back();
        const int secondPageItems = drawnChestItems(secondPage, 1);
        game.pressAndReleaseButton(BUTTON_LEFT, chestTable[vChests[1].chestTypeId].inventoryOffset + Pointi(16, 16)); // The staff's top cell.
        game.tick();

        EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_CHEST));
        EXPECT_EQ(pageButtonTape, tape(false, true, false));
        EXPECT_EQ(heldTape, tape(ITEM_NULL, emeraldStaff));
        EXPECT_EQ(firstPageItems, 11);
        EXPECT_FALSE(firstPage.contains([&](const DrawnQuad &quad) { return quad.texture == staffIcon; }));
        EXPECT_EQ(secondPageItems, 1);
        EXPECT_TRUE(secondPage.contains([&](const DrawnQuad &quad) { return quad.texture == staffIcon; }));
        EXPECT_EQ(drawnChestItems(quadsTape.back(), 1), 11); // Back on the first page.
    }
}

GAME_TEST(MM6Chest, SecondPageStaysAfterLeavingTheMap) {
    // A chest's second page is kept with the map. With this roll chest 1 of the Abandoned Temple has two items on its
    // second page. The party takes one, leaves the temple and comes back, and the chest still shows the other one there.
    enterAbandonedTempleWithChestRoll(game, secondPageRoll);
    auto pagesTape = tapes.custom([] { return chestPages(1); });
    auto heldTape = tapes.custom([] { return pParty->pPickedItem.itemId; });
    auto mapTape = tapes.map();
    test.startTaping();

    useMM6CampaignChainIndoorEventFace(game, 12); // Opens chest 1.
    game.pressGuiButton("Chest_NextPage");
    game.tick();
    const std::vector<std::pair<ItemId, Pointi>> opened = pagesTape.back();
    ASSERT_EQ(opened.size(), 2);
    game.pressAndReleaseButton(BUTTON_LEFT, chestTable[vChests[1].chestTypeId].inventoryOffset + opened.front().second * 32 + Pointi(16, 16));
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    game.pressGuiButton("Game_Character1"); // Into the first backpack.
    game.tick();
    game.teleportTo(engine->_currentLoadedMapId, Vec3f(16384, -19626, 864.5f), 270); // In front of the way out, facing it.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    game.pressGuiButton("Transition_Yes");
    game.tick();
    game.skipLoadingScreen();
    game.tick();
    game.teleportTo(engine->_currentLoadedMapId, Vec3f(-21456, -286, 193), 90); // In front of the temple door, facing it.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    game.pressGuiButton("Transition_Yes");
    game.tick();
    game.skipLoadingScreen();
    game.tick();
    useMM6CampaignChainIndoorEventFace(game, 12);
    game.pressGuiButton("Chest_NextPage");
    game.tick();

    const MapId d02 = pMapTable->GetMapInfo("d02.blv");
    EXPECT_EQ(mapTape, tape(d02, pMapTable->GetMapInfo("oute3.odm"), d02));
    EXPECT_EQ(heldTape, tape(ITEM_NULL, opened.front().first, ITEM_NULL));
    EXPECT_EQ(pagesTape.back(), std::vector(opened.begin() + 1, opened.end()));
}

GAME_TEST(MM6ChestHeadless, DISABLED_ChestWithSecondPageSaves) {
    // A party takes one of the items on the second page of the Abandoned Temple's chest 1 and saves in the temple, the
    // first half of the chest page round trip.
    if (!enabled("strict-chest-pages-write"))
        GTEST_SKIP() << "Run only as the chest page write child of the first-map gate";
    const std::filesystem::path profile = ownerOnlyProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to an owner-only profile";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    const std::filesystem::path slot = profile / "saves/save000.ec6";
    ASSERT_FALSE(std::filesystem::exists(slot));
    game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });

    enterAbandonedTempleWithChestRoll(game, secondPageRoll);
    auto pagesTape = tapes.custom([] { return chestPagesText(1); });
    auto statusTape = tapes.statusBar();
    test.startTaping();

    useMM6CampaignChainIndoorEventFace(game, 12); // Opens chest 1.
    game.pressGuiButton("Chest_NextPage");
    game.tick();
    const mm6::ChestPageItem taken = vChests[1].mm6Pages.front();
    game.pressAndReleaseButton(BUTTON_LEFT, chestTable[vChests[1].chestTypeId].inventoryOffset + Pointi(taken.cell % 9, taken.cell / 9) * 32 + Pointi(16, 16));
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    game.pressGuiButton("Game_Character1"); // Into the first backpack.
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick(2);
    game.pressGuiButton("GameMenu_SaveGame");
    game.tick(2);
    game.doubleClickGuiButton("SaveMenu_Slot0"); // The new save slot.
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_C);
    game.tick();
    game.pressGuiButton("SaveMenu_Save");
    game.tick(3); // The button animates for a frame, then posts the save for the next one.

    EXPECT_CONTAINS(statusTape, localization->str(LSTR_GAME_SAVED));
    EXPECT_TRUE(std::filesystem::is_regular_file(slot));
    ASSERT_EQ(pagesTape.size(), 3); // Shut, opened, then one item fewer on the second page.
    EXPECT_TRUE(pagesTape.back().contains(" p2:"));
    game.runGameRoutine([&] {
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    });
    std::ofstream(profile / "expected-chest.txt") << pagesTape.back() << "\n";
}

GAME_TEST(MM6ChestHeadless, DISABLED_ChestKeepsItsSecondPageInFreshProcess) {
    // A fresh MM6 process loads the temple save, and chest 1 still has both pages with the items the party left there.
    if (!enabled("strict-chest-pages-reload"))
        GTEST_SKIP() << "Run only after the chest page write child of the first-map gate";
    const std::filesystem::path profile = ownerOnlyProfile();
    if (profile.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_RESTART_PROFILE to an owner-only profile";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    std::istringstream expected(fileText(profile / "expected-chest.txt"));
    std::string savedPages;
    ASSERT_TRUE(std::getline(expected, savedPages));
    game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });

    test.startTaping();
    auto screenTape = tapes.screen();
    auto mapTape = tapes.map();
    auto quadsTape = tapes.hudQuads();

    game.pressGuiButton("MainMenu_LoadGame");
    game.tick(4);
    ASSERT_EQ(screenTape.back(), SCREEN_LOADGAME);
    const auto chestSlot = std::ranges::find(saveLoadMenu()->slots(), "save000.ec6", [](const auto &slot) { return slot.fileName; });
    ASSERT_NE(chestSlot, saveLoadMenu()->slots().end());
    game.pressGuiButton(fmt::format("LoadMenu_Slot{}", chestSlot - saveLoadMenu()->slots().begin()));
    game.tick(2);
    game.pressGuiButton("LoadMenu_Load");
    game.skipLoadingScreen();
    game.tick();
    ASSERT_EQ(mapTape.back(), pMapTable->GetMapInfo("d02.blv"));
    useMM6CampaignChainIndoorEventFace(game, 12); // Opens chest 1.
    const int firstPageItems = drawnChestItems(quadsTape.back(), 1);
    game.pressGuiButton("Chest_NextPage");
    game.tick();

    EXPECT_EQ(chestPagesText(1), savedPages);
    EXPECT_GT(firstPageItems, 0);
    EXPECT_EQ(drawnChestItems(quadsTape.back(), 1), std::ranges::count(vChests[1].mm6Pages, 1, &mm6::ChestPageItem::page));
    game.runGameRoutine([&] {
        mm6::disarmFirstMapManualSave();
        mm6::testing::setNativeProfileRootForTest({});
    });
}
