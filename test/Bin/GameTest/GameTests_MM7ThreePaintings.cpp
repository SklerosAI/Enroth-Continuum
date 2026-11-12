#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <numbers>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/HouseEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Graphics/Vis.h"
#include "Engine/MapEnums.h"
#include "Engine/MM7/MM7AtomicSaveFile.h"
#include "Engine/MM7/MM7SaveValidation.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/Pid.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/UISaveLoad.h"

#include "Utility/Exception.h"

namespace {

constexpr std::string_view checkpointSlot = "saves/save000.mm7";
constexpr std::string_view checkpointFileName = "save000.mm7";
constexpr std::array<uint64_t, 4> checkpointExperienceBaseline = {5000, 5100, 5200, 5300};
constexpr int checkpointGoldBaseline = 6000;

struct PaintingSource {
    MapId map;
    ItemId item;
    QuestBit takenBit;
};

constexpr std::array paintingSources = {
    PaintingSource{MAP_HAUNTED_MANSION, ITEM_QUEST_ANGEL_STATUE_PAINTING, QBIT_ANGEL_STATUE_PAINTING_TAKEN},
    PaintingSource{MAP_CASTLE_GRYPHONHEART, ITEM_QUEST_ARCHIBALD_IRONFIST_PAINTING, QBIT_ARCHIBALD_PAINTING_TAKEN},
    PaintingSource{MAP_CASTLE_GRYPHONHEART, ITEM_QUEST_ROLAND_IRONFIST_PAINTING, QBIT_ROLAND_PAINTING_TAKEN},
};

std::pair<Vec3f, int> approachFace(const BLVFace &face) {
    Vec3f position = face.boundingBox.center() + 150 * face.facePlane.normal;
    position.z = face.boundingBox.z1;
    int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) * 180 / std::numbers::pi);
    return {position, yaw};
}

bool eventOpensHouse(int eventId, HouseId houseId) {
    return engine->_localEventMap.hasEvent(eventId) &&
           std::ranges::any_of(engine->_localEventMap.function(eventId), [houseId](const EvtInstruction &instruction) {
               return instruction.opcode == EVENT_SpeakInHouse && instruction.data.house_id == houseId;
           });
}

bool eventSetsQuestBit(const EvtInstruction &instruction, QuestBit bit) {
    return (instruction.opcode == EVENT_Set || instruction.opcode == EVENT_Add) &&
           instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
           instruction.data.variable_descr.value == std::to_underlying(bit);
}

bool eventGrantsItem(const EvtProgram &program, int eventId, ItemId item) {
    if (!program.hasEvent(eventId))
        return false;
    return std::ranges::any_of(program.function(eventId), [item](const EvtInstruction &instruction) {
        return (instruction.opcode == EVENT_Set || instruction.opcode == EVENT_Add) &&
               instruction.data.variable_descr.type == VAR_PlayerItemInHands &&
               instruction.data.variable_descr.value == std::to_underlying(item);
    });
}

NPCData *findVisconti() {
    auto npc = std::ranges::find_if(pNPCStats->pNPCData, [](const NPCData &candidate) {
        std::array eventIds = {candidate.dialogue_1_evt_id, candidate.dialogue_2_evt_id, candidate.dialogue_3_evt_id,
                               candidate.dialogue_4_evt_id, candidate.dialogue_5_evt_id, candidate.dialogue_6_evt_id};
        return candidate.house == HOUSE_TATALIA_VISCONTI_RESIDENCE &&
               std::ranges::any_of(eventIds, [](unsigned int eventId) {
                   return eventId != 0 && engine->_globalEventMap.hasEvent(eventId) &&
                          std::ranges::any_of(engine->_globalEventMap.function(eventId), [](const EvtInstruction &instruction) {
                              return eventSetsQuestBit(instruction, QBIT_PAINTINGS_ACTIVE);
                          });
               });
    });
    EXPECT_NE(npc, pNPCStats->pNPCData.end());
    return npc == pNPCStats->pNPCData.end() ? nullptr : &*npc;
}

