#include <array>
#include <cmath>
#include <numbers>
#include <string>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Vis.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Objects/NPC.h"
#include "Engine/Party.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/ItemGrid.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIHouses.h"

#include "Library/Geometry/Point.h"

#include "Utility/Exception.h"

namespace {

constexpr Vec3f markhamHeadquartersDoor(16154, 8560, 128);
constexpr Pointi inventoryOffset(14, 17);
constexpr int salSharktoothNpcId = 7;
constexpr int salSharktoothOfferEventId = 21;
constexpr int salSharktoothRefusalEventId = 24;
constexpr int salSharktoothGroup = 3;

struct Delivery {
    ItemId item;
    QuestBit active;
    QuestBit given;
};

struct NpcOfferChain {
    unsigned int offer;
    unsigned int reward;
};

constexpr std::array<Delivery, 6> deliveries = {{
    {ITEM_POTION_CURE_WOUNDS, QBIT_EMERALD_ISLAND_RED_POTION_ACTIVE, QBIT_EMERALD_ISLAND_RED_POTION_GIVEN},
    {ITEM_QUEST_SEASHELL, QBIT_EMERALD_ISLAND_SEASHELL_ACTIVE, QBIT_EMERALD_ISLAND_SEASHELL_GIVEN},
    {ITEM_LONGBOW, QBIT_EMERALD_ISLAND_LONGBOW_ACTIVE, QBIT_EMERALD_ISLAND_LONGBOW_GIVEN},
    {ITEM_QUEST_FLOOR_TILE_W_MOON_INSIGNIA, QBIT_EMERALD_ISLAND_FLOOR_TILE_ACTIVE,
     QBIT_EMERALD_ISLAND_FLOOR_TILE_GIVEN},
    {ITEM_QUEST_LUTE, QBIT_EMERALD_ISLAND_LUTE_ACTIVE, QBIT_EMERALD_ISLAND_LUTE_GIVEN},
    {ITEM_QUEST_WEALTHY_HAT, QBIT_EMERALD_ISLAND_HAT_ACTIVE, QBIT_EMERALD_ISLAND_HAT_GIVEN},
}};

NPCData *findNpcByGreeting(int greeting) {
    auto npc = std::ranges::find(pNPCStats->pNPCData, greeting, &NPCData::greetingIndex);
    EXPECT_NE(npc, pNPCStats->pNPCData.end()) << greeting;
    return npc == pNPCStats->pNPCData.end() ? nullptr : &*npc;
}

unsigned int findNpcEvent(const NPCData &npc, auto predicate) {
    std::array eventIds = {npc.dialogue_1_evt_id, npc.dialogue_2_evt_id, npc.dialogue_3_evt_id,
                           npc.dialogue_4_evt_id, npc.dialogue_5_evt_id, npc.dialogue_6_evt_id};
    auto event = std::ranges::find_if(eventIds, [predicate](unsigned int eventId) {
        return eventId != 0 && engine->_globalEventMap.hasEvent(eventId) &&
               std::ranges::any_of(engine->_globalEventMap.function(eventId), predicate);
    });
    std::string dump;
    for (unsigned int eventId : eventIds) {
        if (!eventId || !engine->_globalEventMap.hasEvent(eventId))
            continue;
        dump += fmt::format("\nevent {}:", eventId);
        for (const EvtInstruction &instruction : engine->_globalEventMap.function(eventId))
            dump += fmt::format("\n  {}", instruction.toString(EVT_SOURCE_GLOBAL));
    }
    EXPECT_NE(event, eventIds.end()) << "npc=" << npc.name << " greeting=" << npc.greetingIndex
                                     << dump;
    return event == eventIds.end() ? 0 : *event;
}

bool eventChangesVariable(const EvtInstruction &instruction, EvtOpcode opcode,
                          EvtVariable variable, int value) {
    return instruction.opcode == opcode && instruction.data.variable_descr.type == variable &&
           instruction.data.variable_descr.value == value;
}

NpcOfferChain itemOfferChain(const NPCData &npc, ItemId item) {
    const int npcId = &npc - pNPCStats->pNPCData.data();
    const unsigned int offer = findNpcEvent(npc, [npcId, item](const EvtInstruction &instruction) {
        if (instruction.opcode != EVENT_SetNPCTopic ||
            instruction.data.npc_topic_descr.npc_id != npcId)
            return false;
        const unsigned int reward = instruction.data.npc_topic_descr.event_id;
        return engine->_globalEventMap.hasEvent(reward) &&
               std::ranges::any_of(engine->_globalEventMap.function(reward), [item](const EvtInstruction &nested) {
                   return eventChangesVariable(nested, EVENT_Add, VAR_PlayerItemInHands,
                                               std::to_underlying(item));
               });
    });
    auto topic = std::ranges::find_if(engine->_globalEventMap.function(offer), [npcId, item](const EvtInstruction &instruction) {
        if (instruction.opcode != EVENT_SetNPCTopic ||
            instruction.data.npc_topic_descr.npc_id != npcId)
            return false;
        const unsigned int reward = instruction.data.npc_topic_descr.event_id;
        return engine->_globalEventMap.hasEvent(reward) &&
               std::ranges::any_of(engine->_globalEventMap.function(reward), [item](const EvtInstruction &nested) {
                   return eventChangesVariable(nested, EVENT_Add, VAR_PlayerItemInHands,
                                               std::to_underlying(item));
               });
    });
    EXPECT_NE(topic, engine->_globalEventMap.function(offer).end());
    return {offer, topic == engine->_globalEventMap.function(offer).end() ? 0u :
                       static_cast<unsigned int>(topic->data.npc_topic_descr.event_id)};
}

bool selectNpcEvent(EngineController &game, NPCData &npc, unsigned int eventId) {
    std::array eventIds = {npc.dialogue_1_evt_id, npc.dialogue_2_evt_id, npc.dialogue_3_evt_id,
                           npc.dialogue_4_evt_id, npc.dialogue_5_evt_id, npc.dialogue_6_evt_id};
    auto event = std::ranges::find(eventIds, eventId);
    if (event == eventIds.end())
        return false;
    DialogueId line = static_cast<DialogueId>(std::to_underlying(DIALOGUE_SCRIPTED_LINE_1) +
                                               (event - eventIds.begin()));
    auto topics = prepareScriptedNPCDialogueTopics(&npc);
    auto option = std::ranges::find(topics, line);
    if (option == topics.end())
        return false;
    game.pressGuiButton(fmt::format("NpcDialogue_Option{}", option - topics.begin()));
    game.tick();
    return true;
}

void leaveDialogue(EngineController &game) {
    for (int i = 0; i < 4 && current_screen_type != SCREEN_GAME; ++i) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

void openMapNpc(EngineController &game, MapId map, NPCData *npc) {
    ASSERT_NE(npc, nullptr);
    ASSERT_EQ(engine->_currentLoadedMapId, map);
    const int npcId = npc - pNPCStats->pNPCData.data();
    auto actor = std::ranges::find(pActors, npcId, &Actor::npcId);
    ASSERT_NE(actor, pActors.end()) << npc->name;
    for (const auto &[offset, yaw] : std::array{
             std::pair{Vec3f(200, 0, 0), 180}, std::pair{Vec3f(-200, 0, 0), 0},
             std::pair{Vec3f(0, 200, 0), 270}, std::pair{Vec3f(0, -200, 0), 90}}) {
        game.teleportTo(map, actor->pos + offset, yaw);
        try {
            game.pointMouseAtActor(actor->id);
        } catch (const Exception &) {
            continue;
        }
        game.pressAndReleaseButton(BUTTON_LEFT);
        game.tick();
        if (current_screen_type == SCREEN_NPC_DIALOGUE)
            return;
    }
    FAIL() << "Could not physically open dialogue with " << npc->name;
}

int countPartyItem(ItemId item) {
    int result = pParty->pPickedItem.itemId == item;
    for (const Character &character : pParty->pCharacters)
        for (InventoryConstEntry entry : character.inventory.entries())
            result += entry->itemId == item;
    return result;
}

void storeHeldItem(EngineController &game) {
    ASSERT_NE(pParty->pPickedItem.itemId, ITEM_NULL);
    game.pressGuiButton("Game_Character1");
    game.tick();
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
}

void buyNpcItem(EngineController &game, NPCData *npc, ItemId item, int price) {
    const NpcOfferChain chain = itemOfferChain(*npc, item);
    ASSERT_TRUE(std::ranges::any_of(engine->_globalEventMap.function(chain.reward), [price](const EvtInstruction &instruction) {
        return eventChangesVariable(instruction, EVENT_Subtract, VAR_FixedGold, price);
    }));
    openMapNpc(game, MAP_EMERALD_ISLAND, npc);
    ASSERT_TRUE(selectNpcEvent(game, *npc, chain.offer));
    const int goldBefore = pParty->GetGold();
    ASSERT_TRUE(selectNpcEvent(game, *npc, chain.reward));
    EXPECT_EQ(pParty->GetGold(), goldBefore - price);
    EXPECT_EQ(pParty->pPickedItem.itemId, item);
    leaveDialogue(game);
    storeHeldItem(game);
}

Pointi itemPoint(InventoryEntry entry) {
    return mapFromInventoryGrid(entry.geometry().topLeft(), inventoryOffset) + Pointi(16, 16);
}

void brewCureWounds(EngineController &game, int characterIndex) {
    Character &character = pParty->pCharacters[characterIndex];
    InventoryEntry reagent = character.inventory.find(ITEM_REAGENT_WIDOWSWEEP_BERRIES);
    InventoryEntry bottle = character.inventory.find(ITEM_POTION_BOTTLE);
    ASSERT_TRUE(reagent);
    ASSERT_TRUE(bottle);
    game.goToInventory(characterIndex);
    game.pressAndReleaseButton(BUTTON_LEFT, itemPoint(reagent));
    game.tick();
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_REAGENT_WIDOWSWEEP_BERRIES);
    const Pointi bottlePoint = itemPoint(bottle);
    game.pressButton(BUTTON_RIGHT, bottlePoint);
    game.tick();
    game.releaseButton(BUTTON_RIGHT, bottlePoint);
    game.tick();
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_EQ(bottle->itemId, ITEM_POTION_CURE_WOUNDS);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

void tradePotionForHat(EngineController &game, NPCData *brent) {
    const NpcOfferChain chain = itemOfferChain(*brent, ITEM_QUEST_WEALTHY_HAT);
    ASSERT_TRUE(std::ranges::any_of(engine->_globalEventMap.function(chain.reward), [](const EvtInstruction &instruction) {
        return eventChangesVariable(instruction, EVENT_Subtract, VAR_PlayerItemInHands,
                                    std::to_underlying(ITEM_POTION_CURE_WOUNDS));
    }));
    openMapNpc(game, MAP_EMERALD_ISLAND, brent);
    ASSERT_TRUE(selectNpcEvent(game, *brent, chain.offer));
    const int potionBefore = countPartyItem(ITEM_POTION_CURE_WOUNDS);
    const int goldBefore = pParty->GetGold();
    ASSERT_TRUE(selectNpcEvent(game, *brent, chain.reward));
    EXPECT_EQ(countPartyItem(ITEM_POTION_CURE_WOUNDS), potionBefore - 1);
    EXPECT_EQ(pParty->GetGold(), goldBefore);
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_QUEST_WEALTHY_HAT);
    leaveDialogue(game);
    storeHeldItem(game);
}

void takeItemToCursor(EngineController &game, ItemId item) {
    for (int characterIndex = 0; characterIndex < 4; ++characterIndex) {
        InventoryEntry entry = pParty->pCharacters[characterIndex].inventory.find(item);
        if (!entry)
            continue;
        game.goToInventory(characterIndex);
        game.pressAndReleaseButton(BUTTON_LEFT, itemPoint(entry));
        game.tick();
        ASSERT_EQ(pParty->pPickedItem.itemId, item);
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
        ASSERT_EQ(current_screen_type, SCREEN_GAME);
        return;
    }
    FAIL() << "Missing acquired item " << std::to_underlying(item);
}

bool actorCarriesItem(const Actor &actor, ItemId item) {
    return actor.carriedItemId == item || std::ranges::any_of(actor.items, [item](const Item &candidate) {
        return candidate.itemId == item;
    });
}

Actor *findSalSharktooth() {
    auto sal = std::ranges::find_if(pActors, [](const Actor &actor) {
        return actor.npcId == salSharktoothNpcId && actorCarriesItem(actor, ITEM_LONGBOW);
    });
    EXPECT_NE(sal, pActors.end());
    return sal == pActors.end() ? nullptr : &*sal;
}

void prepareContestCombatParty() {
    for (Character &character : pParty->pCharacters) {
        character.inventory.equip(ITEM_SLOT_MAIN_HAND, Item(ITEM_CHAMPION_SWORD));
        character.setSkillValue(SKILL_SWORD, CombinedSkillValue(30, MASTERY_GRANDMASTER));
        character._stats[ATTRIBUTE_MIGHT] = 200;
        character._stats[ATTRIBUTE_ACCURACY] = 200;
        character.uLevel = 100;
    }
}

void lootSalSharktooth(EngineController &game, int actorId) {
    ASSERT_EQ(pActors[actorId].aiState, Dead);
    auto viewport = std::ranges::find(pPrimaryWindow->vButtons, "Game_Viewport", &GUIButton::id);
    ASSERT_NE(viewport, pPrimaryWindow->vButtons.end());

    bool corpseVisible = false;
    for (const auto &[offset, yaw] : std::array{
             std::pair{Vec3f(0, -250, 0), 90}, std::pair{Vec3f(-250, 0, 0), 0},
             std::pair{Vec3f(0, 250, 0), 270}, std::pair{Vec3f(250, 0, 0), 180}}) {
        for (int pitch : {-20, 0, 20}) {
            game.teleportTo(MAP_TEMPLE_OF_THE_MOON, pActors[actorId].pos + offset, yaw, pitch);
            for (int y = 10; y < 470 && !corpseVisible; y += 10) {
                for (int x = 10; x < 630 && !corpseVisible; x += 10) {
                    Pointi point(x, y);
                    if (!(*viewport)->Contains(point) ||
                        engine->PickMouse(engine->config->gameplay.MouseInteractionDepth.value(), x, y,
                                          &vis_anything_filter, &vis_face_filter).pid != Pid::actor(actorId))
                        continue;
                    game.moveMouse(point);
                    game.tick();
                    corpseVisible = engine->PickMouseForInteraction().pid == Pid::actor(actorId);
                }
            }
            if (corpseVisible)
                break;
        }
        if (corpseVisible)
            break;
    }
    ASSERT_TRUE(corpseVisible);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
}

void acquireLongbowFromSalSharktooth(EngineController &game) {
    takeItemToCursor(game, ITEM_QUEST_WEALTHY_HAT);

    // Setup teleport beside the authored carrier. MM7TempleOfMoon separately covers the in-world map transition.
    game.teleportTo(MAP_TEMPLE_OF_THE_MOON, Vec3f(-5568, 6534, -96), 90);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_TEMPLE_OF_THE_MOON);
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_QUEST_WEALTHY_HAT);

    NPCData &salNpc = pNPCStats->pNPCData[salSharktoothNpcId];
    ASSERT_EQ(salNpc.dialogue_1_evt_id, salSharktoothOfferEventId);
    ASSERT_EQ(salNpc.dialogue_2_evt_id, 22u);
    ASSERT_TRUE(engine->_globalEventMap.hasEvent(salSharktoothRefusalEventId));
    const auto &refusal = engine->_globalEventMap.function(salSharktoothRefusalEventId);
    auto hostility = std::ranges::find(refusal, EVENT_ToggleActorGroupFlag, &EvtInstruction::opcode);
    ASSERT_NE(hostility, refusal.end());
    EXPECT_EQ(hostility->data.actor_flag_descr.id, salSharktoothGroup);
    EXPECT_EQ(hostility->data.actor_flag_descr.attr, ACTOR_AGGRESSOR);
    EXPECT_TRUE(hostility->data.actor_flag_descr.is_set);

    Actor *sal = findSalSharktooth();
    ASSERT_NE(sal, nullptr);
    const int salActorId = sal->id;
    EXPECT_EQ(sal->group, 0u);
    EXPECT_FALSE(sal->ActorEnemy());

    openMapNpc(game, MAP_TEMPLE_OF_THE_MOON, &salNpc);
    ASSERT_TRUE(selectNpcEvent(game, salNpc, salSharktoothOfferEventId));
    EXPECT_EQ(salNpc.dialogue_1_evt_id, 23u);
    EXPECT_EQ(salNpc.dialogue_2_evt_id, salSharktoothRefusalEventId);
    ASSERT_TRUE(selectNpcEvent(game, salNpc, salSharktoothRefusalEventId));
    leaveDialogue(game);
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_QUEST_WEALTHY_HAT);
    EXPECT_EQ(salNpc.dialogue_1_evt_id, 0u);
    EXPECT_EQ(salNpc.dialogue_2_evt_id, 0u);
    EXPECT_TRUE(std::ranges::any_of(pActors, [](const Actor &actor) {
        return actor.group == salSharktoothGroup && actor.ActorEnemy();
    }));
    EXPECT_FALSE(pActors[salActorId].ActorEnemy());
    storeHeldItem(game);

    prepareContestCombatParty();
    const int initialHp = pActors[salActorId].hp;
    bool becameAggressor = false;
    game.teleportTo(MAP_TEMPLE_OF_THE_MOON, pActors[salActorId].pos + Vec3f(0, -250, 0), 90);
    game.pointMouseAtActor(salActorId);
    for (int attack = 0; attack < 400 && !pActors[salActorId].IsNotAlive(); ++attack) {
        SCOPED_TRACE(fmt::format("attack {}", attack));
        if (attack && attack % 10 == 0) {
            game.teleportTo(MAP_TEMPLE_OF_THE_MOON,
                            pActors[salActorId].pos + Vec3f(0, -250, 0), 90);
            game.pointMouseAtActor(salActorId);
        }
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick(5);
        becameAggressor |= pActors[salActorId].ActorEnemy();
    }
    EXPECT_LT(pActors[salActorId].hp, initialHp);
    EXPECT_TRUE(becameAggressor);
    ASSERT_TRUE(pActors[salActorId].IsNotAlive()) << "remaining HP: " << pActors[salActorId].hp;
    for (int frame = 0; frame < 120 && pActors[salActorId].aiState != Dead; ++frame)
        game.tick();
    ASSERT_EQ(pActors[salActorId].aiState, Dead);

    lootSalSharktooth(game, salActorId);
    EXPECT_FALSE(actorCarriesItem(pActors[salActorId], ITEM_LONGBOW));
    EXPECT_EQ(countPartyItem(ITEM_LONGBOW), 1);
    EXPECT_EQ(countPartyItem(ITEM_QUEST_WEALTHY_HAT), 1);

    // Setup teleport back to the contest hub after the fully physical acquisition.
    game.teleportTo(MAP_EMERALD_ISLAND, markhamHeadquartersDoor, 180);
    for (Character &character : pParty->pCharacters) {
        character.health = character.GetMaxHealth();
        character.timeToRecovery = 0_ticks;
    }
    pParty->setActiveCharacterIndex(0);
}

