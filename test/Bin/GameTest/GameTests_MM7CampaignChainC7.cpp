#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <numbers>
#include <optional>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UICharacter.h"
#include "Io/Mouse.h"
#include "Utility/Exception.h"

#include "MM7CampaignChain.h"

static bool partyHasOverthruster() {
    return pParty->hasItem(ITEM_QUEST_OSCILLATION_OVERTHRUSTER) || pParty->pPickedItem.itemId == ITEM_QUEST_OSCILLATION_OVERTHRUSTER;
}

static int64_t autosaveWriteTime() {
    return std::filesystem::last_write_time(std::filesystem::path(ufs->displayPath("saves/autosave.mm7"))).time_since_epoch().count();
}

static Pointi inventoryCellPoint(Pointi cell) {
    return Pointi(14 + 32 * cell.x + 16, 17 + 32 * cell.y + 16);
}

static void putHeldItemInBag(EngineController &game, int characterIndex) {
    std::optional<Pointi> space = pParty->pCharacters[characterIndex].inventory.findSpace(pParty->pPickedItem);
    ASSERT_TRUE(space);
    game.pressAndReleaseButton(BUTTON_LEFT, inventoryCellPoint(*space) - Pointi(16, 16) - mouse->pickedItemOffset); // The bag snaps a held item by its center.
    game.tick();
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
}

static void takeOffEquipment(EngineController &game, int characterIndex) {
    // A wetsuit goes on only when nothing but blasters is worn, so everything else goes into the bag first.
    for (int i = 0; i < 16 && !pParty->pCharacters[characterIndex].NothingOrJustBlastersEquipped(); i++) {
        InventoryEntry entry = *pParty->pCharacters[characterIndex].inventory.equipment().begin();
        const int index = entry.index();
        const ItemId itemId = entry->itemId;
        std::optional<Pointi> point;
        for (int y = 2; y < 345 && !point; y += 4) // The paperdoll is drawn into a hit map of equipment indices.
            for (int x = 478; x < 640 && !point; x += 4)
                if (equipmentHitMap.query(Pointi(x, y), -1) == index)
                    point = Pointi(x, y);
        ASSERT_TRUE(point);
        game.pressAndReleaseButton(BUTTON_LEFT, *point);
        game.tick();
        ASSERT_EQ(pParty->pPickedItem.itemId, itemId);
        putHeldItemInBag(game, characterIndex);
    }
}

static std::optional<Pointi> spareWetsuitCell(int characterIndex) {
    for (InventoryConstEntry entry : pParty->pCharacters[characterIndex].inventory.entries())
        if (entry->itemId == ITEM_QUEST_WETSUIT && entry.zone() == INVENTORY_ZONE_GRID)
            return entry.geometry().topLeft();
    return std::nullopt;
}

static void equipWetsuitThroughInventory(EngineController &game, int characterIndex) {
    int holder = characterIndex;
    for (int i = 0; i < 4 && !spareWetsuitCell(holder); i++)
        holder = i;
    ASSERT_TRUE(spareWetsuitCell(holder));
    game.goToInventory(holder);
    if (holder != characterIndex) {
        game.pressAndReleaseButton(BUTTON_LEFT, inventoryCellPoint(*spareWetsuitCell(holder)));
        game.tick();
        ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_QUEST_WETSUIT);
        game.pressGuiButton(fmt::format("Game_Character{}", characterIndex + 1)); // A held item goes into the bag of the clicked portrait.
        game.tick();
        ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
        game.pressGuiButton(fmt::format("Game_Character{}", characterIndex + 1));
        game.tick();
        ASSERT_EQ(pParty->activeCharacterIndex(), characterIndex);
    }
    takeOffEquipment(game, characterIndex);
    ASSERT_TRUE(spareWetsuitCell(characterIndex));
    game.pressAndReleaseButton(BUTTON_LEFT, inventoryCellPoint(*spareWetsuitCell(characterIndex)));
    game.tick();
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_QUEST_WETSUIT);
    game.pressAndReleaseButton(BUTTON_LEFT, 558, 160); // The paperdoll.
    game.tick();
    ASSERT_TRUE(pParty->pCharacters[characterIndex].hasUnderwaterSuitEquipped());
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

