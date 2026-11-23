#include "MM6CampaignChain.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <numbers>
#include <string>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Application/GameConfig.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Graphics/Vis.h"
#include "Engine/MM6/MM6FirstMapManualSave.h"
#include "Engine/Localization.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Tables/ChestTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIButton.h"
#include "GUI/GUIDialogues.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/Houses/MM6House.h"
#include "GUI/UI/UIBranchlessDialogue.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/UISaveLoad.h"
#include "Library/Platform/Interface/PlatformEnumFunctions.h"

#include "Utility/Exception.h"

static std::filesystem::path profileRoot;

static std::string slotFileName(int chapter) {
    return fmt::format("save{:03}.ec6", chapter);
}

static void tickUntil(EngineController &game, const std::function<bool()> &condition) {
    for (int i = 0; i < 10 && !condition(); i++)
        game.tick();
}

static void clickArrow(EngineController &game, UIMessageType arrow, int times) {
    auto button = std::ranges::find(pGUIWindow_CurrentMenu->vButtons, arrow, &GUIButton::msg);
    ASSERT_NE(button, pGUIWindow_CurrentMenu->vButtons.end());
    for (int i = 0; i < times; i++) {
        game.pressAndReleaseButton(BUTTON_LEFT, (*button)->rect.center());
        game.tick();
    }
}

static std::vector<std::pair<Vec3f, int>> placesNextTo(Vec3f target) {
    // Floor positions 150 units to each side of the target, and the yaws that face the target.
    std::vector<std::pair<Vec3f, int>> result;
    for (Vec3f offset : {Vec3f(0, -150, 0), Vec3f(-150, 0, 0), Vec3f(0, 150, 0), Vec3f(150, 0, 0)}) {
        int sectorId = 0;
        Vec3f position = target + offset;
        position.z = GetIndoorFloorZ(position + Vec3f(0, 0, 40), &sectorId);
        if (position.z != -30000)
            result.emplace_back(position, std::lround(std::atan2(-offset.y, -offset.x) * 180 / std::numbers::pi));
    }
    return result;
}

static void standNextTo(EngineController &game, Vec3f target, int pitch) {
    ASSERT_EQ(uCurrentlyLoadedLevelType, LEVEL_INDOOR);
    const auto places = placesNextTo(target);
    ASSERT_FALSE(places.empty());
    game.teleportTo(engine->_currentLoadedMapId, places.front().first, places.front().second, pitch);
}

void setMM6CampaignChainProfile(const std::filesystem::path &root) {
    mm6::testing::setNativeProfileRootForTest(root);
    profileRoot = root;
}

const std::filesystem::path &mm6CampaignChainProfile() {
    return profileRoot;
}

void assertMM6CampaignChainProfile() {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_FALSE(profileRoot.empty());
    ASSERT_EQ(profileRoot, std::filesystem::canonical(profileRoot));
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
}

void loadMM6CampaignChainSlot(EngineController &game, int chapter) {
    const std::string fileName = slotFileName(chapter);
    ASSERT_TRUE(std::filesystem::is_regular_file(profileRoot / "saves" / fileName));
    game.pressGuiButton("MainMenu_LoadGame");
    tickUntil(game, [] { return pGUIWindow_CurrentMenu && pGUIWindow_CurrentMenu->eWindowType == WINDOW_LOAD; });
    ASSERT_EQ(current_screen_type, SCREEN_LOADGAME);
    const auto &slots = saveLoadMenu()->slots();
    const auto found = std::ranges::find(slots, fileName, &SavegameSlot::fileName);
    ASSERT_NE(found, slots.end());
    const int index = found - slots.begin();
    const int scroll = std::max(0, index - 6); // The Load screen shows seven slots.
    clickArrow(game, UIMSG_DownArrow, scroll);
    game.pressGuiButton(fmt::format("LoadMenu_Slot{}", index - scroll));
    game.tick();
    ASSERT_EQ(saveLoadMenu()->selectedSlot().fileName, fileName);
    game.pressGuiButton("LoadMenu_Load");
    game.skipLoadingScreen();
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    ASSERT_EQ(engine->_lastLoadedSaveFileName, fileName);
    ASSERT_TRUE(mm6::isManualSaveArmed());
}

