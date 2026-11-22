#include <unistd.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iterator>
#include <memory>
#include <numbers>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Application/GameConfig.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Localization.h"
#include "Engine/MM6/MM6ArtifactCollection.h"
#include "Engine/MM6/MM6ChestTitleState.h"
#include "Engine/MM6/MM6FirstMapManualSave.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/ItemEnumFunctions.h"
#include "Engine/Objects/Monsters.h"
#include "Engine/Party.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/UIMonsterLoot.h"
#include "GUI/UI/UIRest.h"

#include "Library/Environment/Interface/Environment.h"

#include "Utility/ScopeGuard.h"

static constexpr std::uint32_t kMerlinLuck = 17; // Item luck that rolls artifact 404, Merlin, at tier 6.
static constexpr ItemId kMerlin = static_cast<ItemId>(404);
static constexpr MonsterId kGreatWyrm = static_cast<MonsterId>(39); // Loot is gold and a level 6 item of any kind.
static const Vec3f kTownHallDoor(-11090, -6536, 300); // New Sorpigal Town Hall, facing west.
static constexpr ItemId kGoldenPyramid = static_cast<ItemId>(472);
static const Vec3f kNajatTentDoor(-7300, -12048, 224); // Najat Ramadi's circus tent in Dragonsand, facing east.

struct ScopedEnv {
    // Sets a launch variable the way Atlas does, and clears it when the test is done with it.
    ScopedEnv(std::string_view name, std::string_view value) : name(name) {
        environment->setenv(this->name, std::string(value));
    }
    ~ScopedEnv() {
        environment->setenv(name, "");
    }
    std::string name;
    std::unique_ptr<Environment> environment = Environment::createStandardEnvironment();
};

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

static int spawnWyrm(EngineController &game) {
    // A Great Wyrm at 1 HP in front of the party.
    const Vec3f ahead(std::cos(pParty->_viewYaw * std::numbers::pi / 1024), std::sin(pParty->_viewYaw * std::numbers::pi / 1024), 0);
    Actor *wyrm = game.spawnMonster(pParty->pos + ahead * 400, kGreatWyrm, SPAWN_DUMMY);
    wyrm->hp = 1;
    return wyrm->id;
}

static void killAndClickCorpse(EngineController &game, int actorId) {
    // The party hits the monster until it dies, waits for it to fall, and clicks the corpse.
    for (int frame = 0; frame < 300 && !pActors[actorId].IsNotAlive(); frame++) { // Blows can miss.
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick();
    }
    for (int frame = 0; frame < 100 && pActors[actorId].aiState != Dead; frame++) // The monster falls.
        game.tick();
    game.pointMouseAtActor(actorId);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
}

static void clickButton(EngineController &game, std::string_view button) {
    game.hoverGuiButton(button);
    game.tick();
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
}

static std::optional<MonsterLootOffer> shownOffer() {
    // What the Fortune's Turn popup offers, empty while it's closed.
    for (GUIWindow *window : lWindowList)
        if (const auto *popup = dynamic_cast<const GUIWindow_MonsterLoot *>(window))
            return popup->offer();
    return std::nullopt;
}

static std::optional<std::pair<int, ItemId>> shownLoot() {
    if (std::optional<MonsterLootOffer> offer = shownOffer())
        return std::pair(offer->gold, offer->item.itemId);
    return std::nullopt;
}

static std::vector<ItemId> carriedItems() {
    // Every item in the backpacks and on the cursor, sorted.
    std::vector<ItemId> result;
    for (const Character &character : pParty->pCharacters)
        for (InventoryConstEntry entry : character.inventory.entries())
            result.push_back(entry->itemId);
    if (pParty->pPickedItem.itemId != ITEM_NULL)
        result.push_back(pParty->pPickedItem.itemId);
    std::ranges::sort(result);
    return result;
}

