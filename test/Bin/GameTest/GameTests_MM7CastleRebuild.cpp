#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <numbers>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Data/AwardEnums.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"
#include "Utility/Exception.h"

static std::pair<Vec3f, int> approachFace(const BLVFace &face) {
    Vec3f position = face.boundingBox.center() + 150 * face.facePlane.normal;
    position.z = face.boundingBox.z1;
    int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) * 180 / std::numbers::pi);
    return {position, yaw};
}

GAME_TEST(MM7CastleRebuild, Rescue) {
    // The Stone City king rebuilds Castle Harmondale only after all seven rescued dwarves are present at hand-in.
    for (bool missing : {false, true}) {
        SCOPED_TRACE(fmt::format("missing={}", missing));
        test.prepareForNextTest();
        game.startNewGame();
        engine->config->debug.NoDamage.setValue(true);
        game.teleportTo(MAP_HARMONDALE, Vec3f(), 0);
        auto castleDoor = std::ranges::find_if(pOutdoor->faces, [](const BLVFace &face) { return face.eventId == 301; });
        ASSERT_NE(castleDoor, pOutdoor->faces.end());
        auto [castlePosition, castleYaw] = approachFace(*castleDoor);
        game.teleportTo(MAP_HARMONDALE, castlePosition, castleYaw);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        game.pressGuiButton("Transition_Yes");
        game.skipLoadingScreen();
        ASSERT_EQ(engine->_currentLoadedMapId, MAP_CASTLE_HARMONDALE);
        for (Actor &actor : pActors)
            if (actor.group == 5)
                actor.aiState = Dead;
        game.teleportTo(MAP_CASTLE_HARMONDALE, pParty->pos, 180);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        game.pressGuiButton("Transition_Yes");
        game.skipLoadingScreen();
        ASSERT_TRUE(pParty->_questBits[QBIT_CASTLE_HARMONDALE_GOBLINS_KILLED]);
        auto tavernDoor = std::ranges::find_if(pOutdoor->faces, [](const BLVFace &face) { return face.eventId == 15; });
        ASSERT_NE(tavernDoor, pOutdoor->faces.end());
        auto [tavernPosition, tavernYaw] = approachFace(*tavernDoor);
        game.teleportTo(MAP_HARMONDALE, tavernPosition, tavernYaw);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
        auto butler = std::ranges::find_if(houseNpcs, [](const HouseNpcDesc &npc) { return npc.npc == &pNPCStats->pNPCData[58]; });
        ASSERT_NE(butler, houseNpcs.end());
        game.pressGuiButton(fmt::format("House_Npc{}", std::distance(houseNpcs.begin(), butler)));
        game.tick();
        game.pressGuiButton("HouseNpcDialogue_Option0");
        game.tick();
        ASSERT_TRUE(pParty->_questBits[QBIT_CASTLE_HARMONDALE_REPAIR_ACTIVE]);
        ASSERT_TRUE(pParty->pCharacters[0]._achievedAwardsBits[AWARD_HARMONDALE_CASTLE_CLEARED]);
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
        ASSERT_EQ(current_screen_type, SCREEN_GAME);

        auto rescueTape = tapes.questBit(QBIT_RED_DWARF_MINES_RESCUE_ACTIVE);
        auto repairTape = tapes.questBit(QBIT_CASTLE_HARMONDALE_REPAIR_ACTIVE);
        auto oracleTape = tapes.questBit(QBIT_ORACLE_RECOVERS_ELIXIR);
        auto rebuiltTape = tapes.questBit(QBIT_HARMONDALE_REBUILT);
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
        game.teleportTo(MAP_STONE_CITY, Vec3f(-3100, 2176, -64), 270);
        auto throneDoor = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) { return face.eventId == 416; });
        ASSERT_NE(throneDoor, pIndoor->faces.end());
        auto [thronePosition, throneYaw] = approachFace(*throneDoor);
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
        ASSERT_TRUE(pParty->_questBits[QBIT_RED_DWARF_MINES_RESCUE_ACTIVE]);
        ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_QUEST_ELIXIR);
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
        ASSERT_EQ(current_screen_type, SCREEN_GAME);
        game.teleportTo(MAP_RED_DWARF_MINES, Vec3f(), 0);
        ASSERT_EQ(engine->_currentLoadedMapId, MAP_RED_DWARF_MINES);
        for (int eventId = 376; eventId <= 382; eventId++) {
            if (missing && eventId == 382)
                continue;
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
            int npcId = eventId == 382 ? 60 : eventId - 315;
            ASSERT_TRUE(pNPCStats->pNPCData[npcId].Hired());
            game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
            game.tick();
        }
        ASSERT_EQ(rescuedTape.back(), missing ? 126 : 127);
        game.teleportTo(MAP_STONE_CITY, Vec3f(-3100, 2176, -64), 270);
        auto returnDoor = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) { return face.eventId == 416; });
        ASSERT_NE(returnDoor, pIndoor->faces.end());
        auto [returnPosition, returnYaw] = approachFace(*returnDoor);
        game.teleportTo(MAP_STONE_CITY, returnPosition, returnYaw);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_STONE_CITY_THRONE_ROOM);
        game.pressGuiButton("HouseNpcDialogue_Option0");
        game.tick();

        EXPECT_EQ(rescueTape, missing ? tape(false, true) : tape(false, true, false));
        EXPECT_EQ(repairTape, tape(true, false));
        EXPECT_EQ(oracleTape, missing ? tape(false, true) : tape(false, true, false));
        EXPECT_EQ(rebuiltTape, missing ? tape(false) : tape(false, true));
        EXPECT_EQ(awardTape, missing ? tape(false) : tape(false, true));
        EXPECT_EQ(elixirTape, missing ? tape(false, true) : tape(false, true, false));
        EXPECT_EQ(goldTape.delta(), missing ? 0 : 5000);
        EXPECT_EQ(experienceTape.delta(), missing ? 0 : 4 * 12500);
        EXPECT_EQ(envoysTape.size(), missing ? 1 : 2);
        if (!missing)
            EXPECT_EQ(envoysTape.back(), std::pair(HOUSE_CASTLE_HARMONDALE_THRONE_ROOM, HOUSE_CASTLE_HARMONDALE_THRONE_ROOM));
        EXPECT_EQ(rescuedTape, missing ? tape(0, 2, 6, 14, 30, 62, 126) : tape(0, 2, 6, 14, 30, 62, 126, 127, 0));
        EXPECT_EQ(mapTape, tape(MAP_HARMONDALE, MAP_STONE_CITY, MAP_RED_DWARF_MINES, MAP_STONE_CITY));
        EXPECT_EQ(houseTape, tape(HOUSE_INVALID, HOUSE_STONE_CITY_THRONE_ROOM, HOUSE_INVALID, HOUSE_STONE_CITY_THRONE_ROOM));
    }
}