void saveMM6CampaignChainSlot(EngineController &game, int chapter) {
    const std::string fileName = slotFileName(chapter);
    const std::string name = fmt::format("c{}", chapter);
    for (int earlier = 0; earlier < chapter; earlier++)
        ASSERT_TRUE(std::filesystem::is_regular_file(profileRoot / "saves" / slotFileName(earlier)));
    ASSERT_FALSE(std::filesystem::exists(profileRoot / "saves" / fileName));
    ASSERT_EQ(current_screen_type, SCREEN_GAME);

    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    tickUntil(game, [] { return current_screen_type == SCREEN_MENU; });
    ASSERT_EQ(current_screen_type, SCREEN_MENU);
    game.pressGuiButton("GameMenu_SaveGame");
    tickUntil(game, [] { return current_screen_type == SCREEN_SAVEGAME; });
    ASSERT_EQ(current_screen_type, SCREEN_SAVEGAME);
    const auto &slots = saveLoadMenu()->slots();
    ASSERT_EQ(std::ssize(slots), chapter + 1); // The new save slot, then the slots of the earlier chapters.
    ASSERT_TRUE(slots.front().fileName.empty());
    for (int i = 0; i < std::ssize(slots) && !saveLoadMenu()->selectedSlot().fileName.empty(); i++) {
        clickArrow(game, UIMSG_ArrowUp, 1); // The menu opens scrolled to the loaded slot.
        game.pressGuiButton("SaveMenu_Slot0");
        game.tick();
    }
    ASSERT_TRUE(saveLoadMenu()->selectedSlot().fileName.empty());
    game.doubleClickGuiButton("SaveMenu_Slot0");
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_C);
    game.tick();
    game.pressAndReleaseKey(platformKeyForDigit(chapter));
    game.tick();
    game.pressGuiButton("SaveMenu_Save");
    tickUntil(game, [] { return current_screen_type == SCREEN_GAME; });
    ASSERT_EQ(current_screen_type, SCREEN_GAME);

    ASSERT_TRUE(std::filesystem::is_regular_file(profileRoot / "saves" / fileName));
    const std::string mapFileName = pMapTable->pInfos[engine->_currentLoadedMapId].fileName;
    game.runGameRoutine([&] {
        const std::vector<mm6::SaveSlotInfo> listed = mm6::listFirstMapManualSlots();
        const auto slot = std::ranges::find(listed, fileName, &mm6::SaveSlotInfo::fileName);
        ASSERT_NE(slot, listed.end());
        EXPECT_EQ(slot->header.displayName, name);
        EXPECT_EQ(slot->header.mapFileName, mapFileName);
        EXPECT_NO_THROW((void) mm6::readFirstMapManualSlot(fileName));
    });
}