static std::vector<ItemId> newItems(const std::vector<ItemId> &before, const std::vector<ItemId> &after) {
    std::vector<ItemId> result;
    std::ranges::set_difference(after, before, std::back_inserter(result));
    return result;
}

static void pressSpaceAt(EngineController &game, Vec3f position, int viewYaw) {
    // Same map teleport, then Space at the door in front of the party.
    game.teleportTo(engine->_currentLoadedMapId, position, viewYaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
}

static void waitUntilTownHallOpens(EngineController &game) {
    // Waits in the rest screen an hour at a time until 10 am, when the Town Hall opens.
    game.pressAndReleaseKey(PlatformKey::KEY_R);
    game.tick();
    while (pParty->GetPlayingTime().toCivilTime().hour < 10) {
        game.pressGuiButton("Rest_Wait1Hour");
        game.tick();
        while (currentRestType != REST_NONE)
            game.tick(); // The hour passes.
    }
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
}

static void clickOption(EngineController &game, GUIWindow *window, std::string_view label) {
    // Left click in the middle of the shown house option with this line.
    ASSERT_NE(window, nullptr);
    std::optional<Pointi> center;
    for (GUIButton *button : window->vButtons)
        if (button->id.contains("Dialogue_Option") && button->rect.h > 0 && button->label == label)
            center = button->rect.center();
    ASSERT_TRUE(center) << "No option " << label;
    game.pressAndReleaseButton(BUTTON_LEFT, *center);
    game.tick();
}

static void askJaniceForBounty(EngineController &game) {
    // The party enters the Town Hall, picks Janice and asks her about the bounty.
    pressSpaceAt(game, kTownHallDoor, 180);
    ASSERT_NE(window_SpeakInHouse, nullptr);
    std::optional<Pointi> portrait;
    for (GUIButton *button : window_SpeakInHouse->vButtons)
        if (button->id.starts_with("House_Npc") && button->label.contains("Janice"))
            portrait = button->rect.center();
    ASSERT_TRUE(portrait);
    game.pressAndReleaseButton(BUTTON_LEFT, *portrait);
    game.tick();
    clickOption(game, window_SpeakInHouse.get(), "Bounty Hunt");
}

static void tradePyramidWithNajat(EngineController &game) {
    // The party enters Najat Ramadi's circus tent and picks her pyramid trade.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    clickOption(game, window_SpeakInHouse.get(), "Trade Pyramid for Armor");
}

static void leaveTownHall(EngineController &game) {
    for (int i = 0; i < 3 && current_screen_type != SCREEN_GAME; ++i) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    game.teleportTo(engine->_currentLoadedMapId, kTownHallDoor, 0);
}

static void killBountyMonster(EngineController &game) {
    // The party hits a dummy of this month's New Sorpigal bounty monster in front of it until it's dead.
    Actor *monster = game.spawnMonster(kTownHallDoor + Vec3f(200, 0, 0), pParty->monster_id_for_hunting[HOUSE_FIRST_TOWN_HALL], SPAWN_DUMMY);
    monster->hp = 1;
    game.pointMouseAtActor(monster->id);
    for (int attack = 0; attack < 1000 && monster->aiState != Dead; attack++) {
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick();
    }
    for (int frame = 0; frame < 100 && !pParty->hasActiveCharacter(); frame++)
        game.tick(); // A door opens only for a character who has recovered from the fight.
}

static std::filesystem::path createTempProfile() {
    // An owner-only mm6-restart- folder in the system temp folder, the only kind of profile a test saves into.
    const std::filesystem::path tempRoot = std::filesystem::canonical(std::filesystem::temp_directory_path());
    std::string created = (tempRoot / "mm6-restart-save-XXXXXX").string();
    if (!mkdtemp(created.data()))
        throw std::runtime_error("Can't create a temporary MM6 profile");
    return std::filesystem::canonical(created);
}

static void saveFromMenu(EngineController &game) {
    // Saves into the new slot from the Escape menu.
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
}

static void loadFromEscapeMenu(EngineController &game, int slot) {
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick(2);
    game.pressGuiButton("GameMenu_LoadGame");
    game.tick(3);
    game.pressGuiButton(fmt::format("LoadMenu_Slot{}", slot));
    game.tick(2);
    game.pressGuiButton("LoadMenu_Load");
    game.skipLoadingScreen();
    game.tick();
}

GAME_TEST(MM6FortunesTurn, CorpseLootRerolls) {
    // With Fortune's Turn a dead monster's loot shows in a popup. Reroll rolls it again without marking the artifact it
    // showed, and Take gives the shown gold and item to the party and removes the corpse.
    for (bool reroll : {true, false}) {
        SCOPED_TRACE(fmt::format("reroll={}", reroll));
        test.prepareForNextTest();
        ScopedEnv mod("ENROTH_MM6_MONSTER_LOOT_REROLL", "1");
        ScopedEnv artifacts("ENROTH_MM6_ALL_ARTIFACTS", "1");
        startNewGameThroughInput(game);
        engine->config->debug.NoDamage.setValue(true); // The wyrm breathes on the party.
        const int wyrmId = spawnWyrm(game);
        auto screenTape = tapes.screen();
        auto offerTape = tapes.custom([] { return shownLoot(); });
        auto goldTape = tapes.gold();
        auto itemsTape = tapes.custom([] { return carriedItems(); });
        auto wyrmTape = actorTapes.aiState(wyrmId);
        auto generatedTape = tapes.custom([] { return mm6::chestTitleState().generatedArtifactFlags[404 - 400]; });
        auto collectedTape = tapes.custom([] { return mm6::artifactCollection().collected[404 - 400]; });
        auto textTape = tapes.drawnTexts();
        test.startTaping();

        mm6::chestTitleState().lcgState = kMerlinLuck;
        killAndClickCorpse(game, wyrmId);
        if (reroll)
            clickButton(game, "MonsterLoot_Reroll");
        clickButton(game, "MonsterLoot_Take");

        EXPECT_EQ(screenTape.back(), SCREEN_GAME);
        EXPECT_CONTAINS(screenTape, SCREEN_MONSTER_LOOT);
        ASSERT_EQ(offerTape.size(), reroll ? 4 : 3);
        EXPECT_EQ(offerTape.front(), std::nullopt);
        EXPECT_EQ(offerTape[1]->second, kMerlin);
        EXPECT_EQ(offerTape.back(), std::nullopt);
        const std::pair<int, ItemId> taken = *offerTape[offerTape.size() - 2];
        if (reroll)
            EXPECT_NE(taken, *offerTape[1]);
        ASSERT_NE(taken.second, ITEM_NULL);
        EXPECT_EQ(goldTape, tape(200, 200 + taken.first));
        ASSERT_EQ(itemsTape.size(), 2);
        EXPECT_EQ(newItems(itemsTape.front(), itemsTape.back()), std::vector{taken.second});
        EXPECT_EQ(wyrmTape.back(), Removed);
        EXPECT_EQ(generatedTape, reroll ? tape(0) : tape(0, 1));
        EXPECT_EQ(collectedTape, reroll ? tape(0) : tape(0, 1));
        EXPECT_CONTAINS(textTape.flatten().map([](const DrawnText &text) { return text.text; }),
                        fmt::format("Gold: {}", offerTape[1]->first));
    }
}

GAME_TEST(MM6FortunesTurn, CorpseLootWithoutTheSwitch) {
    // Without the launch flag, or with the dice checkbox clicked off, a corpse gives its loot at once, as in MM6.
    for (bool flag : {false, true}) {
        SCOPED_TRACE(fmt::format("flag={}", flag));
        test.prepareForNextTest();
        ScopedEnv mod("ENROTH_MM6_MONSTER_LOOT_REROLL", flag ? "1" : "0");
        startNewGameThroughInput(game);
        engine->config->debug.NoDamage.setValue(true); // The wyrm breathes on the party.
        const int wyrmId = spawnWyrm(game);
        auto screenTape = tapes.screen();
        auto checkboxTape = tapes.config(engine->config->gameplay.MM6MonsterLootRerollInGame);
        auto merlinTape = tapes.custom([] { return std::ranges::count(carriedItems(), kMerlin); });
        auto wyrmTape = actorTapes.aiState(wyrmId);
        test.startTaping();

        if (flag)
            clickButton(game, "MonsterLoot_InGameToggle");
        mm6::chestTitleState().lcgState = kMerlinLuck;
        killAndClickCorpse(game, wyrmId);

        EXPECT_EQ(checkboxTape, flag ? tape(true, false) : tape(true));
        EXPECT_FALSE(screenTape.contains(SCREEN_MONSTER_LOOT));
        EXPECT_EQ(merlinTape, tape(0, 1));
        EXPECT_CONTAINS(wyrmTape, Dead);
    }
}

GAME_TEST(MM6FortunesTurn, HangingCageItemRerolls) {
    // A hanging cage in the Abandoned Temple gives a random item once. With Fortune's Turn the item shows in a popup
    // that can reroll it, Take puts it on the cursor, and the cage gives nothing more. Without it the item goes on the
    // cursor at once.
    for (bool flag : {true, false}) {
        SCOPED_TRACE(fmt::format("flag={}", flag));
        test.prepareForNextTest();
        ScopedEnv mod("ENROTH_MM6_MONSTER_LOOT_REROLL", flag ? "1" : "0");
        startNewGameThroughInput(game);
        game.teleportTo(pMapTable->GetMapInfo("d02.blv"), Vec3f(7596, 10368, -1850), 180); // East of the cage, facing it.
        auto screenTape = tapes.screen();
        auto offerTape = tapes.custom([] { return shownOffer() ? std::optional(shownOffer()->item.itemId) : std::nullopt; });
        auto heldTape = tapes.custom([] { return pParty->pPickedItem.itemId; });
        test.startTaping();

        constexpr int cage = 107; // Its event gives a level 1 item of any kind once.
        for (int click = 0; click < 2; click++) {
            game.pointMouseAtDecoration(cage);
            game.pressAndReleaseButton(BUTTON_LEFT);
            game.tick();
            if (flag && click == 0) {
                clickButton(game, "MonsterLoot_Reroll");
                clickButton(game, "MonsterLoot_Take");
            }
        }

        if (flag) {
            ASSERT_EQ(offerTape.size(), 4);
            EXPECT_NE(offerTape[1], offerTape[2]);
            EXPECT_EQ(heldTape, tape(ITEM_NULL, *offerTape[2]));
            EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_MONSTER_LOOT, SCREEN_GAME));
        } else {
            EXPECT_EQ(offerTape, tape(std::optional<ItemId>()));
            ASSERT_EQ(heldTape.size(), 2);
            EXPECT_NE(heldTape.back(), ITEM_NULL);
            EXPECT_FALSE(screenTape.contains(SCREEN_MONSTER_LOOT));
        }
    }
}

