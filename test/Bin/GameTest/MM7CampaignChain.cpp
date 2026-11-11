#include "MM7CampaignChain.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <numbers>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MM7/MM7AtomicSaveFile.h"
#include "Engine/MM7/MM7SaveValidation.h"
#include "Engine/Party.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/UISaveLoad.h"
#include "Utility/Exception.h"

std::pair<Vec3f, int> approachCampaignChainFace(const BLVFace &face) {
    Vec3f position = face.boundingBox.center() + 150 * face.facePlane.normal;
    position.z = face.boundingBox.z1;
    int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) * 180 / std::numbers::pi);
    return {position, yaw};
}

void assertCampaignChainProfile() {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    const auto profileRoot = mm7::startupNativeProfileRoot();
    ASSERT_TRUE(profileRoot);
    ASSERT_EQ(*profileRoot, std::filesystem::canonical(std::filesystem::path(ufs->displayPath(""))));
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
}

void loadCampaignChainSlot(EngineController &game, int chapter) {
    const std::string fileName = fmt::format("save{:03}.mm7", chapter);
    ASSERT_TRUE(ufs->exists("saves/" + fileName));
    game.pressGuiButton("MainMenu_LoadGame");
    auto loadWindowOpen = [] { return pGUIWindow_CurrentMenu && pGUIWindow_CurrentMenu->eWindowType == WINDOW_LOAD; };
    for (int i = 0; i < 10 && !loadWindowOpen(); i++)
        game.tick();
    ASSERT_TRUE(loadWindowOpen());
    ASSERT_EQ(current_screen_type, SCREEN_LOADGAME);
    const auto &slots = saveLoadMenu()->slots();
    const auto found = std::ranges::find_if(slots, [&fileName](const SavegameSlot &entry) { return entry.fileName == fileName; });
    ASSERT_NE(found, slots.end());
    const int index = static_cast<int>(found - slots.begin());
    ASSERT_LT(index, 7); // The Load screen shows seven slots before it scrolls.
    game.pressGuiButton(fmt::format("LoadMenu_Slot{}", index));
    game.tick();
    ASSERT_EQ(saveLoadMenu()->selectedSlot().fileName, fileName);
    game.pressGuiButton("LoadMenu_Load");
    game.skipLoadingScreen();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    ASSERT_EQ(engine->_lastLoadedSaveFileName, fileName);
}

void saveCampaignChainSlot(EngineController &game, int chapter) {
    const std::string path = fmt::format("saves/save{:03}.mm7", chapter);
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    game.runGameRoutine([&] {
        saveGame(false, false, path, fmt::format("Campaign C{}", chapter));
    });
    ASSERT_TRUE(ufs->exists(path));
    const Blob saved = ufs->read(path);
    ASSERT_GT(saved.size(), 1024u);
    ASSERT_TRUE(mm7::preflightNativeSaveStructure(
        std::span<const std::uint8_t>(static_cast<const std::uint8_t *>(saved.data()), saved.size()),
        GameTitle::MM7, *pGames_LOD));
}

bool clickCampaignChainEventFace(EngineController &game, MapId map, int eventId, int clicks, const std::function<bool()> &done) {
    for (const BLVFace &face : pIndoor->faces) {
        if (face.eventId != eventId || !face.Clickable())
            continue;
        const int faceId = &face - pIndoor->faces.data();
        for (float distance : {150.0f, 250.0f, 350.0f}) {
            for (auto [direction, yaw] : {std::pair(Vec3f(1, 0, 0), 180), std::pair(Vec3f(-1, 0, 0), 0),
                                          std::pair(Vec3f(0, 1, 0), 270), std::pair(Vec3f(0, -1, 0), 90)}) {
                Vec3f position = face.boundingBox.center() + distance * direction;
                int sector = pIndoor->GetSector(position);
                int floorFace = -1;
                float floor = sector ? BLV_GetFloorLevel(position, sector, &floorFace) : -30000;
                if (floorFace == -1 || floor > face.boundingBox.z2 || floor < face.boundingBox.z1 - 200)
                    continue;
                position.z = floor;
                for (int pitch : {-15, -30, -45}) {
                    game.teleportTo(map, position, yaw, pitch);
                    try {
                        game.pointMouseAtFace(faceId);
                    } catch (const Exception &) {
                        continue;
                    }
                    for (int i = 0; i < clicks && !done(); i++) {
                        game.pressAndReleaseButton(BUTTON_LEFT);
                        game.tick();
                    }
                    if (done())
                        return true;
                }
            }
        }
    }
    return false;
}