void useMM6CampaignChainEventFace(EngineController &game, MapId map, int eventId) {
    if (engine->_currentLoadedMapId != map)
        game.teleportTo(map, Vec3f(0, 0, 0), 0);
    ASSERT_EQ(engine->_currentLoadedMapId, map);
    auto face = std::ranges::find_if(pOutdoor->faces, [eventId](const BLVFace &face) { return face.eventId == eventId; });
    ASSERT_NE(face, pOutdoor->faces.end());
    Vec3f position = face->boundingBox.center() + 150 * face->facePlane.normal;
    position.z = face->boundingBox.z1;
    const int yaw = std::lround(std::atan2(-face->facePlane.normal.y, -face->facePlane.normal.x) * 180 / std::numbers::pi);
    game.teleportTo(map, position, yaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
}

void useMM6CampaignChainIndoorEventFace(EngineController &game, int eventId) {
    ASSERT_EQ(uCurrentlyLoadedLevelType, LEVEL_INDOOR);
    Vec3f position;
    auto face = std::ranges::find_if(pIndoor->faces, [&](const BLVFace &face) {
        if (face.eventId != eventId || std::abs(face.facePlane.normal.z) > 0.1f)
            return false;
        position = face.boundingBox.center() + 150 * face.facePlane.normal;
        int sectorId = 0;
        position.z = GetIndoorFloorZ(position, &sectorId);
        return sectorId == face.sectorId;
    });
    ASSERT_NE(face, pIndoor->faces.end());
    const int yaw = std::lround(std::atan2(-face->facePlane.normal.y, -face->facePlane.normal.x) * 180 / std::numbers::pi);
    game.teleportTo(engine->_currentLoadedMapId, position, yaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
}

void clickMM6CampaignChainIndoorEventFace(EngineController &game, int eventId) {
    ASSERT_EQ(uCurrentlyLoadedLevelType, LEVEL_INDOOR);
    for (size_t faceId = 0; faceId < pIndoor->faces.size(); faceId++) {
        const BLVFace &face = pIndoor->faces[faceId];
        if (face.eventId != eventId || std::abs(face.facePlane.normal.z) > 0.1f)
            continue;
        const Vec3f front = face.boundingBox.center() + 150 * face.facePlane.normal;
        int sectorId = 0;
        Vec3f position = front;
        position.z = GetIndoorFloorZ(front + Vec3f(0, 0, 40), &sectorId);
        if (position.z <= -29000)
            position.z = GetIndoorFloorZ(front - Vec3f(0, 0, 150), &sectorId); // A face above eye level.
        if (position.z <= -29000)
            continue;
        const int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) * 180 / std::numbers::pi);
        const int pitch = std::lround(std::atan2(face.boundingBox.center().z - position.z - pParty->eyeLevel, 150) * 180 / std::numbers::pi);
        game.teleportTo(engine->_currentLoadedMapId, position, yaw, pitch);
        game.tick();
        game.pointMouseAtFace(faceId);
        game.pressAndReleaseButton(BUTTON_LEFT);
        game.tick();
        return;
    }
    FAIL() << "No face of event " << eventId << " to click";
}

void walkOntoMM6CampaignChainIndoorEventPlate(EngineController &game, int eventId) {
    ASSERT_EQ(uCurrentlyLoadedLevelType, LEVEL_INDOOR);
    auto plate = std::ranges::find_if(pIndoor->faces, [&](const BLVFace &face) {
        return face.eventId == eventId && face.Pressure_Plate() && face.facePlane.normal.z > 0.9f;
    });
    ASSERT_NE(plate, pIndoor->faces.end());
    const auto onPlate = [eventId] { // An event can have several plates side by side.
        return pParty->floor_face_id >= 0 && pIndoor->faces[pParty->floor_face_id].eventId == eventId;
    };
    const Vec3f center = plate->boundingBox.center();
    const float reach = std::max(plate->boundingBox.x2 - plate->boundingBox.x1, plate->boundingBox.y2 - plate->boundingBox.y1) / 2 + 100;
    for (Vec3f direction : {Vec3f(0, -1, 0), Vec3f(-1, 0, 0), Vec3f(0, 1, 0), Vec3f(1, 0, 0)}) {
        Vec3f start = center + reach * direction;
        int sectorId = 0;
        start.z = GetIndoorFloorZ(start + Vec3f(0, 0, 40), &sectorId);
        if (start.z <= -29000)
            continue; // No floor there.
        game.teleportTo(engine->_currentLoadedMapId, start, std::lround(std::atan2(-direction.y, -direction.x) * 180 / std::numbers::pi));
        game.pressKey(PlatformKey::KEY_UP);
        for (int i = 0; i < 30 && !onPlate(); i++)
            game.tick();
        game.releaseKey(PlatformKey::KEY_UP);
        ASSERT_TRUE(onPlate());
        return;
    }
    FAIL() << "No floor next to the plate of event " << eventId;
}