GAME_TEST(MM6FortunesTurn, BountyRerolls) {
    // With Fortune's Turn Janice's monthly bounty shows in a popup that can reroll the monster. The deadline stays, and
    // once the monster is dead she pays without offering a reroll.
    ScopedEnv mod("ENROTH_MM6_MONSTER_LOOT_REROLL", "1");
    startNewGameThroughInput(game);
    waitUntilTownHallOpens(game);
    auto screenTape = tapes.screen();
    auto bountyTape = tapes.custom([] { return pParty->monster_id_for_hunting[HOUSE_FIRST_TOWN_HALL]; });
    auto deadlineTape = tapes.custom([] { return pParty->PartyTimes.bountyHuntNextGenTime[HOUSE_FIRST_TOWN_HALL]; });
    auto textTape = tapes.custom([] { return current_npc_text; });
    auto goldTape = tapes.gold();
    test.startTaping();

    askJaniceForBounty(game);
    clickButton(game, "MonsterLoot_Reroll");
    clickButton(game, "MonsterLoot_Take");
    leaveTownHall(game);
    killBountyMonster(game);
    askJaniceForBounty(game);
    leaveTownHall(game);

    ASSERT_EQ(bountyTape.size(), 4); // None, rolled, rerolled, claimed.
    const MonsterId rerolled = bountyTape[2];
    EXPECT_NE(bountyTape[1], rerolled);
    EXPECT_EQ(bountyTape.back(), MONSTER_INVALID);
    EXPECT_EQ(deadlineTape.size(), 2);
    EXPECT_EQ(screenTape.count(SCREEN_MONSTER_LOOT), 1);
    const std::string name = fmt::format("{::}{}{::}", colorTable.PaleCanary.tag(), pMonsterStats->infos[rerolled].name, colorTable.White.tag());
    EXPECT_CONTAINS(textTape, fmt::format("This month's bounty is on a {}.  Kill it and return before the end of the month to collect the {} gold reward.",
                                          name, 100 * pMonsterStats->infos[rerolled].level));
    EXPECT_EQ(goldTape, tape(200, 200 + 100 * pMonsterStats->infos[rerolled].level));
}