template<class Done>
static bool interactWithLincolnEvent(EngineController &game, int eventId, Done done) {
    // Clicks or presses Space at a Lincoln event face from every safe spot around it, until `done`.
    auto safeFloor = [](Vec3f position) {
        int sector = pIndoor->GetSector(position);
        int floorFace = -1;
        return sector != 0 && BLV_GetFloorLevel(position + Vec3f(0, 0, pParty->radius), sector, &floorFace) != -30000 && floorFace != -1;
    };
    for (const BLVFace &face : pIndoor->faces) {
        if (face.eventId != eventId || !face.Clickable())
            continue;
        const int faceIndex = &face - pIndoor->faces.data();
        if (std::abs(face.facePlane.normal.z) > 0.5f) {
            for (const auto &[offset, yaw] : std::array{
                     std::pair{Vec3f(300, 0, 50), 180}, std::pair{Vec3f(-300, 0, 50), 0},
                     std::pair{Vec3f(0, 300, 50), 270}, std::pair{Vec3f(0, -300, 50), 90}}) {
                Vec3f position = face.boundingBox.center() + offset;
                if (!safeFloor(position))
                    continue;
                game.teleportTo(MAP_LINCOLN, position, yaw, -45);
                try {
                    game.pointMouseAtFace(faceIndex);
                } catch (const Exception &) {
                    continue;
                }
                game.pressAndReleaseButton(BUTTON_LEFT);
                game.tick();
                if (done())
                    return true;
            }
        }
        for (float distance : {150.0f, 300.0f, -150.0f, -300.0f}) {
            for (float height : {face.boundingBox.z1, face.boundingBox.center().z}) {
                Vec3f position = face.boundingBox.center() + distance * face.facePlane.normal;
                position.z = height;
                if (!safeFloor(position))
                    continue;
                int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) * 180 / std::numbers::pi);
                if (distance < 0)
                    yaw += 180;
                game.teleportTo(MAP_LINCOLN, position, yaw);
                game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
                game.tick();
                if (done())
                    return true;
                try {
                    game.pointMouseAtFace(faceIndex);
                } catch (const Exception &) {
                    continue;
                }
                game.pressAndReleaseButton(BUTTON_LEFT);
                game.tick();
                if (done())
                    return true;
            }
        }
    }
    return false;
}