void answerMM6CampaignChainQuestion(EngineController &game, std::string_view answer) {
    ASSERT_NE(pGUIWindow_BranchlessDialogue, nullptr);
    ASSERT_EQ(pGUIWindow_BranchlessDialogue->opcode(), EVENT_InputString);
    for (char c : answer) {
        if (c == ' ') {
            game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        } else if (c >= 'a' && c <= 'z') {
            game.pressAndReleaseKey(static_cast<PlatformKey>(std::to_underlying(PlatformKey::KEY_A) + c - 'a'));
        } else {
            ASSERT_TRUE(c >= '0' && c <= '9') << c;
            game.pressAndReleaseKey(static_cast<PlatformKey>(std::to_underlying(PlatformKey::KEY_DIGIT_0) + c - '0'));
        }
    }
    game.pressAndReleaseKey(PlatformKey::KEY_RETURN);
    game.tick(2); // The prompt takes the answer when it draws, and closes on the next frame.
}

void takeMM6CampaignChainChestItem(EngineController &game, int chestId, ItemId itemId) {
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    InventoryEntry item = vChests[chestId].inventory.find(itemId);
    ASSERT_TRUE(item);
    game.pressAndReleaseButton(BUTTON_LEFT, chestTable[vChests[chestId].chestTypeId].inventoryOffset + item.geometry().topLeft() * 32 + Pointi(16, 16));
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    game.pressGuiButton("Game_Character1"); // The held item goes into the first character's backpack.
    game.tick();
}

void killMM6CampaignChainActor(EngineController &game, int actorId) {
    for (int attack = 0; attack < 500 && !pActors[actorId].IsNotAlive(); attack++) {
        if (attack % 10 == 0)
            standNextTo(game, pActors[actorId].pos, 0); // The monster moves while it fights.
        try {
            game.pointMouseAtActor(actorId);
        } catch (const Exception &) {
            // The monster can be out of view for a frame, A still attacks then.
        }
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick();
    }
    ASSERT_TRUE(pActors[actorId].IsNotAlive());
}

void lootMM6CampaignChainCorpse(EngineController &game, int actorId) {
    ASSERT_EQ(uCurrentlyLoadedLevelType, LEVEL_INDOOR);
    for (int frame = 0; frame < 100 && pActors[actorId].aiState != Dead; frame++)
        game.tick(); // The monster falls.
    ASSERT_EQ(pActors[actorId].aiState, Dead);
    for (int round = 0; round < 5; round++) {
        int blocker = -1;
        for (auto [position, yaw] : placesNextTo(pActors[actorId].pos)) {
            game.teleportTo(engine->_currentLoadedMapId, position, yaw, -20);
            game.tick();
            const Pid target = engine->PickKeyboard(engine->config->gameplay.KeyboardInteractionDepth.value(), &vis_decoration_noevent_filter,
                                                    &vis_door_filter).pid;
            if (target == Pid::actor(actorId)) {
                game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
                game.tick();
                return;
            }
            if (target.type() == OBJECT_Actor && !pActors[target.id()].IsNotAlive())
                blocker = target.id(); // A living monster stands between the party and the corpse.
            const Pid caster = target.type() == OBJECT_Sprite ? pSpriteObjects[target.id()].spell_caster_pid : Pid();
            if (caster.type() == OBJECT_Actor && !pActors[caster.id()].IsNotAlive())
                blocker = caster.id(); // A monster's spell flies between the party and the corpse.
        }
        if (blocker < 0)
            break;
        killMM6CampaignChainActor(game, blocker);
    }
    FAIL() << "No side of the corpse where Space reaches it";
}