GAME_TEST(MM6FortunesTurn, SavesAfterThePopup) {
    // The game saves and loads after a Fortune's Turn corpse popup was taken or dismissed.
    for (bool take : {true, false}) {
        SCOPED_TRACE(fmt::format("take={}", take));
        test.prepareForNextTest();
        ScopedEnv mod("ENROTH_MM6_MONSTER_LOOT_REROLL", "1");
        const std::filesystem::path profile = createTempProfile();
        MM_AT_SCOPE_EXIT(std::filesystem::remove_all(profile));
        game.runGameRoutine([&] { mm6::testing::setNativeProfileRootForTest(profile); });
        MM_AT_SCOPE_EXIT(game.runGameRoutine([] {
            mm6::disarmFirstMapManualSave();
            mm6::testing::setNativeProfileRootForTest({});
        }));

        startNewGameThroughInput(game);
        engine->config->debug.NoDamage.setValue(true); // The wyrm breathes on the party.
        const int wyrmId = spawnWyrm(game);
        auto merlinTape = tapes.custom([] { return std::ranges::count(carriedItems(), kMerlin); });
        auto statusTape = tapes.statusBar();
        auto slotTape = tapes.custom([&] { return std::filesystem::exists(profile / "saves/save000.ec6"); });
        test.startTaping();

        mm6::chestTitleState().lcgState = kMerlinLuck;
        killAndClickCorpse(game, wyrmId);
        if (take) {
            clickButton(game, "MonsterLoot_Take");
        } else {
            game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
            game.tick();
        }
        saveFromMenu(game);
        game.goToMainMenu();
        startNewGameThroughInput(game);
        loadFromEscapeMenu(game, 1);

        EXPECT_CONTAINS(statusTape, localization->str(LSTR_GAME_SAVED));
        EXPECT_EQ(slotTape, tape(false, true));
        EXPECT_EQ(merlinTape, take ? tape(0, 1, 0, 1) : tape(0)); // Taken, a new game, the load.
    }
}