void lootCampaignChainChest(EngineController &game, MapId map, int eventId, ItemId itemId) {
    ASSERT_TRUE(clickCampaignChainEventFace(game, map, eventId, 2, [] { return current_screen_type == SCREEN_CHEST; })); // A trapped chest goes off on the first click.
    for (int i = 0; i < 40 && !pParty->hasItem(itemId) && pParty->pPickedItem.itemId == ITEM_NULL; i++) { // Space takes one item.
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
    }
    ASSERT_TRUE(pParty->hasItem(itemId));
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

void useCampaignChainTransition(EngineController &game, MapId map, int eventId) {
    auto door = std::ranges::find_if(pIndoor->faces, [eventId](const BLVFace &face) {
        return face.eventId == eventId && face.Clickable();
    });
    ASSERT_NE(door, pIndoor->faces.end());
    auto [position, yaw] = approachCampaignChainFace(*door);
    game.teleportTo(map, position, yaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
}

static bool opensHouse(int eventId, HouseId houseId) {
    if (!engine->_localEventMap.hasEvent(eventId))
        return false;
    return std::ranges::any_of(engine->_localEventMap.function(eventId), [houseId](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SpeakInHouse && instruction.data.house_id == houseId;
    });
}

static void pickHouseNpc(EngineController &game, HouseId houseId, int npcId) {
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), houseId);
    auto npc = std::ranges::find_if(houseNpcs, [npcId](const HouseNpcDesc &entry) {
        return entry.npc == &pNPCStats->pNPCData[npcId];
    });
    ASSERT_NE(npc, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", npc - houseNpcs.begin()));
        game.tick();
    }
}

void openCampaignChainCelesteHouse(EngineController &game, HouseId houseId, int npcId) {
    game.teleportTo(MAP_CELESTE, Vec3f(-158, 7624, 1), 0);
    auto door = std::ranges::find_if(pIndoor->faces, [houseId](const BLVFace &face) {
        return face.Clickable() && opensHouse(face.eventId, houseId);
    });
    ASSERT_NE(door, pIndoor->faces.end());
    auto [position, yaw] = approachCampaignChainFace(*door);
    game.teleportTo(MAP_CELESTE, position, yaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    pickHouseNpc(game, houseId, npcId);
}

void openCampaignChainThroneRoom(EngineController &game, int npcId) {
    game.teleportTo(MAP_CASTLE_LAMBENT, Vec3f(-3456, 12700, 1115), 90);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    pickHouseNpc(game, HOUSE_CASTLE_LAMBENT_THRONE_ROOM, npcId);
}

bool selectCampaignChainNpcEvent(EngineController &game, int npcId, int eventId) {
    NPCData &npc = pNPCStats->pNPCData[npcId];
    std::array eventIds = {npc.dialogue_1_evt_id, npc.dialogue_2_evt_id, npc.dialogue_3_evt_id,
                           npc.dialogue_4_evt_id, npc.dialogue_5_evt_id, npc.dialogue_6_evt_id};
    auto event = std::ranges::find(eventIds, static_cast<unsigned int>(eventId));
    if (event == eventIds.end())
        return false;
    DialogueId line = static_cast<DialogueId>(std::to_underlying(DIALOGUE_SCRIPTED_LINE_1) + (event - eventIds.begin()));
    std::vector<DialogueId> topics = prepareScriptedNPCDialogueTopics(&npc);
    auto option = std::ranges::find(topics, line);
    if (option == topics.end())
        return false;
    game.pressGuiButton(fmt::format("HouseNpcDialogue_Option{}", option - topics.begin()));
    game.tick();
    return true;
}

void leaveCampaignChainHouse(EngineController &game) {
    for (int i = 0; i < 4 && current_screen_type != SCREEN_GAME; i++) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}
