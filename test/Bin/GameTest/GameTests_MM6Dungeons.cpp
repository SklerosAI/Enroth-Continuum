#include <algorithm>
#include <cmath>
#include <numbers>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Evt/MM6MapByteState.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Localization.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Party.h"
#include "Engine/Tables/ChestTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/UIRest.h"

#include "Utility/Exception.h"

#include "MM6CampaignChain.h"

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

static void confirmTransition(EngineController &game) {
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.tick();
    game.skipLoadingScreen();
    game.tick();
}

static void enterD02(EngineController &game) {
    game.teleportTo(pMapTable->GetMapInfo("oute3.odm"), Vec3f(-21456, -286, 193), 90); // In front of the d02 door, facing it.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    confirmTransition(game);
}

static void strengthenParty() {
    // Fixture for the dungeon walks, a new party can't clear a dungeon whose monsters all come for it.
    for (Character &character : pParty->pCharacters) {
        character.health = 2000;
        character._stats[ATTRIBUTE_MIGHT] = 200;
        character._stats[ATTRIBUTE_ACCURACY] = 200;
    }
}

static std::vector<int> monstersNearParty(float range) {
    std::vector<int> result;
    for (Actor &actor : pActors)
        if (!actor.IsNotAlive() && (actor.pos - pParty->pos).length() < range)
            result.push_back(actor.id);
    return result;
}

static bool partyUnhurtAt(int health) {
    return std::ranges::all_of(pParty->pCharacters, [health](const Character &character) { return character.health == health; });
}

static bool partyHurt() {
    return std::ranges::any_of(pParty->pCharacters, [](const Character &character) { return character.health < character.GetMaxHealth(); });
}

static bool standAwayFrom(EngineController &game, Vec3f target, float distance) {
    // On the floor of the target's sector, the given distance away from it and facing it. False when there's no floor.
    int targetSector = 0;
    GetIndoorFloorZ(target + Vec3f(0, 0, 40), &targetSector);
    for (Vec3f direction : {Vec3f(0, -1, 0), Vec3f(-1, 0, 0), Vec3f(0, 1, 0), Vec3f(1, 0, 0)}) {
        Vec3f position = target + distance * direction;
        int sectorId = 0;
        position.z = GetIndoorFloorZ(position + Vec3f(0, 0, 40), &sectorId);
        if (sectorId != targetSector)
            continue;
        game.teleportTo(engine->_currentLoadedMapId, position, std::lround(std::atan2(-direction.y, -direction.x) * 180 / std::numbers::pi));
        return true;
    }
    return false;
}

static void fightUntilDead(EngineController &game, const std::vector<int> &actorIds) {
    // The party attacks the nearest of the monsters until all of them are dead, and steps up to one out of reach.
    for (int attack = 0; attack < 8000; attack++) {
        auto alive = actorIds | std::views::filter([](int id) { return !pActors[id].IsNotAlive(); });
        auto nearest = std::ranges::min_element(alive, {}, [](int id) { return (pActors[id].pos - pParty->pos).length(); });
        if (nearest == alive.end())
            return;
        if (attack % 20 == 0 && (pActors[*nearest].pos - pParty->pos).length() > 300)
            standAwayFrom(game, pActors[*nearest].pos, 150); // A flying monster can hang over no floor of its own sector.
        try {
            game.pointMouseAtActor(*nearest);
        } catch (const Exception &) {
            // The monster can be out of view for a frame, A still attacks then.
        }
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick();
    }
    FAIL() << "The monsters outlived eight thousand attacks";
}

static void restAndHeal(EngineController &game) {
    game.pressAndReleaseKey(PlatformKey::KEY_R);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_REST);
    game.pressGuiButton("Rest_RestAndHeal");
    game.tick();
    while (currentRestType != REST_NONE)
        game.tick();
}

struct FacePlace {
    int faceId;
    Vec3f position;
    int yaw;
};