void handItemToJudge(EngineController &game, const Delivery &delivery) {
    if (delivery.item == ITEM_QUEST_FLOOR_TILE_W_MOON_INSIGNIA) {
        // The Temple-of-the-Moon suite already proves this sixth item's physical route.
        pParty->setHoldingItem(Item(delivery.item));
    } else {
        takeItemToCursor(game, delivery.item);
    }
    ASSERT_TRUE(pParty->_questBits[delivery.active]);
    ASSERT_FALSE(pParty->_questBits[delivery.given]);
    game.teleportTo(MAP_EMERALD_ISLAND, markhamHeadquartersDoor, 180);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_EMERALD_ISLAND_MARKHAMS_HEADQUARTERS);
    game.pressGuiButton("House_Npc1");
    game.tick();
    game.pressGuiButton("HouseNpcDialogue_Option1");
    game.tick();
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_FALSE(pParty->_questBits[delivery.active]);
    EXPECT_TRUE(pParty->_questBits[delivery.given]);
    leaveDialogue(game);
}

}  // namespace

GAME_TEST(MM7EmeraldContestAcquisition, FivePhysicalSourcesPersistAndJudgeAcceptsThem) {
    engine->config->debug.NoMargaret.setValue(true);
    engine->config->debug.NoDamage.setValue(true);
    game.startNewGame();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_EMERALD_ISLAND);
    pParty->SetGold(3000, true);

    NPCData *sally = findNpcByGreeting(5);
    NPCData *ailyssa = findNpcByGreeting(4);
    NPCData *brent = findNpcByGreeting(10);
    ASSERT_NE(sally, nullptr);
    ASSERT_NE(ailyssa, nullptr);
    ASSERT_NE(brent, nullptr);

    // Ingredients are setup; the contest potion itself is produced through the live inventory/alchemy interaction.
    Character &alchemist = pParty->pCharacters[2];
    for (int i = 0; i < 2; ++i) {
        ASSERT_TRUE(alchemist.inventory.add(Item(ITEM_REAGENT_WIDOWSWEEP_BERRIES)));
        ASSERT_TRUE(alchemist.inventory.add(Item(ITEM_POTION_BOTTLE)));
        brewCureWounds(game, 2);
    }
    ASSERT_EQ(countPartyItem(ITEM_POTION_CURE_WOUNDS), 2);

    const int initialGold = pParty->GetGold();
    buyNpcItem(game, sally, ITEM_QUEST_SEASHELL, 100);
    buyNpcItem(game, ailyssa, ITEM_QUEST_LUTE, 500);
    tradePotionForHat(game, brent);
    EXPECT_EQ(pParty->GetGold(), initialGold - 100 - 500);

    acquireLongbowFromSalSharktooth(game);

    for (ItemId item : {ITEM_POTION_CURE_WOUNDS, ITEM_QUEST_SEASHELL,
                        ITEM_LONGBOW, ITEM_QUEST_LUTE, ITEM_QUEST_WEALTHY_HAT})
        ASSERT_EQ(countPartyItem(item), 1) << std::to_underlying(item);

    const std::array sallyTopics = {sally->dialogue_1_evt_id, sally->dialogue_2_evt_id,
                                    sally->dialogue_3_evt_id, sally->dialogue_4_evt_id};
    const std::array ailyssaTopics = {ailyssa->dialogue_1_evt_id, ailyssa->dialogue_2_evt_id,
                                      ailyssa->dialogue_3_evt_id, ailyssa->dialogue_4_evt_id};
    const std::array brentTopics = {brent->dialogue_1_evt_id, brent->dialogue_2_evt_id,
                                    brent->dialogue_3_evt_id, brent->dialogue_4_evt_id};
    const std::array salTopics = {pNPCStats->pNPCData[salSharktoothNpcId].dialogue_1_evt_id,
                                  pNPCStats->pNPCData[salSharktoothNpcId].dialogue_2_evt_id};
    const int acquiredGold = pParty->GetGold();
    Blob checkpoint = game.saveGame();

    for (Character &character : pParty->pCharacters)
        character.inventory.clear();
    pParty->SetGold(0, true);
    game.loadGame(checkpoint);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_EMERALD_ISLAND);
    EXPECT_EQ(pParty->GetGold(), acquiredGold);
    for (ItemId item : {ITEM_POTION_CURE_WOUNDS, ITEM_QUEST_SEASHELL,
                        ITEM_LONGBOW, ITEM_QUEST_LUTE, ITEM_QUEST_WEALTHY_HAT})
        EXPECT_EQ(countPartyItem(item), 1) << std::to_underlying(item);
    sally = findNpcByGreeting(5);
    ailyssa = findNpcByGreeting(4);
    brent = findNpcByGreeting(10);
    EXPECT_EQ((std::array{sally->dialogue_1_evt_id, sally->dialogue_2_evt_id,
                          sally->dialogue_3_evt_id, sally->dialogue_4_evt_id}), sallyTopics);
    EXPECT_EQ((std::array{ailyssa->dialogue_1_evt_id, ailyssa->dialogue_2_evt_id,
                          ailyssa->dialogue_3_evt_id, ailyssa->dialogue_4_evt_id}), ailyssaTopics);
    EXPECT_EQ((std::array{brent->dialogue_1_evt_id, brent->dialogue_2_evt_id,
                          brent->dialogue_3_evt_id, brent->dialogue_4_evt_id}), brentTopics);
    EXPECT_EQ((std::array{pNPCStats->pNPCData[salSharktoothNpcId].dialogue_1_evt_id,
                          pNPCStats->pNPCData[salSharktoothNpcId].dialogue_2_evt_id}), salTopics);

    for (const Delivery &delivery : deliveries)
        if (delivery.item != ITEM_QUEST_FLOOR_TILE_W_MOON_INSIGNIA)
            handItemToJudge(game, delivery);
    EXPECT_FALSE(pParty->_questBits[QBIT_EMERALD_ISLAND_SCAVENGER_HUNT_WON]);
    EXPECT_FALSE(pParty->_questBits[QBIT_EMERALD_ISLAND_LONGBOW_ACTIVE]);
    EXPECT_TRUE(pParty->_questBits[QBIT_EMERALD_ISLAND_LONGBOW_GIVEN]);
    EXPECT_TRUE(pParty->_questBits[QBIT_EMERALD_ISLAND_FLOOR_TILE_ACTIVE]);
}