GAME_TEST(MM6FortunesTurn, CircusPrizeTradeRerolls) {
    // With Fortune's Turn Najat Ramadi's pyramid trade shows its armor in a popup that can reroll it. The pyramid goes
    // once, and Take or Escape puts the shown armor on the cursor.
    for (bool take : {true, false}) {
        SCOPED_TRACE(fmt::format("take={}", take));
        test.prepareForNextTest();
        ScopedEnv mod("ENROTH_MM6_MONSTER_LOOT_REROLL", "1");
        startNewGameThroughInput(game);
        game.teleportTo(pMapTable->GetMapInfo("outb3.odm"), kNajatTentDoor, 0);
        game.runGameRoutine([] { pParty->setHoldingItem(Item(kGoldenPyramid)); });
        auto screenTape = tapes.screen();
        auto offerTape = tapes.custom([] { return shownOffer() ? std::optional(shownOffer()->item.itemId) : std::nullopt; });
        auto heldTape = tapes.custom([] { return pParty->pPickedItem.itemId; });
        auto pyramidTape = tapes.custom([] { return std::ranges::count(carriedItems(), kGoldenPyramid); });
        auto textTape = tapes.custom([] { return current_npc_text; });
        auto drawnTape = tapes.drawnTexts();
        test.startTaping();

        tradePyramidWithNajat(game);
        clickButton(game, "MonsterLoot_Reroll");
        if (take) {
            clickButton(game, "MonsterLoot_Take");
        } else {
            game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
            game.tick();
        }

        EXPECT_EQ(screenTape, tape(SCREEN_HOUSE, SCREEN_MONSTER_LOOT, SCREEN_HOUSE));
        ASSERT_EQ(offerTape.size(), 4); // Closed, rolled, rerolled, closed.
        EXPECT_NE(offerTape[1], offerTape[2]);
        EXPECT_TRUE(isArmor(pItemTable->items[*offerTape[1]].type));
        EXPECT_TRUE(isArmor(pItemTable->items[*offerTape[2]].type));
        EXPECT_EQ(heldTape, tape(kGoldenPyramid, ITEM_NULL, *offerTape[2]));
        EXPECT_EQ(pyramidTape, tape(1, 0));
        EXPECT_TRUE(textTape.back().contains("here's your armor!"));
        EXPECT_CONTAINS(drawnTape.flatten().map([](const DrawnText &text) { return text.text; }), "Circus prize trade");
    }
}