static std::vector<FacePlace> placesNearIndoorEventFace(int eventId) {
    // Floor spots in front of the wall faces of the event, then around all its faces, with the yaws that face them.
    std::vector<FacePlace> result;
    auto tryPlace = [&](int faceId, Vec2f offset) {
        const BLVFace &face = pIndoor->faces[faceId];
        Vec3f position = face.boundingBox.center() + Vec3f(offset.x, offset.y, 0);
        int sectorId = 0;
        position.z = GetIndoorFloorZ(position + Vec3f(0, 0, 40), &sectorId);
        if (position.z > -29000 && position.z <= face.boundingBox.z2 + 50 && position.z + 400 >= face.boundingBox.z1)
            result.push_back({faceId, position, static_cast<int>(std::lround(std::atan2(-offset.y, -offset.x) * 180 / std::numbers::pi))});
    };
    for (float distance : {150.0f, 250.0f, 100.0f, 350.0f}) {
        for (int faceId = 0; faceId < std::ssize(pIndoor->faces); faceId++) {
            const Vec2f normal = pIndoor->faces[faceId].facePlane.normal.xy();
            if (pIndoor->faces[faceId].eventId == eventId && normal.length() > 0.5f)
                tryPlace(faceId, normal / normal.length() * distance);
        }
    }
    for (float distance : {150.0f, 250.0f, 350.0f}) {
        for (int faceId = 0; faceId < std::ssize(pIndoor->faces); faceId++) {
            if (pIndoor->faces[faceId].eventId != eventId)
                continue;
            for (int angle = 0; angle < 360; angle += 45)
                tryPlace(faceId, Vec2f(std::cos(angle * std::numbers::pi / 180), std::sin(angle * std::numbers::pi / 180)) * distance);
        }
    }
    return result;
}

static void waitForActiveCharacter(EngineController &game) {
    for (int frame = 0; frame < 100 && !pParty->hasActiveCharacter(); frame++)
        game.tick(); // Nobody acts while recovering from the last blow.
}

static void useIndoorEventFace(EngineController &game, int eventId) {
    // Stands on the floor in front of a face of the event, faces it and presses Space.
    const auto places = placesNearIndoorEventFace(eventId);
    ASSERT_FALSE(places.empty()) << "No floor near a face of event " << eventId;
    game.teleportTo(engine->_currentLoadedMapId, places.front().position, places.front().yaw);
    waitForActiveCharacter(game);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
}

static void clickIndoorEventFace(EngineController &game, int eventId) {
    // Stands on the floor near a face of the event, faces it and clicks it.
    const auto places = placesNearIndoorEventFace(eventId);
    ASSERT_FALSE(places.empty()) << "No floor near a face of event " << eventId;
    for (const FacePlace &place : places) {
        for (int pitch : {0, -25, 25}) {
            game.teleportTo(engine->_currentLoadedMapId, place.position, place.yaw, pitch);
            waitForActiveCharacter(game);
            try {
                game.pointMouseAtFace(place.faceId);
            } catch (const Exception &) {
                continue; // The face is off screen from here.
            }
            game.pressAndReleaseButton(BUTTON_LEFT);
            game.tick();
            return;
        }
    }
    useIndoorEventFace(game, eventId); // Space reaches a face the mouse can't point at from anywhere near.
}

static void stepOntoIndoorEventPlate(EngineController &game, int eventId) {
    // Drops the party onto the middle of the first floor face of the event.
    auto plate = std::ranges::find_if(pIndoor->faces, [&](const BLVFace &face) {
        return face.eventId == eventId && face.Pressure_Plate() && face.facePlane.normal.z > 0.9f;
    });
    ASSERT_NE(plate, pIndoor->faces.end());
    game.teleportTo(engine->_currentLoadedMapId, plate->boundingBox.center() + Vec3f(0, 0, 50), 0);
    game.tick();
}