GAME_TEST(MM7CampaignChain, DISABLED_Chapter7) {
    // Fixtures: teleports to the wetsuit chests in Castle Lambent, to the Avlee shore, to the Shoals entrance of
    // Lincoln, to the Lincoln console, Overthruster and exit, and to the Castle Lambent throne room door.
    assertCampaignChainProfile();
    ASSERT_FALSE(ufs->exists("saves/save007.mm7"));
    loadCampaignChainSlot(game, 6);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_CASTLE_LAMBENT);
    ASSERT_TRUE(pParty->_questBits[QBIT_OSCILLATION_OVERTHRUSTER_LIGHT_ACTIVE]);
    ASSERT_FALSE(ufs->exists("MM7_Win.Pcx"));

    auto wetsuitTape = tapes.questBit(QBIT_ORACLE_RECOVERS_WETSUIT);
    auto suitedTape = tapes.custom([] {
        return std::array{pParty->pCharacters[0].hasUnderwaterSuitEquipped(), pParty->pCharacters[1].hasUnderwaterSuitEquipped(),
                          pParty->pCharacters[2].hasUnderwaterSuitEquipped(), pParty->pCharacters[3].hasUnderwaterSuitEquipped()};
    });
    auto takenTape = tapes.questBit(QBIT_OSCILLATION_OVERTHRUSTER_TAKEN);
    auto oracleTape = tapes.questBit(QBIT_ORACLE_RECOVERS_OSCILLATION_OVERTHRUSTER);
    auto overthrusterTape = tapes.custom([] { return partyHasOverthruster(); });
    auto activeTape = tapes.questBit(QBIT_OSCILLATION_OVERTHRUSTER_LIGHT_ACTIVE);
    auto topicTape = tapes.custom([] { return pNPCStats->pNPCData[80].dialogue_2_evt_id; });
    auto certificateTape = tapes.custom([] { return ufs->exists("MM7_Win.Pcx"); });
    auto screenTape = tapes.screen();
    auto mapTape = tapes.map();
    test.startTaping();

    for (int eventId : {182, 183}) {
        ASSERT_TRUE(clickCampaignChainEventFace(game, MAP_CASTLE_LAMBENT, eventId, 1, [] { return current_screen_type == SCREEN_CHEST; }));
        for (int i = 0; i < 8; i++) { // Space takes one item.
            game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
            game.tick();
        }
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
        ASSERT_EQ(current_screen_type, SCREEN_GAME);
    }
    for (int i = 0; i < 4; i++)
        equipWetsuitThroughInventory(game, i);

    game.teleportTo(MAP_AVLEE, Vec3f(-22200, 0, 1), 180); // On the shore, facing the sea to the Shoals.
    game.pressKey(PlatformKey::KEY_UP);
    for (int i = 0; i < 20 && current_screen_type != SCREEN_CHANGE_LOCATION; i++) // The party walks into the sea border.
        game.tick();
    game.releaseKey(PlatformKey::KEY_UP);
    ASSERT_EQ(current_screen_type, SCREEN_CHANGE_LOCATION);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_SHOALS);

    auto lincoln = std::ranges::find_if(pOutdoor->faces, [](const BLVFace &face) { return face.eventId == 501 && face.Clickable(); });
    ASSERT_NE(lincoln, pOutdoor->faces.end());
    auto [lincolnPosition, lincolnYaw] = approachCampaignChainFace(*lincoln);
    game.teleportTo(MAP_SHOALS, lincolnPosition, lincolnYaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_LINCOLN);

    ASSERT_TRUE(interactWithLincolnEvent(game, 475, [] { return engine->_persistentVariables.mapVars[2] == 1; })); // The console powers the Overthruster.
    ASSERT_TRUE(interactWithLincolnEvent(game, 376, [] { return pParty->_questBits[QBIT_OSCILLATION_OVERTHRUSTER_TAKEN]; }));
    ASSERT_TRUE(interactWithLincolnEvent(game, 501, [] { return current_screen_type == SCREEN_INPUT_BLV; }));
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_SHOALS);

    openCampaignChainThroneRoom(game, 80);
    auto autosaveTape = tapes.custom([] { return autosaveWriteTime(); });
    ASSERT_TRUE(selectCampaignChainNpcEvent(game, 80, 170));
    game.tick(); // The game over window opens on the frame after the topic.
    ASSERT_EQ(current_screen_type, SCREEN_GAMEOVER_WINDOW);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE); // Shows the farewell box.
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick(2); // The window closes, then the party is sent to Harmondale.
    ufs->remove("MM7_Win.Pcx"); // The chain runner accepts nothing in the profile root but the marker, the saves and the ini.

    EXPECT_EQ(wetsuitTape, tape(false, true, false));
    EXPECT_EQ(suitedTape, tape(std::array{false, false, false, false}, std::array{true, false, false, false},
                               std::array{true, true, false, false}, std::array{true, true, true, false},
                               std::array{true, true, true, true}));
    EXPECT_EQ(takenTape, tape(false, true));
    EXPECT_EQ(oracleTape, tape(false, true, false));
    EXPECT_EQ(overthrusterTape, tape(false, true, false));
    EXPECT_EQ(activeTape, tape(true));
    EXPECT_EQ(topicTape, tape(170u, 0u));
    EXPECT_EQ(certificateTape, tape(false, true));
    EXPECT_EQ(autosaveTape.size(), 2); // Closing the game over window autosaves.
    EXPECT_CONTAINS(screenTape, SCREEN_GAMEOVER_WINDOW);
    EXPECT_EQ(screenTape.back(), SCREEN_GAME);
    EXPECT_EQ(mapTape, tape(MAP_CASTLE_LAMBENT, MAP_AVLEE, MAP_SHOALS, MAP_LINCOLN, MAP_SHOALS, MAP_CASTLE_LAMBENT, MAP_HARMONDALE));
    EXPECT_EQ(pParty->pos, Vec3f(-17331, 12547, 465));
    ASSERT_FALSE(::testing::Test::HasFailure());

    saveCampaignChainSlot(game, 7);
}