GAME_TEST(MM6FortunesTurn, CircusPrizeTradeWithoutTheSwitch) {
    // Without the launch flag Najat Ramadi's pyramid trade puts an armor on the cursor at once, as in MM6.
    ScopedEnv mod("ENROTH_MM6_MONSTER_LOOT_REROLL", "0");
    startNewGameThroughInput(game);
    game.teleportTo(pMapTable->GetMapInfo("outb3.odm"), kNajatTentDoor, 0);
    game.runGameRoutine([] { pParty->setHoldingItem(Item(kGoldenPyramid)); });
    auto screenTape = tapes.screen();
    auto heldTape = tapes.custom([] { return pParty->pPickedItem.itemId; });
    auto textTape = tapes.custom([] { return current_npc_text; });
    test.startTaping();

    tradePyramidWithNajat(game);

    EXPECT_EQ(screenTape, tape(SCREEN_HOUSE));
    ASSERT_EQ(heldTape.size(), 2);
    EXPECT_EQ(heldTape.front(), kGoldenPyramid);
    EXPECT_TRUE(isArmor(pItemTable->items[heldTape.back()].type));
    EXPECT_TRUE(textTape.back().contains("here's your armor!"));
}

GAME_TEST(MM6FortunesTurn, CircusPrizeTradeWithoutAPyramid) {
    // With Fortune's Turn on, Najat Ramadi still turns down a party without a pyramid and opens no popup.
    ScopedEnv mod("ENROTH_MM6_MONSTER_LOOT_REROLL", "1");
    startNewGameThroughInput(game);
    game.teleportTo(pMapTable->GetMapInfo("outb3.odm"), kNajatTentDoor, 0);
    auto screenTape = tapes.screen();
    auto itemsTape = tapes.custom([] { return carriedItems(); });
    auto textTape = tapes.custom([] { return current_npc_text; });
    test.startTaping();

    tradePyramidWithNajat(game);

    EXPECT_EQ(screenTape, tape(SCREEN_HOUSE));
    EXPECT_EQ(itemsTape.size(), 1);
    EXPECT_TRUE(textTape.back().contains("you don't have a golden pyramid"));
}