static void killActor(EngineController &game, int actorId) {
    // Stands next to the monster and attacks it until it dies.
    for (int attack = 0; attack < 8000 && !pActors[actorId].IsNotAlive(); attack++) {
        if (attack % 20 == 0)
            standAwayFrom(game, pActors[actorId].pos, 150); // A flying monster can hang over no floor of its own sector.
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

static std::vector<int> d02QuestBits() {
    // Potbello's candelabra, the spider queen, Angela and her rescue, and the candelabra chest's first opening.
    std::vector<int> result;
    for (int bit : {32, 34, 126, 130, 139})
        if (pParty->_questBits[static_cast<QuestBit>(bit)])
            result.push_back(bit);
    return result;
}

static void takeTownQuest(EngineController &game, int doorEvent, int npcId, int topicEvent) {
    useMM6CampaignChainEventFace(game, pMapTable->GetMapInfo("oute3.odm"), doorEvent);
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    talkToMM6CampaignChainNpc(game, npcId);
    ASSERT_TRUE(selectMM6CampaignChainTopic(game, topicEvent));
    leaveMM6CampaignChainHouse(game);
}

static void openChest(EngineController &game, int eventId) {
    // Clicks the chest. A failed disarm sets the trap off and leaves it shut, the next click opens it.
    for (int attempt = 0; attempt < 3 && current_screen_type != SCREEN_CHEST; attempt++)
        clickIndoorEventFace(game, eventId);
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
}

static void takeChestItem(EngineController &game, int chestId, ItemId itemId) {
    // Picks the item up from the open chest, closes the chest and puts the item into the first backpack with room.
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    InventoryEntry item = vChests[chestId].inventory.find(itemId);
    ASSERT_TRUE(item);
    game.pressAndReleaseButton(BUTTON_LEFT, chestTable[vChests[chestId].chestTypeId].inventoryOffset + item.geometry().topLeft() * 32 + Pointi(16, 16));
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    for (int character = 1; character <= 4 && pParty->pPickedItem.itemId != ITEM_NULL; character++) {
        game.pressGuiButton(fmt::format("Game_Character{}", character));
        game.tick();
    }
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
}

static ItemId firstNonGoldItem(int chestId) {
    for (InventoryEntry entry : vChests[chestId].inventory.entries())
        if (!entry->isGold())
            return entry->itemId;
    return ITEM_NULL;
}

static void waitUntilHour(EngineController &game, int hour) {
    // The party stands still while game time passes. A frame is 3 game seconds, so a day is 28800 frames.
    for (int frame = 0; frame < 28800; ++frame) {
        if (pParty->GetPlayingTime().toCivilTime().hour == hour)
            return;
        game.tick();
    }
    FAIL() << "Game time didn't reach " << hour << ":00 within a day";
}

static std::vector<int> d01QuestBits() {
    // Janice's Goblinwatch quest, and the keep unlocked.
    std::vector<int> result;
    for (int bit : {83, 300})
        if (pParty->_questBits[static_cast<QuestBit>(bit)])
            result.push_back(bit);
    return result;
}

GAME_TEST(MM6Dungeons, TempleBatsBiteTheParty) {
    // The Abandoned Temple's monsters never noticed the party. The bats of the entrance hall fly up and bite.
    startNewGameThroughInput(game);
    enterD02(game);
    auto hpTape = tapes.totalHp();
    auto huntersTape = tapes.custom([] {
        return std::ranges::count_if(pActors, [](const Actor &actor) { return actor.monsterInfo.hostilityType == HOSTILITY_LONG; });
    });
    test.startTaping();

    game.tick(100); // The bats fly up and bite.

    EXPECT_LT(hpTape.delta(), 0);
    EXPECT_GT(huntersTape.back(), 0);
}

GAME_TEST(MM6Dungeons, TempleEntranceRaisesTheAlert) {
    // The alert gems stayed green with the Abandoned Temple's bats in sight of the entrance.
    startNewGameThroughInput(game);
    enterD02(game);
    auto yellowTape = tapes.custom([] { return pParty->GetYellowAlert(); });
    test.startTaping();

    game.tick();

    EXPECT_EQ(yellowTape, tape(true));
}

GAME_TEST(MM6Dungeons, RestNearAndFarFromMonsters) {
    // The Abandoned Temple's monsters never noticed the party, and the party could rest next to them. Now the bats of
    // the entrance hall bite and the party can't rest there. In the cage room, far from every monster, it rests and
    // heals.
    startNewGameThroughInput(game);
    enterD02(game);
    auto screenTape = tapes.screen();
    auto statusTape = tapes.statusBar();
    auto timeTape = tapes.time();
    auto hpTape = charTapes.hps();
    auto conditionTape = charTapes.conditions();
    test.startTaping();

    game.pressAndReleaseKey(PlatformKey::KEY_R);
    game.tick();
    EXPECT_EQ(statusTape.back(), localization->str(LSTR_THERE_ARE_HOSTILE_ENEMIES_NEAR));
    EXPECT_EQ(screenTape.back(), SCREEN_GAME);
    for (int frame = 0; frame < 300 && !partyHurt(); frame++)
        game.tick(); // The bats fly up and bite.
    ASSERT_TRUE(partyHurt());

    ASSERT_TRUE(standAwayFrom(game, pLevelDecorations[106].vPosition, 150)); // In the cage room, by the first cage.
    ASSERT_TRUE(monstersNearParty(2560).empty());
    restAndHeal(game);

    EXPECT_CONTAINS(screenTape, SCREEN_REST);
    EXPECT_TRUE(std::ranges::any_of(hpTape, [&](const auto &hps) { return hps != hpTape.front(); })); // The bats hurt somebody.
    EXPECT_EQ(hpTape.back(), hpTape.front()); // And the rest healed everyone.
    EXPECT_EQ(conditionTape.back(), tape(CONDITION_GOOD, CONDITION_GOOD, CONDITION_GOOD, CONDITION_GOOD));
    EXPECT_GE(timeTape.back() - timeTape.front(), Duration::fromHours(8));
}

GAME_TEST(MM6Dungeons, AbandonedTemple) {
    // The New Sorpigal side quests of the Abandoned Temple play through: Potbello's candelabra, Buford's spider queen
    // heart and Violet's daughter Angela, every chest and door of the temple on the way, and the rewards back in town.
    // Fixture: the party gets 2000 HP, 200 might and 200 accuracy, a new party doesn't survive the temple's cobras and
    // traps.
    startNewGameThroughInput(game);
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId d02 = pMapTable->GetMapInfo("d02.blv");
    auto mapTape = tapes.map();
    auto bitsTape = tapes.custom([] { return d02QuestBits(); });
    auto goldTape = tapes.gold();
    auto hirelingsTape = tapes.totalHirelings();
    auto candelabraTape = tapes.hasItem(static_cast<ItemId>(449));
    auto heartTape = tapes.hasItem(static_cast<ItemId>(481));
    auto textTape = tapes.custom([] { return current_npc_text; });
    auto doorsTape = tapes.custom([d02] {
        std::vector<int> result;
        if (engine->_currentLoadedMapId == d02)
            for (const BLVDoor &door : pIndoor->doors)
                if (door.doorId >= 1 && door.doorId <= 9)
                    result.push_back(std::to_underlying(door.state));
        return result;
    });
    test.startTaping();

    takeTownQuest(game, 11, 1, 296); // Andover Potbello.
    takeTownQuest(game, 65, 207, 308); // Buford T. Allman.
    takeTownQuest(game, 66, 154, 328); // Violet Dawson.

    enterD02(game);
    ASSERT_EQ(engine->_currentLoadedMapId, d02);
    strengthenParty();
    for (int frame = 0; frame < 300 && partyUnhurtAt(2000); frame++)
        game.tick(); // The bats fly up and bite.
    const std::vector<int> bats = monstersNearParty(1000);
    auto batHpTape = tapes.custom([&] {
        int total = 0;
        for (int id : bats)
            total += pActors[id].hp;
        return total;
    });
    fightUntilDead(game, bats); // The entrance hall, a real fight.

    for (int eventId = 1; eventId <= 9; eventId++)
        clickIndoorEventFace(game, eventId);

    const std::pair<int, int> chests[] = {{26, 2}, {27, 3}, {28, 5}, {29, 6}, {12, 1}};
    for (auto [eventId, chestId] : chests) {
        openChest(game, eventId);
        const ItemId item = chestId == 2 ? static_cast<ItemId>(449) : firstNonGoldItem(chestId);
        if (item != ITEM_NULL)
            takeChestItem(game, chestId, item);
        else
            leaveMM6CampaignChainHouse(game);
    }

    walkOntoMM6CampaignChainIndoorEventPlate(game, 14); // Angela.
    leaveMM6CampaignChainHouse(game);

    const int queen = std::ranges::find(pActors, static_cast<ItemId>(481), &Actor::carriedItemId)->id;
    killActor(game, queen);
    lootMM6CampaignChainCorpse(game, queen);
    for (int character = 1; character <= 4 && pParty->pPickedItem.itemId != ITEM_NULL; character++) {
        game.pressGuiButton(fmt::format("Game_Character{}", character)); // The heart can come in hand.
        game.tick();
    }

    useIndoorEventFace(game, 31); // The teleporter by the queen leads back to the entrance.
    useIndoorEventFace(game, 50); // The way out.
    confirmTransition(game);
    ASSERT_EQ(engine->_currentLoadedMapId, oute3);

    takeTownQuest(game, 11, 1, 297);
    takeTownQuest(game, 65, 207, 309);
    takeTownQuest(game, 66, 154, 329);

    EXPECT_EQ(mapTape, tape(oute3, d02, oute3));
    EXPECT_EQ(bitsTape.front(), std::vector<int>());
    EXPECT_EQ(bitsTape.back(), std::vector<int>({32, 34}));
    EXPECT_CONTAINS(bitsTape, std::vector<int>({32, 34, 126, 130, 139}));
    EXPECT_EQ(candelabraTape, tape(false, true, false));
    EXPECT_EQ(heartTape, tape(false, true, false));
    EXPECT_EQ(hirelingsTape, tape(0, 1, 0));
    EXPECT_CONTAINS(batHpTape, 0);
    const auto closed = std::ranges::find_if(doorsTape, [](const std::vector<int> &states) { return states.size() == 9; });
    ASSERT_NE(closed, doorsTape.end());
    for (int door = 0; door < 9; door++) // Every door moved.
        EXPECT_TRUE(std::ranges::any_of(doorsTape, [&](const std::vector<int> &states) { return states.size() == 9 && states[door] != (*closed)[door]; })) << door + 1;
    const auto gains = goldTape.adjacentDeltas();
    EXPECT_CONTAINS(gains, 1000); // Potbello's and Buford's rewards.
    EXPECT_CONTAINS(gains, 500); // Violet's.
    for (int text : {307, 323, 351}) // Potbello's, Buford's and Violet's thanks.
        EXPECT_CONTAINS(textTape, pNPCTopics[text - 1].pText);
}

GAME_TEST(MM6Dungeons, Goblinwatch) {
    // Janice's Goblinwatch quest plays through: her key opens the keep, the code scroll lies in its first chest, the
    // combination panel opens the vault doors, every chest opens, and Janice pays for the scroll.
    // Fixture: the party gets 2000 HP, 200 might and 200 accuracy, a new party doesn't survive the keep's goblins and
    // traps.
    startNewGameThroughInput(game);
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId d01 = pMapTable->GetMapInfo("d01.blv");
    auto mapTape = tapes.map();
    auto bitsTape = tapes.custom([] { return d01QuestBits(); });
    auto goldTape = tapes.gold();
    auto keyTape = tapes.custom([] { return pParty->hasItem(static_cast<ItemId>(489)) || pParty->pPickedItem.itemId == static_cast<ItemId>(489); }); // Janice puts it in hand.
    auto codeTape = tapes.hasItem(static_cast<ItemId>(543));
    auto vaultTape = tapes.custom([d01] {
        std::vector<int> result;
        if (engine->_currentLoadedMapId == d01)
            for (const BLVDoor &door : pIndoor->doors)
                if (door.doorId == 59 || door.doorId == 60)
                    result.push_back(std::to_underlying(door.state));
        return result;
    });
    test.startTaping();

    waitUntilHour(game, 10); // The Town Hall opens at 10 am.
    takeTownQuest(game, 28, 291, 3); // Janice gives the key.
    useMM6CampaignChainEventFace(game, oute3, 101); // Goblinwatch.
    confirmTransition(game);
    ASSERT_EQ(engine->_currentLoadedMapId, d01);
    strengthenParty();
    for (int frame = 0; frame < 300 && partyUnhurtAt(2000); frame++)
        game.tick(); // The goblins of the gate hall come for the party.
    const std::vector<int> goblins = monstersNearParty(1000);
    auto goblinsHpTape = tapes.custom([&] {
        int total = 0;
        for (int id : goblins)
            total += pActors[id].hp;
        return total;
    });
    fightUntilDead(game, goblins); // A real fight.

    openChest(game, 41);
    takeChestItem(game, 1, static_cast<ItemId>(543));

    clickIndoorEventFace(game, 25); // G on the combination panel opens the vault.
    clickIndoorEventFace(game, 64); // The dais, its Wizard Eye does nothing.

    for (int eventId = 42; eventId <= 50; eventId++) {
        openChest(game, eventId);
        const int chestId = eventId - 40;
        const ItemId item = firstNonGoldItem(chestId);
        if (item != ITEM_NULL)
            takeChestItem(game, chestId, item);
        else
            leaveMM6CampaignChainHouse(game);
    }

    useIndoorEventFace(game, 51); // The exit door.
    confirmTransition(game);
    ASSERT_EQ(engine->_currentLoadedMapId, oute3);
    takeTownQuest(game, 28, 291, 4); // Janice pays.

    EXPECT_EQ(mapTape, tape(oute3, d01, oute3));
    EXPECT_EQ(bitsTape, tape(std::vector<int>(), std::vector<int>{83}, std::vector<int>{83, 300}, std::vector<int>{300}));
    EXPECT_EQ(keyTape, tape(false, true, false));
    EXPECT_EQ(codeTape, tape(false, true));
    EXPECT_EQ(goldTape.delta(), 2000);
    EXPECT_EQ(goblinsHpTape.back(), 0);
    const std::vector<int> vaultOpen = {std::to_underlying(DOOR_OPEN), std::to_underlying(DOOR_OPEN)};
    EXPECT_CONTAINS(vaultTape, vaultOpen);
}

GAME_TEST(MM6Dungeons, GharikForgeFloorAndChests) {
    // Gharik's Forge beyond the key lever: the hot floor plate starts the burn timer and the cool plate stops it, the
    // bookcase hands out an item, every chest opens, and the way out leads back to New Sorpigal.
    // Fixture: the party gets 2000 HP, 200 might and 200 accuracy, a new party doesn't survive the forge's burns and
    // fireball vents.
    startNewGameThroughInput(game);
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId d18 = pMapTable->GetMapInfo("d18.blv");
    useMM6CampaignChainEventFace(game, oute3, 103); // Gharik's Forge, on its island.
    confirmTransition(game);
    ASSERT_EQ(engine->_currentLoadedMapId, d18);
    strengthenParty();
    auto mapTape = tapes.map();
    auto hpTape = charTapes.hps();
    auto burningTape = tapes.custom([d18] { return engine->_currentLoadedMapId == d18 && !pIndoor->mm6DeltaBytes.empty() && MM6MapByteState(pIndoor->mm6DeltaBytes).compare(105, 1); });
    auto statusTape = tapes.statusBar();
    auto itemCountTape = tapes.totalItemCount();
    auto letterTape = tapes.hasItem(static_cast<ItemId>(528));
    auto journalTape = tapes.hasItem(static_cast<ItemId>(529));
    test.startTaping();

    stepOntoIndoorEventPlate(game, 51); // The hot floor.
    game.tick(40); // Two game minutes, the burn timer fires every minute.
    stepOntoIndoorEventPlate(game, 53); // The cool floor.
    game.tick(40);

    clickIndoorEventFace(game, 62); // The bookcase.

    const std::pair<int, int> chests[] = {{44, 1}, {45, 2}, {46, 3}, {47, 4}, {49, 6}, {50, 7}};
    for (auto [eventId, chestId] : chests) {
        openChest(game, eventId);
        const ItemId item = chestId == 2 ? static_cast<ItemId>(528) : chestId == 3 ? static_cast<ItemId>(529) : firstNonGoldItem(chestId);
        if (item != ITEM_NULL)
            takeChestItem(game, chestId, item);
        else
            leaveMM6CampaignChainHouse(game);
    }

    useIndoorEventFace(game, 90); // The way out.
    confirmTransition(game);

    EXPECT_EQ(mapTape, tape(d18, oute3));
    EXPECT_EQ(burningTape, tape(false, true, false));
    EXPECT_CONTAINS(statusTape, "Found something!");
    EXPECT_EQ(letterTape, tape(false, true));
    EXPECT_EQ(journalTape, tape(false, true));
    EXPECT_EQ(itemCountTape.delta(), 5); // The bookcase's item, the letter, the journal scrap and two more chest items.
    // Every character takes the timer's 25 fire damage at least once while the party stands on the hot floor.
    for (int i = 0; i < 4; i++)
        EXPECT_LT(std::ranges::min(hpTape, {}, [i](const auto &hps) { return hps[i]; })[i], 2000) << i;
}