void pickUpMM6CampaignChainFloorItem(EngineController &game, ItemId itemId) {
    auto sprite = std::ranges::find_if(pSpriteObjects, [itemId](const SpriteObject &sprite) {
        return sprite.uObjectDescID && sprite.containing_item.itemId == itemId;
    });
    ASSERT_NE(sprite, pSpriteObjects.end());
    const int spriteId = sprite - pSpriteObjects.begin();
    standNextTo(game, pSpriteObjects[spriteId].vPosition, -20);
    game.pointMouseAtSprite(spriteId);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
}

void useMM6CampaignChainDecoration(EngineController &game, int decorationId) {
    standNextTo(game, pLevelDecorations[decorationId].vPosition, -20);
    game.tick(); // The party lands, and the decoration gets drawn.
    game.pointMouseAtDecoration(decorationId);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
}

void enterMM6CampaignChainCastle(EngineController &game, MapId map, int eventId) {
    useMM6CampaignChainEventFace(game, map, eventId);
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
}

void confirmMM6CampaignChainTransition(EngineController &game) {
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.tick();
    game.skipLoadingScreen();
    game.tick();
}

void enterMM6CampaignChainOracle(EngineController &game) {
    const MapId oracle = pMapTable->GetMapInfo("oracle.blv");
    useMM6CampaignChainEventFace(game, pMapTable->GetMapInfo("outc2.odm"), 49);
    takeMM6CampaignChainHouseExit(game, oracle);
}

void takeMM6CampaignChainHouseExit(EngineController &game, MapId map) {
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    auto button = std::ranges::find(window_SpeakInHouse->vButtons, localization->format(LSTR_ENTER_S, pMapTable->pInfos[map].name), &GUIButton::label);
    ASSERT_NE(button, window_SpeakInHouse->vButtons.end());
    game.pressGuiButton((*button)->id);
    game.tick();
    game.pressGuiButton("HouseExit_Yes");
    game.tick();
    game.skipLoadingScreen();
    game.tick();
    ASSERT_EQ(engine->_currentLoadedMapId, map);
}

void talkToMM6CampaignChainNpc(EngineController &game, int npcId) {
    ASSERT_NE(mm6HouseWindow(), nullptr);
    if (mm6HouseWindow()->selectedNpc() == &pNPCStats->pNPCData[npcId])
        return; // A house with one person opens straight into the conversation.
    const std::string label = localization->format(LSTR_CONVERSE_WITH_S, pNPCStats->pNPCData[npcId].name);
    auto button = std::ranges::find(window_SpeakInHouse->vButtons, label, &GUIButton::label);
    ASSERT_NE(button, window_SpeakInHouse->vButtons.end());
    game.pressGuiButton((*button)->id);
    game.tick();
    ASSERT_EQ(mm6HouseWindow()->selectedNpc(), &pNPCStats->pNPCData[npcId]);
}

bool selectMM6CampaignChainTopic(EngineController &game, int eventId) {
    const NPCData *npc = mm6HouseWindow() ? mm6HouseWindow()->selectedNpc() : nullptr;
    if (!npc)
        return false;
    const unsigned events[] = {npc->dialogue_1_evt_id, npc->dialogue_2_evt_id, npc->dialogue_3_evt_id,
                               npc->dialogue_4_evt_id, npc->dialogue_5_evt_id, npc->dialogue_6_evt_id};
    for (GUIButton *button : window_SpeakInHouse->vButtons) {
        const int slot = static_cast<int>(button->msg_param) - std::to_underlying(DIALOGUE_SCRIPTED_LINE_1);
        if (button->id.starts_with("HouseNpcDialogue_Option") && slot >= 0 && slot < std::ssize(events) &&
            events[slot] == static_cast<unsigned>(eventId)) {
            game.pressGuiButton(button->id);
            game.tick();
            return true;
        }
    }
    return false;
}

void leaveMM6CampaignChainHouse(EngineController &game) {
    for (int i = 0; i < 5 && current_screen_type != SCREEN_GAME; i++) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}
