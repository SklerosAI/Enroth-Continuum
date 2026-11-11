#include <array>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string_view>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MM7/MM7AtomicSaveFile.h"
#include "Engine/MM7/MM7SaveValidation.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"

namespace {

struct Delivery {
    ItemId item;
    QuestBit active;
    QuestBit given;
};

constexpr std::array<Delivery, 6> deliveries = {{
    {ITEM_POTION_CURE_WOUNDS, QBIT_EMERALD_ISLAND_RED_POTION_ACTIVE, QBIT_EMERALD_ISLAND_RED_POTION_GIVEN},
    {ITEM_QUEST_SEASHELL, QBIT_EMERALD_ISLAND_SEASHELL_ACTIVE, QBIT_EMERALD_ISLAND_SEASHELL_GIVEN},
    {ITEM_LONGBOW, QBIT_EMERALD_ISLAND_LONGBOW_ACTIVE, QBIT_EMERALD_ISLAND_LONGBOW_GIVEN},
    {ITEM_QUEST_FLOOR_TILE_W_MOON_INSIGNIA, QBIT_EMERALD_ISLAND_FLOOR_TILE_ACTIVE, QBIT_EMERALD_ISLAND_FLOOR_TILE_GIVEN},
    {ITEM_QUEST_LUTE, QBIT_EMERALD_ISLAND_LUTE_ACTIVE, QBIT_EMERALD_ISLAND_LUTE_GIVEN},
    {ITEM_QUEST_WEALTHY_HAT, QBIT_EMERALD_ISLAND_HAT_ACTIVE, QBIT_EMERALD_ISLAND_HAT_GIVEN},
}};

constexpr std::string_view slot = "saves/save000.mm7";

}  // namespace

GAME_TEST(MM7CampaignChain, DISABLED_Chapter0) {
    // Fixtures: six held Scavenger items and party strength. Travel uses teleports to Markham's door and ship.
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    const auto profileRoot = mm7::startupNativeProfileRoot();
    ASSERT_TRUE(profileRoot);
    ASSERT_EQ(*profileRoot, std::filesystem::canonical(std::filesystem::path(ufs->displayPath(""))));
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    ASSERT_FALSE(ufs->exists(slot));

    game.startNewGame();
    for (Character &character : pParty->pCharacters) {
        character.inventory.equip(ITEM_SLOT_MAIN_HAND, Item(ITEM_CHAMPION_SWORD));
        character.setSkillValue(SKILL_SWORD, CombinedSkillValue(30, MASTERY_GRANDMASTER));
        character._stats[ATTRIBUTE_MIGHT] = 200;
        character._stats[ATTRIBUTE_ACCURACY] = 200;
        character.uLevel = 100;
    }
    auto tourOffTape = tapes.questBit(QBIT_EMERALD_ISLAND_MARGARET_TOUR_OFF);
    auto wonTape = tapes.questBit(QBIT_EMERALD_ISLAND_SCAVENGER_HUNT_WON);
    auto shipTape = tapes.questBit(QBIT_EMERALD_ISLAND_SHIP_OFFERED);
    auto escapedTape = tapes.questBit(QBIT_ESCAPED_EMERALD_ISLE);
    auto mapTape = tapes.map();
    test.startTaping();

    game.pressGuiButton("Game_Hireling1"); // Lady Margaret, the only hireling at the start.
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_NPC_DIALOGUE);
    game.pressGuiButton("NpcDialogue_Option2"); // Tour Off, or her tour plates take the Space presses at doors.
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    game.teleportTo(MAP_EMERALD_ISLAND, Vec3f(16154, 8560, 128), 180);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_EMERALD_ISLAND_MARKHAMS_HEADQUARTERS);
    game.pressGuiButton("House_Npc1");
    game.tick();
    for (const Delivery &delivery : deliveries) {
        ASSERT_TRUE(pParty->_questBits[delivery.active]);
        ASSERT_FALSE(pParty->_questBits[delivery.given]);
        pParty->setHoldingItem(Item(delivery.item));
        game.pressGuiButton("HouseNpcDialogue_Option1");
        game.tick();
        ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
        ASSERT_FALSE(pParty->_questBits[delivery.active]);
        ASSERT_TRUE(pParty->_questBits[delivery.given]);
    }
    game.pressGuiButton("HouseNpcDialogue_Option1");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_EMERALD_ISLAND_SCAVENGER_HUNT_WON]);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    game.pressGuiButton("House_Npc0");
    game.tick();
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_EMERALD_ISLAND_SHIP_OFFERED]);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    game.teleportTo(MAP_EMERALD_ISLAND, Vec3f(11008, 34, 193), 90);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_HARMONDALE);
    ASSERT_TRUE(pParty->_questBits[QBIT_ESCAPED_EMERALD_ISLE]);
    EXPECT_EQ(tourOffTape, tape(false, true));
    EXPECT_EQ(wonTape, tape(false, true));
    EXPECT_EQ(shipTape, tape(false, true));
    EXPECT_EQ(escapedTape, tape(false, true));
    EXPECT_EQ(mapTape.back(), MAP_HARMONDALE);
    ASSERT_FALSE(::testing::Test::HasFailure());

    game.runGameRoutine([] {
        saveGame(false, false, slot, "Campaign C0");
    });
    ASSERT_TRUE(ufs->exists(slot));
    const Blob saved = ufs->read(slot);
    ASSERT_GT(saved.size(), 1024u);
    ASSERT_TRUE(mm7::preflightNativeSaveStructure(
        std::span<const std::uint8_t>(static_cast<const std::uint8_t *>(saved.data()), saved.size()),
        GameTitle::MM7, *pGames_LOD));
}