unsigned int findNpcEvent(const NPCData *npc, auto predicate) {
    std::array eventIds = {npc->dialogue_1_evt_id, npc->dialogue_2_evt_id, npc->dialogue_3_evt_id,
                           npc->dialogue_4_evt_id, npc->dialogue_5_evt_id, npc->dialogue_6_evt_id};
    auto event = std::ranges::find_if(eventIds, [predicate](unsigned int eventId) {
        return eventId != 0 && engine->_globalEventMap.hasEvent(eventId) &&
               std::ranges::any_of(engine->_globalEventMap.function(eventId), predicate);
    });
    EXPECT_NE(event, eventIds.end());
    return event == eventIds.end() ? 0 : *event;
}

bool selectNpcEvent(EngineController &game, NPCData *npc, unsigned int eventId) {
    std::array eventIds = {npc->dialogue_1_evt_id, npc->dialogue_2_evt_id, npc->dialogue_3_evt_id,
                           npc->dialogue_4_evt_id, npc->dialogue_5_evt_id, npc->dialogue_6_evt_id};
    auto event = std::ranges::find(eventIds, eventId);
    if (event == eventIds.end())
        return false;
    DialogueId line = static_cast<DialogueId>(std::to_underlying(DIALOGUE_SCRIPTED_LINE_1) + (event - eventIds.begin()));
    auto topics = prepareScriptedNPCDialogueTopics(npc);
    auto option = std::ranges::find(topics, line);
    if (option == topics.end())
        return false;
    game.pressGuiButton(fmt::format("HouseNpcDialogue_Option{}", option - topics.begin()));
    game.tick();
    return true;
}

void openViscontiHouse(EngineController &game, NPCData *visconti) {
    game.teleportTo(MAP_TATALIA, Vec3f(), 0);
    std::vector<std::pair<Vec3f, int>> doors;
    for (const BLVFace &face : pOutdoor->faces) {
        if (face.Clickable() && eventOpensHouse(face.eventId, HOUSE_TATALIA_VISCONTI_RESIDENCE))
            doors.push_back(approachFace(face));
    }
    ASSERT_FALSE(doors.empty());
    for (auto [position, yaw] : doors) {
        game.teleportTo(MAP_TATALIA, position, yaw);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        if (current_screen_type == SCREEN_HOUSE && window_SpeakInHouse->houseId() == HOUSE_TATALIA_VISCONTI_RESIDENCE)
            break;
        if (current_screen_type == SCREEN_NPC_DIALOGUE) {
            game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
            game.tick();
        }
    }
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_TATALIA_VISCONTI_RESIDENCE);
    auto entry = std::ranges::find(houseNpcs, visconti, &HouseNpcDesc::npc);
    ASSERT_NE(entry, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", entry - houseNpcs.begin()));
        game.tick();
    }
}

