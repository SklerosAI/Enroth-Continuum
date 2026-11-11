#include <algorithm>
#include <array>
#include <iterator>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Data/AwardEnums.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"
#include "Utility/Exception.h"

#include "MM7CampaignChain.h"

static void talkToStoneCityKing(EngineController &game) {
    auto throneDoor = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) { return face.eventId == 416; });
    ASSERT_NE(throneDoor, pIndoor->faces.end());
    auto [thronePosition, throneYaw] = approachCampaignChainFace(*throneDoor);
    game.teleportTo(MAP_STONE_CITY, thronePosition, throneYaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_STONE_CITY_THRONE_ROOM);
    auto king = std::ranges::find_if(houseNpcs, [](const HouseNpcDesc &npc) { return npc.npc == &pNPCStats->pNPCData[59]; });
    ASSERT_NE(king, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", std::distance(houseNpcs.begin(), king)));
        game.tick();
    }
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
}

GAME_TEST(MM7CampaignChain, DISABLED_Chapter2) {
    // Fixtures: teleports to the Stone City entrance and into the Red Dwarf Mines, and to each statue in the mines.
    assertCampaignChainProfile();
    ASSERT_FALSE(ufs->exists("saves/save002.mm7"));
    loadCampaignChainSlot(game, 1);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_HARMONDALE);
    ASSERT_TRUE(pParty->_questBits[QBIT_CASTLE_HARMONDALE_REPAIR_ACTIVE]);
    ASSERT_FALSE(pParty->_questBits[QBIT_HARMONDALE_REBUILT]);

    auto rescueTape = tapes.questBit(QBIT_RED_DWARF_MINES_RESCUE_ACTIVE);
    auto repairTape = tapes.questBit(QBIT_CASTLE_HARMONDALE_REPAIR_ACTIVE);
    auto oracleTape = tapes.questBit(QBIT_ORACLE_RECOVERS_ELIXIR);
    auto rebuiltTape = tapes.questBit(QBIT_HARMONDALE_REBUILT);
    auto timerTape = tapes.questBit(QBIT_CASTLE_HARMONDALE_TIMER_STARTED);
    auto awardTape = tapes.custom([] { return static_cast<bool>(pParty->pCharacters[0]._achievedAwardsBits[AWARD_BARROW_DOWNS_RESCUED_MINERS]); });
    auto elixirTape = tapes.custom([] { return pParty->pPickedItem.itemId == ITEM_QUEST_ELIXIR || pParty->hasItem(ITEM_QUEST_ELIXIR); });
    auto goldTape = tapes.gold();
    auto experienceTape = tapes.totalExperience();
    auto envoysTape = tapes.custom([] { return std::pair(pNPCStats->pNPCData[67].house, pNPCStats->pNPCData[68].house); });
    auto rescuedTape = tapes.custom([] {
        int mask = 0;
        for (int npc = 60; npc <= 66; npc++)
            if (pNPCStats->pNPCData[npc].Hired())
                mask |= 1 << (npc - 60);
        return mask;
    });
    auto mapTape = tapes.map();
    auto houseTape = tapes.house();
    test.startTaping();

    game.teleportTo(MAP_STONE_CITY, Vec3f(-3100, 2176, -64), 270); // The Stone City entrance.
    talkToStoneCityKing(game);
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_QUEST_ELIXIR);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);

    game.teleportTo(MAP_RED_DWARF_MINES, Vec3f(), 0);
    for (int eventId = 376; eventId <= 382; eventId++) {
        auto statue = std::ranges::find_if(pLevelDecorations, [eventId](const LevelDecoration &decoration) {
            return decoration.uEventID == eventId;
        });
        ASSERT_NE(statue, pLevelDecorations.end());
        const int decorationId = std::distance(pLevelDecorations.begin(), statue);
        bool pointed = false;
        for (const auto &[offset, yaw] : std::array{
                 std::pair{Vec3f(300, 0, 0), 180}, std::pair{Vec3f(-300, 0, 0), 0},
                 std::pair{Vec3f(0, 300, 0), 270}, std::pair{Vec3f(0, -300, 0), 90}}) {
            game.teleportTo(MAP_RED_DWARF_MINES, statue->vPosition + offset, yaw);
            try {
                game.pointMouseAtDecoration(decorationId);
                pointed = true;
                break;
            } catch (const Exception &) {
            }
        }
        ASSERT_TRUE(pointed);
        game.pressAndReleaseButton(BUTTON_LEFT);
        game.tick();
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE); // The rescued dwarf speaks.
        game.tick();
    }

    game.teleportTo(MAP_STONE_CITY, Vec3f(-3100, 2176, -64), 270);
    talkToStoneCityKing(game);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();

    EXPECT_EQ(rescueTape, tape(false, true, false));
    EXPECT_EQ(repairTape, tape(true, false));
    EXPECT_EQ(oracleTape, tape(false, true, false));
    EXPECT_EQ(rebuiltTape, tape(false, true));
    EXPECT_EQ(timerTape, tape(false));
    EXPECT_EQ(awardTape, tape(false, true));
    EXPECT_EQ(elixirTape, tape(false, true, false));
    EXPECT_EQ(goldTape.delta(), 5000);
    EXPECT_EQ(experienceTape.delta(), 4 * 12500);
    EXPECT_EQ(envoysTape.back(), std::pair(HOUSE_CASTLE_HARMONDALE_THRONE_ROOM, HOUSE_CASTLE_HARMONDALE_THRONE_ROOM));
    EXPECT_EQ(rescuedTape, tape(0, 2, 6, 14, 30, 62, 126, 127, 0));
    EXPECT_EQ(mapTape, tape(MAP_HARMONDALE, MAP_STONE_CITY, MAP_RED_DWARF_MINES, MAP_STONE_CITY));
    EXPECT_EQ(houseTape, tape(HOUSE_INVALID, HOUSE_STONE_CITY_THRONE_ROOM, HOUSE_INVALID, HOUSE_STONE_CITY_THRONE_ROOM, HOUSE_INVALID));
    ASSERT_FALSE(::testing::Test::HasFailure());

    saveCampaignChainSlot(game, 2);
}