void leaveHouse(EngineController &game) {
    for (int i = 0; i < 4 && current_screen_type != SCREEN_GAME; ++i) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

bool clickPaintingFace(EngineController &game, const PaintingSource &source) {
    auto face = std::ranges::find_if(pIndoor->faces, [&source](const BLVFace &candidate) {
        if (!candidate.Clickable())
            return false;
        return eventGrantsItem(engine->_localEventMap, candidate.eventId, source.item) ||
               eventGrantsItem(engine->_globalEventMap, candidate.eventId, source.item);
    });
    EXPECT_NE(face, pIndoor->faces.end());
    if (face == pIndoor->faces.end())
        return false;
    const int faceId = face - pIndoor->faces.begin();
    auto safeFloor = [](Vec3f position) {
        const int sector = pIndoor->GetSector(position);
        int floorFace = -1;
        return sector != 0 && BLV_GetFloorLevel(position + Vec3f(0, 0, pParty->radius), sector, &floorFace) != -30000 &&
               floorFace != -1;
    };
    for (float distance : {150.0f, 300.0f, -150.0f, -300.0f}) {
        for (float height : {face->boundingBox.z1, face->boundingBox.center().z}) {
            Vec3f position = face->boundingBox.center() + distance * face->facePlane.normal;
            position.z = height;
            if (!safeFloor(position))
                continue;
            int yaw = std::lround(std::atan2(-face->facePlane.normal.y, -face->facePlane.normal.x) * 180 / std::numbers::pi);
            if (distance < 0)
                yaw += 180;
            for (int pitch : {0, -15, -30, -45}) {
                game.teleportTo(source.map, position, yaw, pitch);
                if (engine->PickKeyboard(engine->config->gameplay.KeyboardInteractionDepth.value(),
                                         &vis_decoration_noevent_filter, &vis_door_filter).pid == Pid::face(faceId)) {
                    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
                    game.tick();
                    if (pParty->pPickedItem.itemId == source.item || pParty->hasItem(source.item))
                        return true;
                }
                try {
                    game.pointMouseAtFace(faceId);
                    if (engine->PickMouseForInteraction().pid != Pid::face(faceId))
                        continue;
                } catch (const Exception &) {
                    continue;
                }
                game.pressAndReleaseButton(BUTTON_LEFT);
                game.tick();
                if (pParty->pPickedItem.itemId == source.item || pParty->hasItem(source.item))
                    return true;
            }
        }
    }
    for (float distance : {150.0f, 300.0f, 500.0f}) {
        for (auto [direction, yaw] : {std::pair(Vec3f(1, 0, 0), 180), std::pair(Vec3f(-1, 0, 0), 0),
                                      std::pair(Vec3f(0, 1, 0), 270), std::pair(Vec3f(0, -1, 0), 90)}) {
            Vec3f position = face->boundingBox.center() + distance * direction;
            const int sector = pIndoor->GetSector(position);
            int floorFace = -1;
            const float floor = sector ? BLV_GetFloorLevel(position, sector, &floorFace) : -30000;
            if (floorFace == -1)
                continue;
            position.z = floor;
            game.teleportTo(source.map, position, yaw, -30);
            try {
                game.pointMouseAtFace(faceId);
            } catch (const Exception &) {
                continue;
            }
            game.pressAndReleaseButton(BUTTON_LEFT);
            game.tick();
            if (pParty->pPickedItem.itemId == source.item || pParty->hasItem(source.item))
                return true;
        }
    }
    return false;
}

void acquirePainting(EngineController &game, const PaintingSource &source) {
    game.teleportTo(source.map, Vec3f(), 0);
    ASSERT_TRUE(clickPaintingFace(game, source));
    EXPECT_TRUE(pParty->_questBits[source.takenBit]);
    if (pParty->pPickedItem.itemId == source.item)
        pParty->placeHeldItemInInventoryOrDrop();
    ASSERT_TRUE(pParty->hasItem(source.item));
}

int totalExperience() {
    int result = 0;
    for (const Character &character : pParty->pCharacters)
        result += character.experience;
    return result;
}

void assertDisposableHeadlessProfile() {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    const auto profileRoot = mm7::startupNativeProfileRoot();
    ASSERT_TRUE(profileRoot);
    ASSERT_EQ(*profileRoot, std::filesystem::canonical(std::filesystem::path(ufs->displayPath(""))));
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
}

void runThreePaintingsRoute(EngineController &game, bool useCheckpointBaseline = false) {
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    if (useCheckpointBaseline) {
        pParty->SetGold(checkpointGoldBaseline);
        for (int i = 0; i < 4; ++i)
            pParty->pCharacters[i].experience = checkpointExperienceBaseline[i];
    }
    NPCData *visconti = findVisconti();
    ASSERT_NE(visconti, nullptr);
    const unsigned int offerEvent = findNpcEvent(visconti, [](const EvtInstruction &instruction) {
        return eventSetsQuestBit(instruction, QBIT_PAINTINGS_ACTIVE);
    });
    ASSERT_NE(offerEvent, 0u);
    const int gold = pParty->uNumGold;
    const int experience = totalExperience();

    openViscontiHouse(game, visconti);
    ASSERT_TRUE(selectNpcEvent(game, visconti, offerEvent));
    ASSERT_TRUE(pParty->_questBits[QBIT_PAINTINGS_ACTIVE]);
    const unsigned int handInEvent = findNpcEvent(visconti, [](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_Subtract && instruction.data.variable_descr.type == VAR_PlayerItemInHands &&
               instruction.data.variable_descr.value == std::to_underlying(ITEM_QUEST_ANGEL_STATUE_PAINTING);
    });
    ASSERT_NE(handInEvent, 0u);
    leaveHouse(game);

    openViscontiHouse(game, visconti);
    ASSERT_TRUE(selectNpcEvent(game, visconti, handInEvent));
    EXPECT_TRUE(pParty->_questBits[QBIT_PAINTINGS_ACTIVE]);
    EXPECT_EQ(pParty->uNumGold, gold);
    EXPECT_EQ(totalExperience(), experience);
    leaveHouse(game);

    for (const PaintingSource &source : paintingSources)
        acquirePainting(game, source);

    openViscontiHouse(game, visconti);
    ASSERT_TRUE(selectNpcEvent(game, visconti, handInEvent));
    EXPECT_FALSE(pParty->_questBits[QBIT_PAINTINGS_ACTIVE]);
    for (const PaintingSource &source : paintingSources) {
        EXPECT_TRUE(pParty->_questBits[source.takenBit]);
        EXPECT_FALSE(pParty->hasItem(source.item));
    }
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_EQ(pParty->uNumGold, gold + 50000);
    EXPECT_EQ(totalExperience(), experience + 4 * 50000);
}

} // namespace

GAME_TEST(MM7SideQuests, ThreePaintingsRequireAuthoredWallPickups) {
    runThreePaintingsRoute(game);
}

GAME_TEST(MM7SideQuestCheckpoint, DISABLED_WriteThreePaintings) {
    assertDisposableHeadlessProfile();
    ASSERT_FALSE(ufs->exists(checkpointSlot));
    runThreePaintingsRoute(game, true);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_FALSE(::testing::Test::HasFailure());
    leaveHouse(game);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_TATALIA);
    game.runGameRoutine([] {
        saveGame(false, false, checkpointSlot, "Three Paintings checkpoint");
    });

    ASSERT_TRUE(ufs->exists(checkpointSlot));
    const Blob saved = ufs->read(checkpointSlot);
    ASSERT_GT(saved.size(), 1024u);
    ASSERT_TRUE(mm7::preflightNativeSaveStructure(
        std::span<const std::uint8_t>(static_cast<const std::uint8_t *>(saved.data()), saved.size()),
        GameTitle::MM7, *pGames_LOD));
}

GAME_TEST(MM7SideQuestCheckpoint, DISABLED_ReloadThreePaintings) {
    assertDisposableHeadlessProfile();
    ASSERT_TRUE(ufs->exists(checkpointSlot));
    const Blob saved = Blob::copy(ufs->read(checkpointSlot));
    ASSERT_GT(saved.size(), 1024u);

    game.pressGuiButton("MainMenu_LoadGame");
    game.tick(3);
    ASSERT_EQ(current_screen_type, SCREEN_LOADGAME);
    const auto &slots = saveLoadMenu()->slots();
    const auto found = std::ranges::find(slots, checkpointFileName, &SavegameSlot::fileName);
    ASSERT_NE(found, slots.end());
    const int index = static_cast<int>(found - slots.begin());
    ASSERT_LT(index, 7);
    game.pressGuiButton(fmt::format("LoadMenu_Slot{}", index));
    game.tick(2);
    ASSERT_EQ(saveLoadMenu()->selectedSlot().fileName, checkpointFileName);
    game.pressGuiButton("LoadMenu_Load");
    game.skipLoadingScreen();
    game.tick(2);

    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_TATALIA);
    ASSERT_EQ(engine->_lastLoadedSaveFileName, checkpointFileName);
    EXPECT_FALSE(pParty->_questBits[QBIT_PAINTINGS_ACTIVE]);
    for (const PaintingSource &source : paintingSources) {
        EXPECT_TRUE(pParty->_questBits[source.takenBit]);
        EXPECT_FALSE(pParty->hasItem(source.item));
    }
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_EQ(pParty->uNumGold, checkpointGoldBaseline + 50000);
    for (int i = 0; i < 4; ++i)
        EXPECT_EQ(pParty->pCharacters[i].experience, checkpointExperienceBaseline[i] + 50000);
    EXPECT_TRUE(ufs->read(checkpointSlot).str() == saved.str());
}
