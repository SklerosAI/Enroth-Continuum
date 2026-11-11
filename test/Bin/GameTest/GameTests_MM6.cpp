#include <algorithm>
#include <array>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Data/TileEnumFunctions.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Evt/MM6MapByteState.h"
#include "Engine/Evt/MM6EventVariables.h"
#include "Engine/Evt/MM6SpriteEvent.h"
#include "Engine/Evt/Processor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Graphics/Image.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/MM6/MM6Archive.h"
#include "Engine/MM7/MM7AtomicSaveFile.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6EncounterMonster.h"
#include "Engine/MM6/MM6BoundedInventoryProjection.h"
#include "Engine/MM6/MM6PlainItemProjection.h"
#include "Engine/MM6/MM6StandardItemProjection.h"
#include "Engine/MapEnumFunctions.h"
#include "Engine/mm7_data.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Objects/NPCEnumFunctions.h"
#include "Engine/Objects/Monsters.h"
#include "Engine/Party.h"
#include "Engine/PartyPlacement.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Resources/LOD.h"
#include "Engine/Resources/ResourceManager.h"
#include "Engine/SaveLoad.h"
#include "Engine/Snapshots/CompositeSnapshots.h"
#include "Engine/Snapshots/EntitySnapshots.h"
#include "Engine/Snapshots/MM6OutdoorSnapshot.h"
#include "Engine/Tables/IconFrameTable.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"
#include "Engine/Tables/TileTable.h"
#include "Engine/Timer.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIProgressBar.h"
#include "GUI/GUIWindow.h"
#include "GUI/HudFrameResources.h"
#include "GUI/HudNavigationResources.h"
#include "GUI/HudStatusResources.h"
#include "GUI/HudVitalBarResources.h"
#include "GUI/LoadingScreenResources.h"
#include "GUI/MM6HudActionControls.h"
#include "GUI/MM6HudBookControls.h"
#include "GUI/UI/Books/CalendarBackgroundResources.h"
#include "GUI/UI/MainMenuResources.h"
#include "GUI/UI/UIGame.h"

#include "Library/FileSystem/Memory/MemoryFileSystem.h"
#include "Library/Platform/Null/NullEventLoop.h"
#include "Library/Platform/Null/NullPlatform.h"
#include "Library/Platform/Null/NullWindow.h"
#include "Library/Platform/Proxy/ProxyBase.h"

#include "Utility/Exception.h"

#include "GameTestOptions.h"
#include "MM6HeadlessRules.h"

namespace {
template<class T>
T *platformLeaf(T *value) {
    // PlatformApplication exposes a root proxy. Components installed by
    // GameStarter add more proxies before the final platform object.
    for (int depth = 0; depth < 64 && value; ++depth) {
        auto *proxy = dynamic_cast<ProxyBase<T> *>(value);
        if (!proxy)
            return value;
        value = proxy->base();
    }
    return nullptr; // Reject a missing leaf or malformed/cyclic chain.
}
} // namespace

GAME_TEST(MM6Headless, StartupAndChestGuard) {
    EXPECT_EQ(engine->gameTitle(), GameTitle::MM6);
    EXPECT_FALSE(mm7::startupNativeProfileRoot());
    EXPECT_NE(dynamic_cast<NullPlatform *>(platformLeaf(platform)), nullptr);
    EXPECT_NE(dynamic_cast<NullEventLoop *>(platformLeaf(eventLoop)), nullptr);
    EXPECT_NE(dynamic_cast<NullWindow *>(platformLeaf(window)), nullptr);
    EXPECT_EQ(window->nativeHandle(), nullptr);
    EXPECT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    EXPECT_NE(dynamic_cast<MemoryFileSystem *>(ufs), nullptr);
    EXPECT_FALSE(ufs->exists("saves"));
    for (std::string_view name : mm6HeadlessRuleNames)
        EXPECT_TRUE(ufs->exists("rules/" + std::string(name)));

    // This is the first live-engine boundary, not a claim that the opening
    // map can load. Keep the existing MM6 guard closed until chest items are
    // staged and committed safely.
    EXPECT_THROW(mm6::generateChestItemsForTitle(GameTitle::MM6), std::runtime_error);
    game.tick(1);
}

GAME_TEST(MM6Headless, OriginalCharacterItemCapacity) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    EXPECT_EQ(CharacterInventory().capacity(), 126); // Protected MM7 default.
    pParty->Zero();
    for (Character &character : pParty->pCharacters)
        EXPECT_EQ(character.inventory.capacity(), 138);
    pParty->Reset();
    for (Character &character : pParty->pCharacters)
        EXPECT_EQ(character.inventory.capacity(), 138);

    Item item;
    item.itemId = static_cast<ItemId>(1);
    ASSERT_FALSE(pItemTable->items[item.itemId].iconName.empty());
    InventoryEntry restored = pParty->pCharacters[0].inventory.tryRestoreAt(137, Pointi(0, 0), item);
    ASSERT_TRUE(restored);
    EXPECT_EQ(restored.index(), 137);
    EXPECT_EQ(pParty->pCharacters[0].inventory.entry(137)->itemId, item.itemId);
    EXPECT_FALSE(pParty->pCharacters[0].inventory.entry(138));

    Character_MM7 inheritedSnapshot{};
    inheritedSnapshot.inventoryItems[0].itemId = 73;
    EXPECT_THROW(snapshot(pParty->pCharacters[0].inventory, &inheritedSnapshot), std::runtime_error);
    EXPECT_EQ(inheritedSnapshot.inventoryItems[0].itemId, 73);
    EXPECT_THROW(snapshot(pParty->pCharacters[0], &inheritedSnapshot), std::runtime_error);
    EXPECT_EQ(inheritedSnapshot.inventoryItems[0].itemId, 73);
    EXPECT_THROW(reconstruct(inheritedSnapshot, &pParty->pCharacters[0], tags::context(0)), std::runtime_error);
    EXPECT_EQ(pParty->pCharacters[0].inventory.entry(137)->itemId, item.itemId);
    EXPECT_THROW(reconstruct(inheritedSnapshot, &pParty->pCharacters[0].inventory, tags::context(0)), std::runtime_error);
    EXPECT_EQ(pParty->pCharacters[0].inventory.entry(137)->itemId, item.itemId);

    auto inheritedParty = std::make_unique<Party_MM7>();
    inheritedParty->partyHeight = 73;
    EXPECT_THROW(snapshot(*pParty, inheritedParty.get()), std::runtime_error);
    EXPECT_EQ(inheritedParty->partyHeight, 73);
    pParty->uNumGold = 4242;
    EXPECT_THROW(reconstruct(*inheritedParty, pParty), std::runtime_error);
    EXPECT_EQ(pParty->uNumGold, 4242);
    EXPECT_EQ(pParty->pCharacters[0].inventory.entry(137)->itemId, item.itemId);
}

GAME_TEST(MM6Headless, PlainItemProjectionUsesMM6TableTypes) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_TRUE(pItemTable);

    mm6::ItemRecord28 raw;
    raw.setWord(0, 221); // MM6 scroll in the inherited MM7 potion-ID range.
    auto scroll = mm6::stagePlainItem(raw, *pItemTable, GameTitle::MM6);
    ASSERT_TRUE(scroll);
    EXPECT_EQ(scroll->sourceType, ITEM_TYPE_SPELL_SCROLL);
    EXPECT_EQ(scroll->item.potionPower, 0);
    EXPECT_EQ(mm6::capturePlainItem(*scroll, *pItemTable, GameTitle::MM6), raw);

    raw.setWord(0, 164);
    auto bottle = mm6::stagePlainItem(raw, *pItemTable, GameTitle::MM6);
    ASSERT_TRUE(bottle);
    EXPECT_EQ(bottle->sourceType, ITEM_TYPE_POTION);
    raw.setWord(4, 12);
    EXPECT_FALSE(mm6::stagePlainItem(raw, *pItemTable, GameTitle::MM6));

    raw.setWord(0, 197);
    raw.setWord(4, 0);
    raw.setWord(12, 79);
    auto gold = mm6::stagePlainItem(raw, *pItemTable, GameTitle::MM6);
    ASSERT_TRUE(gold);
    EXPECT_EQ(gold->sourceType, ITEM_TYPE_GOLD);
    EXPECT_EQ(gold->item.goldAmount, 79);

    mm6::RawCharacterInventory inventoryRaw;
    inventoryRaw.items[137].setWord(0, 221);
    inventoryRaw.items[137][27] = 0xa5;
    const Sizei scrollSize = pItemTable->itemSizes[static_cast<ItemId>(221)];
    ASSERT_GT(scrollSize.w, 0);
    ASSERT_GT(scrollSize.h, 0);
    ASSERT_LE(scrollSize.w, 14);
    ASSERT_LE(scrollSize.h, 9);
    for (int y = 0; y < scrollSize.h; ++y)
        for (int x = 0; x < scrollSize.w; ++x)
            inventoryRaw.gridWords[y * 14 + x] = (x == 0 && y == 0) ? 138 : 0xffffffffu;
    auto projectedInventory = mm6::stageBoundedInventory(inventoryRaw, *pItemTable, GameTitle::MM6);
    ASSERT_TRUE(projectedInventory);
    EXPECT_EQ(projectedInventory->inventory().entry(137)->itemId, static_cast<ItemId>(221));
    EXPECT_EQ(mm6::captureBoundedInventory(*projectedInventory, *pItemTable, GameTitle::MM6), inventoryRaw);
}

GAME_TEST(MM6Headless, StandardItemProjectionUsesMM6ArmorTable) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_TRUE(pItemTable);
    ItemId armor = ITEM_NULL;
    for (ItemId id : pItemTable->items.indices()) {
        if (std::to_underlying(id) <= 580 && pItemTable->items[id].type == ITEM_TYPE_ARMOUR &&
            !pItemTable->items[id].iconName.empty()) {
            armor = id;
            break;
        }
    }
    ASSERT_NE(armor, ITEM_NULL);

    mm6::ItemRecord28 raw;
    raw.setWord(0, std::to_underlying(armor));
    raw.setWord(4, 12); // MM6 electricity -> runtime air resistance.
    raw.setWord(8, 7);
    raw.setWord(20, 1);
    raw[26] = 0x5a;
    auto projection = mm6::stageStandardItem(raw, *pItemTable, GameTitle::MM6);
    ASSERT_TRUE(projection);
    ASSERT_TRUE(projection->item.standardEnchantment);
    EXPECT_EQ(*projection->item.standardEnchantment, ATTRIBUTE_RESIST_AIR);
    EXPECT_EQ(mm6::captureStandardItem(*projection, *pItemTable, GameTitle::MM6), raw);
    projection->item.standardEnchantment = ATTRIBUTE_RESIST_BODY;
    projection->item.standardEnchantmentStrength = 19;
    auto changed = mm6::captureStandardItem(*projection, *pItemTable, GameTitle::MM6);
    ASSERT_TRUE(changed);
    EXPECT_EQ(changed->bonusRaw(), 14);
    EXPECT_EQ(changed->bonusStrengthRaw(), 19);
    EXPECT_EQ((*changed)[26], 0x5a);

    mm6::RawCharacterInventory inventoryRaw;
    inventoryRaw.items[136] = raw;
    inventoryRaw.equipmentWords[3] = 137;
    auto inventory = mm6::stageBoundedInventory(inventoryRaw, *pItemTable, GameTitle::MM6);
    ASSERT_TRUE(inventory);
    EXPECT_EQ(inventory->inventory().entry(ITEM_SLOT_ARMOUR).index(), 136);
    EXPECT_EQ(mm6::captureBoundedInventory(*inventory, *pItemTable, GameTitle::MM6), inventoryRaw);
    inventory->inventory().entry(136)->standardEnchantment = ATTRIBUTE_RESIST_BODY;
    inventory->inventory().entry(136)->standardEnchantmentStrength = 19;
    auto changedInventory = mm6::captureBoundedInventory(*inventory, *pItemTable, GameTitle::MM6);
    ASSERT_TRUE(changedInventory);
    EXPECT_EQ(changedInventory->items[136].bonusRaw(), 14);
    EXPECT_EQ(changedInventory->items[136].bonusStrengthRaw(), 19);
    EXPECT_EQ(changedInventory->items[136][26], 0x5a);
}

GAME_TEST(MM6Headless, FirstOutdoorResourcePreflight) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const MapId firstMap = pMapTable->GetMapInfo("oute3.odm");
    ASSERT_NE(firstMap, MAP_INVALID);
    EXPECT_TRUE(isMapOutdoor(GameTitle::MM6, firstMap));
    EXPECT_FALSE(isMapIndoor(GameTitle::MM6, firstMap));
    EXPECT_FALSE(isHirelingsBlockedOnMap(GameTitle::MM6, firstMap));
    EXPECT_EQ(pMapTable->pInfos[firstMap].fileName, "oute3.odm");
    ASSERT_TRUE(pGames_LOD->exists("oute3.odm"));
    EXPECT_TRUE(pGames_LOD->exists("oute3.ddm"));
    EXPECT_GT(engine->resources()->eventsData("oute3.evt").size(), 0);
    EXPECT_GT(engine->resources()->eventsData("oute3.str").size(), 0);
    EvtProgram events = EvtProgram::load(engine->resources()->eventsData("oute3.evt"), GameTitle::MM6);
    EXPECT_EQ(events.enumerateTriggers(EVENT_OnMapReload).size(), 11);
    EXPECT_EQ(engine->resources()->iconSize("oute3"), Sizei(512, 512));

    const LoadingScreenResources loading = fullscreenLoadingResources(GameTitle::MM6, 0);
    EXPECT_EQ(engine->resources()->iconSize(loading.background), Sizei(640, 480));
    EXPECT_TRUE(loading.progress.empty());
    EXPECT_EQ(engine->resources()->iconSize("bardata"), Sizei(300, 100));
    EXPECT_EQ(pIconsFrameTable->animationId("turnhour"), -1);

    OutdoorLocation_MM7 base;
    deserializeMM6Outdoor(decodeMM6ArchiveEntry(pGames_LOD->read("oute3.odm")), &base);
    size_t reloadSpriteActions = 0;
    size_t reloadFacetTextures = 0;
    for (const EventTrigger trigger : events.enumerateTriggers(EVENT_OnMapReload)) {
        const EvtInstruction &comparison = events.instruction(trigger.eventId, trigger.eventStep + 1);
        ASSERT_EQ(comparison.opcode, EVENT_Compare);
        const int rawVariable = std::to_underlying(comparison.data.variable_descr.type);
        EXPECT_TRUE(rawVariable == 16 ||
                    describeMM6Variable(rawVariable).family == MM6VariableFamily::MapByte);
        const EvtInstruction &branch = events.instruction(trigger.eventId, comparison.target_step);
        EXPECT_TRUE(branch.opcode == EVENT_SetSprite || branch.opcode == EVENT_ShowMovie);
        for (const EvtInstruction &instruction : events.function(trigger.eventId)) {
            if (instruction.opcode == EVENT_SetSprite) {
                EXPECT_GE(instruction.data.sprite_texture_descr.cog, 0);
                EXPECT_LT(static_cast<size_t>(instruction.data.sprite_texture_descr.cog), base.decorations.size());
                ++reloadSpriteActions;
            } else if (instruction.opcode == EVENT_ShowMovie) {
                // Opcode 12 is MM6 outdoor SetTexture, not the MM7 movie opcode.
                ASSERT_EQ(instruction.gameTitle, GameTitle::MM6);
                const int model = instruction.data.mm6_outdoor_texture_descr.model;
                const int facet = instruction.data.mm6_outdoor_texture_descr.facet;
                ASSERT_GE(model, 0);
                ASSERT_LT(static_cast<size_t>(model), base.modelExtras.size());
                ASSERT_GE(facet, 0);
                EXPECT_LT(static_cast<size_t>(facet), base.modelExtras[model].faces.size());
                EXPECT_NE(engine->resources()->bitmapSize(instruction.str), Sizei());
                ++reloadFacetTextures;
            }
        }
    }
    EXPECT_GT(reloadSpriteActions, 0);
    EXPECT_GT(reloadFacetTextures, 0);
}

GAME_TEST(MM6Headless, MapClassificationMatchesMapFiles) {
    // MM6 outdoor maps 7 and 8 were classified as indoor and took the BLV loader.
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    int outdoor = 0;
    int indoor = 0;
    for (MapId map : pMapTable->pInfos.indices()) {
        const std::string &fileName = pMapTable->pInfos[map].fileName;
        EXPECT_EQ(isMapOutdoor(engine->gameTitle(), map), fileName.ends_with(".odm")) << std::to_underlying(map);
        EXPECT_EQ(isMapIndoor(engine->gameTitle(), map), fileName.ends_with(".blv")) << std::to_underlying(map);
        outdoor += fileName.ends_with(".odm");
        indoor += fileName.ends_with(".blv");
    }
    EXPECT_EQ(outdoor, 15);
    EXPECT_EQ(indoor, 52);
    EXPECT_EQ(pMapTable->pInfos[static_cast<MapId>(7)].fileName, "outc1.odm");
    EXPECT_EQ(pMapTable->pInfos[static_cast<MapId>(8)].fileName, "outc2.odm");
}

GAME_TEST(MM6Headless, FirstOutdoorTileSelectors) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    auto base = std::make_unique<OutdoorLocation_MM7>();
    deserializeMM6Outdoor(decodeMM6ArchiveEntry(pGames_LOD->read("oute3.odm")), base.get());
    for (const OutdoorTileType_MM7 &selector : base->tileTypes) {
        ASSERT_LT(static_cast<size_t>(selector.tileId), pTileTable->tileCount());
        EXPECT_EQ(pTileTable->tile(selector.tileId).sourceTileset, static_cast<uint16_t>(selector.tileset));
    }

    auto mm6Terrain = std::make_unique<OutdoorTerrain>();
    auto inheritedTerrain = std::make_unique<OutdoorTerrain>();
    reconstruct(*base, mm6Terrain.get(), GameTitle::MM6);
    reconstruct(*base, inheritedTerrain.get(), GameTitle::MM7);
    EXPECT_TRUE(mm6Terrain->containsGrid({0, 0}));
    EXPECT_FALSE(mm6Terrain->containsGrid({-1, 0}));
    EXPECT_FALSE(mm6Terrain->containsGrid({127, 0}));
    EXPECT_EQ(inheritedTerrain->tilesetByGrid({-1, 0}), TILESET_INVALID);

    size_t inheritedPending = 0;
    size_t mm6Pending = 0;
    std::vector<int> authoredIds;
    std::vector<int> inheritedIds;
    authoredIds.reserve(127 * 127);
    inheritedIds.reserve(127 * 127);
    for (int y = 0; y < 127; ++y) {
        for (int x = 0; x < 127; ++x) {
            inheritedPending += pTileTable->tile(inheritedTerrain->tileIdByGrid({x, y})).textureName == "pending";
            mm6Pending += pTileTable->tile(mm6Terrain->tileIdByGrid({x, y})).textureName == "pending";
            authoredIds.push_back(mm6Terrain->tileIdByGrid({x, y}));
            inheritedIds.push_back(inheritedTerrain->tileIdByGrid({x, y}));
        }
    }
    EXPECT_EQ(inheritedPending, 209u);
    EXPECT_EQ(mm6Pending, 0u);

    mm6Terrain->changeSeason(6, GameTitle::MM6);
    size_t postSeasonPending = 0;
    size_t summerChanges = 0;
    for (int y = 0; y < 127; ++y)
        for (int x = 0; x < 127; ++x) {
            postSeasonPending += pTileTable->tile(mm6Terrain->tileIdByGrid({x, y})).textureName == "pending";
            summerChanges += mm6Terrain->tileIdByGrid({x, y}) != authoredIds[y * 127 + x];
        }
    EXPECT_EQ(postSeasonPending, 0u);
    EXPECT_EQ(summerChanges, 0u);

    mm6Terrain->changeSeason(0, GameTitle::MM6);
    size_t preservedMM6Only = 0;
    size_t changedGrass = 0;
    for (int y = 0; y < 127; ++y) {
        for (int x = 0; x < 127; ++x) {
            const int localId = base->tileMap[y * 128 + x];
            const int currentId = mm6Terrain->tileIdByGrid({x, y});
            if (localId >= 162 && localId < 234) {
                EXPECT_EQ(currentId, authoredIds[y * 127 + x]);
                ++preservedMM6Only;
            } else if (localId >= 90 && localId < 126) {
                changedGrass += currentId != authoredIds[y * 127 + x];
            }
        }
    }
    EXPECT_GT(preservedMM6Only, 0u);
    EXPECT_GT(changedGrass, 0u);

    inheritedTerrain->changeSeason(6, GameTitle::MM7);
    size_t inheritedSeasonChanges = 0;
    for (int y = 0; y < 127; ++y)
        for (int x = 0; x < 127; ++x)
            inheritedSeasonChanges += inheritedTerrain->tileIdByGrid({x, y}) != inheritedIds[y * 127 + x];
    EXPECT_GT(inheritedSeasonChanges, 0u);

    base->tileTypes[0].tileId = 0;
    EXPECT_THROW(reconstruct(*base, mm6Terrain.get(), GameTitle::MM6), std::exception);
    EXPECT_NO_THROW(reconstruct(*base, inheritedTerrain.get(), GameTitle::MM7));
    base->tileTypes[0].tileId = static_cast<uint16_t>(pTileTable->tileCount());
    EXPECT_THROW(reconstruct(*base, mm6Terrain.get(), GameTitle::MM6), std::exception);
    EXPECT_NO_THROW(reconstruct(*base, inheritedTerrain.get(), GameTitle::MM7));

    size_t outdoorCount = 0;
    size_t invalidTextures = 0;
    size_t outdoorSummerChanges = 0;
    for (const std::string &name : pGames_LOD->ls()) {
        if (!name.ends_with(".odm"))
            continue;
        auto location = std::make_unique<OutdoorLocation_MM7>();
        deserializeMM6Outdoor(decodeMM6ArchiveEntry(pGames_LOD->read(name)), location.get());
        auto terrain = std::make_unique<OutdoorTerrain>();
        reconstruct(*location, terrain.get(), GameTitle::MM6);
        std::vector<int> originalIds;
        originalIds.reserve(127 * 127);
        for (int y = 0; y < 127; ++y) {
            for (int x = 0; x < 127; ++x) {
                const int tileId = terrain->tileIdByGrid({x, y});
                ASSERT_GE(tileId, 0);
                ASSERT_LT(static_cast<size_t>(tileId), pTileTable->tileCount());
                const std::string &texture = pTileTable->tile(tileId).textureName;
                invalidTextures += texture.empty() || texture == "pending";
                originalIds.push_back(tileId);
            }
        }
        terrain->changeSeason(6, GameTitle::MM6);
        for (int y = 0; y < 127; ++y)
            for (int x = 0; x < 127; ++x)
                outdoorSummerChanges += terrain->tileIdByGrid({x, y}) != originalIds[y * 127 + x];
        ++outdoorCount;
    }
    EXPECT_EQ(outdoorCount, 15u);
    EXPECT_EQ(invalidTextures, 0u);
    EXPECT_EQ(outdoorSummerChanges, 0u);
}

TEST(MM6OutdoorWalkSounds, RawIdentityAndTitleIsolation) {
    TileData tile;
    tile.tileset = TILESET_DIRT;
    size_t covered = 0;
    const auto checkGroup = [&](std::initializer_list<int> rawIds, SoundId walk, SoundId run) {
        for (int rawId : rawIds) {
            tile.sourceTileset = rawId;
            EXPECT_EQ(walkSoundForTile(GameTitle::MM6, tile, false), walk) << rawId;
            EXPECT_EQ(walkSoundForTile(GameTitle::MM6, tile, true), run) << rawId;
            ++covered;
        }
    };
    checkGroup({0, 8}, SOUND_WalkGrass, SOUND_RunGrass);
    checkGroup({1}, SOUND_WalkSnow, SOUND_RunSnow);
    checkGroup({2}, SOUND_WalkDesert, SOUND_RunDesert);
    checkGroup({3}, SOUND_WalkCooledLava, SOUND_RunCooledLava);
    checkGroup({4, 9, 18, 19, 20, 21}, SOUND_WalkGround, SOUND_RunGround);
    checkGroup({5}, SOUND_WalkWater, SOUND_RunWater);
    checkGroup({6}, SOUND_WalkBadlands, SOUND_RunBadlands);
    checkGroup({7}, SOUND_WalkSwamp, SOUND_RunSwamp);
    checkGroup({10, 11, 12, 13, 14, 15, 16, 17, 22, 23, 24, 25, 26, 27}, SOUND_WalkRoad, SOUND_RunRoad);
    EXPECT_EQ(covered, 28u);

    tile.sourceTileset = 3;
    EXPECT_EQ(walkSoundForTile(GameTitle::MM6, tile, false), SOUND_WalkCooledLava);
    EXPECT_EQ(walkSoundForTile(GameTitle::MM6, tile, true), SOUND_RunCooledLava);
    EXPECT_EQ(walkSoundForTile(GameTitle::MM7, tile, false), SOUND_WalkDirt);

    tile.sourceTileset = 22;
    EXPECT_EQ(walkSoundForTile(GameTitle::MM6, tile, false), SOUND_WalkRoad);
    EXPECT_EQ(walkSoundForTile(GameTitle::MM6, tile, true), SOUND_RunRoad);
    EXPECT_EQ(walkSoundForTile(GameTitle::MM7, tile, true), SOUND_RunDirt);

    tile.sourceTileset = 4;
    EXPECT_EQ(walkSoundForTile(GameTitle::MM6, tile, false), SOUND_WalkGround);
    tile.sourceTileset = 8;
    EXPECT_EQ(walkSoundForTile(GameTitle::MM6, tile, false), SOUND_WalkGrass);
    tile.sourceTileset = 27;
    EXPECT_EQ(walkSoundForTile(GameTitle::MM6, tile, false), SOUND_WalkRoad);
    tile.sourceTileset = 28;
    EXPECT_EQ(walkSoundForTile(GameTitle::MM6, tile, false), SOUND_Invalid);
    tile.sourceTileset = 255;
    EXPECT_EQ(walkSoundForTile(GameTitle::MM6, tile, true), SOUND_Invalid);

    TileData_MM7 wire{};
    wire.tileset = TILESET_MM7_ROAD_CRACKED_COBBLE;
    wire.variant = TILE_VARIANT_MM7_BASE1_NSEW;
    reconstruct(wire, &tile);
    EXPECT_EQ(tile.tileset, TILESET_DIRT);
    EXPECT_EQ(tile.sourceTileset, 22);
    EXPECT_EQ(tile.sourceVariant, 0);
}

TEST(MM6HeadlessFirstOutdoorSpriteEvents, IndexAndTitleIsolation) {
    std::vector<LevelDecoration> decorations(3);
    decorations[0].uCog = 1;
    decorations[1].uCog = 0;
    decorations[2].uCog = 1;
    decorations[1].uFlags |= LEVEL_DECORATION_INVISIBLE;

    EXPECT_FALSE(mm6::applySetSprite(GameTitle::MM7, decorations, 1, true, std::nullopt));
    EXPECT_TRUE(decorations[1].uFlags & LEVEL_DECORATION_INVISIBLE);
    EXPECT_TRUE(mm6::applySetSprite(GameTitle::MM6, decorations, 1, true, std::nullopt));
    EXPECT_FALSE(decorations[1].uFlags & LEVEL_DECORATION_INVISIBLE);
    EXPECT_TRUE(mm6::applySetSprite(GameTitle::MM6, decorations, 2, false, std::nullopt));
    EXPECT_TRUE(decorations[2].uFlags & LEVEL_DECORATION_INVISIBLE);
    EXPECT_FALSE(decorations[0].uFlags & LEVEL_DECORATION_INVISIBLE);
    EXPECT_TRUE(mm6::applySetSprite(GameTitle::MM6, decorations, 1, true,
                                    DecorationId::DECORATION_FOUNTAIN_3));
    EXPECT_EQ(decorations[1].uDecorationDescID, DecorationId::DECORATION_FOUNTAIN_3);
    EXPECT_FALSE(mm6::applySetSprite(GameTitle::MM6, decorations, -1, false, std::nullopt));
    EXPECT_FALSE(mm6::applySetSprite(GameTitle::MM6, decorations, 3, false, std::nullopt));
}

GAME_TEST(MM6Headless, FirstOutdoorEncounterNames) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const MapId firstMap = pMapTable->GetMapInfo("oute3.odm");
    ASSERT_NE(firstMap, MAP_INVALID);
    const MapData &map = pMapTable->pInfos[firstMap];
    for (const std::string &family : {map.encounter1MonsterInternalName,
                                      map.encounter2MonsterInternalName}) {
        ASSERT_FALSE(family.empty());
        ASSERT_NE(family, "0");
        for (char rank : {'A', 'B', 'C'}) {
            const MonsterId id = mm6::resolveEncounterMonster(
                GameTitle::MM6, *pMonsterStats, family + " " + rank);
            ASSERT_NE(id, MONSTER_INVALID);
            EXPECT_EQ(pMonsterStats->infos[id].id, id);
            EXPECT_FALSE(pMonsterList->monsters[id].internalMonsterName.empty());
        }
    }

    // Another retained map uses a family label with different word spacing
    // from the display name in the MM6 monster table.
    const MapId laterMap = pMapTable->GetMapInfo("outd2.odm");
    ASSERT_NE(laterMap, MAP_INVALID);
    const MonsterId laterMonster = mm6::resolveEncounterMonster(
        GameTitle::MM6, *pMonsterStats,
        pMapTable->pInfos[laterMap].encounter3MonsterInternalName + " A");
    ASSERT_NE(laterMonster, MONSTER_INVALID);
    EXPECT_FALSE(pMonsterList->monsters[laterMonster].internalMonsterName.empty());
}

GAME_TEST(MM6Headless, FirstOutdoorPreparation) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    const MapId firstMap = pMapTable->GetMapInfo("oute3.odm");
    ASSERT_NE(firstMap, MAP_INVALID);

    // Seed only default original deltas into the in-memory test owner. The
    // installed game/profile and durable save path are never written here.
    pMapDeltas.clear();
    for (const std::string &name : pGames_LOD->ls()) {
        if (std::string_view(name).ends_with(".dlv") || std::string_view(name).ends_with(".ddm"))
            pMapDeltas.emplace(name, Blob::copy(pGames_LOD->read(name)));
    }
    ASSERT_TRUE(pMapDeltas.contains("oute3.ddm"));

    bool completed = false;
    game.runGameRoutine([firstMap, &completed] {
        pParty->Reset();
        engine->_currentLoadedMapId = firstMap;
        // DoPrepareWorld normally sizes these AI scratch arrays before it
        // calls loadAndPrepareODM. This focused probe enters at that later
        // seam and must establish the same precondition.
        const int actorLimit = engine->config->gameplay.MaxActors.value();
        ai_near_actors_targets_pid.resize(actorLimit, Pid());
        ai_near_actors_ids.resize(actorLimit);
        pGameLoadingUI_ProgressBar->Initialize(GUIProgressBar::TYPE_Fullscreen);
        loadAndPrepareODM(firstMap, /* bLoading = */ true);
        completed = true;
    });

    // The loading screen presents several frames from inside this game
    // routine. Each present yields to the test controller; resume until the
    // routine itself completes, with a bound for a stalled loading path.
    for (int tick = 0; tick < 128 && !completed; ++tick)
        game.tick();
    ASSERT_TRUE(completed);

    EXPECT_EQ(uCurrentlyLoadedLevelType, LEVEL_OUTDOOR);
    EXPECT_EQ(engine->_currentLoadedMapId, firstMap);
    EXPECT_FALSE(pOutdoor->faces.empty());
    EXPECT_FALSE(pOutdoor->pSpawnPoints.empty());
    EXPECT_EQ(window->nativeHandle(), nullptr);
}

// World preparation mutates process-global owners without a complete rollback.
// Run only in a dedicated GameTest process with --gtest_also_run_disabled_tests.
GAME_TEST(MM6Headless, DISABLED_ScopedFirstOutdoorWorld) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    // MainMenuState releases its owned window on this transition. The party
    // creation window is then owned by pGUIWindow_CurrentMenu, which the
    // normal world reset deletes before checking for one primary window.
    ASSERT_NE(pGUIWindow_CurrentMenu, nullptr);
    ASSERT_EQ(pGUIWindow_CurrentMenu->eWindowType, WINDOW_PARTY_CREATION);
    ASSERT_EQ(lWindowList.size(), 2u);
    const MapId firstMap = pMapTable->GetMapInfo("oute3.odm");
    ASSERT_NE(firstMap, MAP_INVALID);

    pMapDeltas.clear();
    for (const std::string &name : pGames_LOD->ls()) {
        if (std::string_view(name).ends_with(".dlv") || std::string_view(name).ends_with(".ddm"))
            pMapDeltas.emplace(name, Blob::copy(pGames_LOD->read(name)));
    }
    ASSERT_TRUE(pMapDeltas.contains("oute3.ddm"));
    mm6::resetChestTitleState(&mm6::chestTitleState());
    ASSERT_TRUE(mm6::restoreChestTitleState(GameTitle::MM6, &mm6::chestTitleState(),
                                            0x12345678u, {}));

    bool completed = false;
    game.runGameRoutine([firstMap, &completed] {
        engine->_pendingTransition = MapDestination(firstMap, MAP_START_POINT_PARTY);
        {
            mm6::ScopedChestMapLoadOptIn chestOptIn;
            DoPrepareWorld(/* bLoading = */ false, /* fullscreen = */ 1);
        }
        completed = !mm6::chestMapLoadOptedIn();
    });
    for (int tick = 0; tick < 256 && !completed; ++tick)
        game.tick();
    ASSERT_TRUE(completed);

    EXPECT_EQ(engine->_currentLoadedMapId, firstMap);
    EXPECT_EQ(uCurrentlyLoadedLevelType, LEVEL_OUTDOOR);
    EXPECT_FALSE(engine->_pendingTransition.has_value());
    EXPECT_FALSE(pOutdoor->faces.empty());
    EXPECT_FALSE(pOutdoor->pSpawnPoints.empty());
    EXPECT_FALSE(pActors.empty());
    EXPECT_EQ(vChests.size(), 20u);
    EXPECT_EQ(lWindowList.size(), 1u);
    EXPECT_EQ(window->nativeHandle(), nullptr);
}

// This is a bounded post-load subsystem step, not Game::gameLoop. Run alone in
// a fresh process because DoPrepareWorld has no complete global rollback.
GAME_TEST(MM6Headless, DISABLED_FirstOutdoorSubsystemStep) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_EQ(window->nativeHandle(), nullptr);
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_NE(pGUIWindow_CurrentMenu, nullptr);
    ASSERT_EQ(pGUIWindow_CurrentMenu->eWindowType, WINDOW_PARTY_CREATION);

    const MapId firstMap = pMapTable->GetMapInfo("oute3.odm");
    ASSERT_NE(firstMap, MAP_INVALID);
    pMapDeltas.clear();
    for (const std::string &name : pGames_LOD->ls()) {
        if (std::string_view(name).ends_with(".dlv") || std::string_view(name).ends_with(".ddm"))
            pMapDeltas.emplace(name, Blob::copy(pGames_LOD->read(name)));
    }
    ASSERT_TRUE(pMapDeltas.contains("oute3.ddm"));
    mm6::resetChestTitleState(&mm6::chestTitleState());
    ASSERT_TRUE(mm6::restoreChestTitleState(GameTitle::MM6, &mm6::chestTitleState(),
                                            0x12345678u, {}));

    bool prepared = false;
    game.runGameRoutine([firstMap, &prepared] {
        // This is normally called by Game::gameLoop before DoPrepareWorld.
        GameUI_LoadPlayerPortraitsAndVoices();
        engine->_pendingTransition = MapDestination(firstMap, MAP_START_POINT_PARTY);
        {
            mm6::ScopedChestMapLoadOptIn chestOptIn;
            DoPrepareWorld(/* bLoading = */ false, /* fullscreen = */ 1);
        }
        prepared = !mm6::chestMapLoadOptedIn();
    });
    for (int tick = 0; tick < 256 && !prepared; ++tick)
        game.tick();
    ASSERT_TRUE(prepared);
    ASSERT_EQ(engine->_currentLoadedMapId, firstMap);
    ASSERT_EQ(uCurrentlyLoadedLevelType, LEVEL_OUTDOOR);
    ASSERT_FALSE(pActors.empty());
    ASSERT_FALSE(pLevelDecorations.empty());

    // Select a reload branch whose source compares an MM6 map byte and then
    // sets a bounded decoration. Force the opposite visibility before replay
    // so the assertion proves that the interpreter ran this event.
    int variableId = -1;
    int variableValue = -1;
    int decorationIndex = -1;
    bool expectedVisible = false;
    for (const EventTrigger trigger : engine->_localEventMap.enumerateTriggers(EVENT_OnMapReload)) {
        const EvtInstruction &comparison = engine->_localEventMap.instruction(trigger.eventId,
                                                                               trigger.eventStep + 1);
        if (comparison.opcode != EVENT_Compare ||
            describeMM6Variable(std::to_underlying(comparison.data.variable_descr.type)).family !=
                MM6VariableFamily::MapByte)
            continue;
        const EvtInstruction &branch = engine->_localEventMap.instruction(trigger.eventId,
                                                                           comparison.target_step);
        if (branch.opcode != EVENT_SetSprite || branch.data.sprite_texture_descr.cog < 0 ||
            static_cast<size_t>(branch.data.sprite_texture_descr.cog) >= pLevelDecorations.size())
            continue;
        variableId = std::to_underlying(comparison.data.variable_descr.type);
        variableValue = comparison.data.variable_descr.value;
        decorationIndex = branch.data.sprite_texture_descr.cog;
        expectedVisible = branch.data.sprite_texture_descr.hide;
        break;
    }
    ASSERT_GE(variableId, 0);
    ASSERT_GE(decorationIndex, 0);
    game.runGameRoutine([=] {
        MM6MapByteState(pOutdoor->mm6DeltaBytes).set(variableId, variableValue);
        LevelDecoration &decoration = pLevelDecorations[decorationIndex];
        if (expectedVisible)
            decoration.uFlags |= LEVEL_DECORATION_INVISIBLE;
        else
            decoration.uFlags &= ~LEVEL_DECORATION_INVISIBLE;
        onMapLoad();
    });
    EXPECT_EQ(!(pLevelDecorations[decorationIndex].uFlags & LEVEL_DECORATION_INVISIBLE), expectedVisible);

    const Vec3f initialPos = pParty->pos;
    const int initialYaw = pParty->_viewYaw;
    game.runGameRoutine([] { gameTimer->setPaused(false); });
    game.tick(2); // Advance the null-platform clock while the parent loop remains in party creation.
    game.runGameRoutine([] {
        gameTimer->tick();
        current_screen_type = SCREEN_GAME;
        onTimer();
        Actor::UpdateActorAI();
        pPartyActionQueue->Reset();
        pPartyActionQueue->Add(PARTY_TurnLeft);
        pPartyActionQueue->Add(PARTY_WalkForward);
        UpdateUserInput_and_MapSpecificStuff();
        gameTimer->setPaused(true);
    });
    EXPECT_NE(pParty->_viewYaw, initialYaw);
    EXPECT_NE(pParty->pos, initialPos);
    EXPECT_EQ(pPartyActionQueue->uNumActions, 0u);
    EXPECT_FALSE(engine->_pendingTransition.has_value());
    EXPECT_EQ(window->nativeHandle(), nullptr);
}

TEST(MM6NpcProfessionChance, ParsesGlobalWeightsAndRejectsBadTables) {
    NPCProfessionChance chance = parseMm6ProfessionChance(Blob::fromString(
        "id\tname\tchance\r\n1\tFirst\t2\r\n2\tSecond\t0\r\n3\tThird\t5\r\n77\tTerminal\t??\r\n"));
    EXPECT_EQ(chance.total, 7);
    EXPECT_EQ(chance.chanceByProfession[NPC_PROFESSION_SMITH], 2);
    EXPECT_EQ(chance.chanceByProfession[NPC_PROFESSION_ARMORER], 0);
    EXPECT_EQ(chance.chanceByProfession[NPC_PROFESSION_ALCHEMIST], 5);
    EXPECT_EQ(chance.chanceByProfession[NPC_PROFESSION_MM6_LAST], 0);
    EXPECT_EQ(selectAdditionalNpcProfession(chance, GameTitle::MM6, 0), NPC_PROFESSION_SMITH);
    EXPECT_EQ(selectAdditionalNpcProfession(chance, GameTitle::MM6, 1), NPC_PROFESSION_SMITH);
    EXPECT_EQ(selectAdditionalNpcProfession(chance, GameTitle::MM6, 2), NPC_PROFESSION_ALCHEMIST);
    EXPECT_EQ(selectAdditionalNpcProfession(chance, GameTitle::MM6, 6), NPC_PROFESSION_ALCHEMIST);
    EXPECT_THROW(selectAdditionalNpcProfession(chance, GameTitle::MM6, 7), std::exception);
    EXPECT_THROW(parseMm6ProfessionChance(Blob::fromString("1\tFirst\t??\r\n")), std::exception);
    EXPECT_THROW(parseMm6ProfessionChance(Blob::fromString("1\tFirst\t0\r\n")), std::exception);
    EXPECT_THROW(parseMm6ProfessionChance(Blob::fromString("1\tFirst\t2\r\n1\tAgain\t3\r\n")),
                 std::exception);

    NPCProfessionChance mm6Only;
    mm6Only.chanceByProfession[static_cast<NpcProfession>(60)] = 1;
    mm6Only.total = 1;
    EXPECT_EQ(selectAdditionalNpcProfession(mm6Only, GameTitle::MM6, 0), static_cast<NpcProfession>(60));
    EXPECT_THROW(selectAdditionalNpcProfession(mm6Only, GameTitle::MM7, 0), std::exception);
}

TEST(MM6NpcProfessionChance, Mm7DistributionRemainsPerMap) {
    auto stats = std::make_unique<NPCStats>();
    stats->InitializeNPCDist(Blob::fromString(
        "header\r\nheader\r\nFirst\t3\t7\r\n"));
    EXPECT_EQ(stats->pProfessionChance[MAP_EMERALD_ISLAND].total, 3);
    EXPECT_EQ(stats->pProfessionChance[MAP_HARMONDALE].total, 7);
    EXPECT_EQ(stats->pProfessionChance[MAP_EMERALD_ISLAND].chanceByProfession[NPC_PROFESSION_SMITH], 3);
    EXPECT_EQ(stats->pProfessionChance[MAP_HARMONDALE].chanceByProfession[NPC_PROFESSION_SMITH], 7);
}

GAME_TEST(MM6Headless, ProfessionChanceAvailableForFirstMap) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const MapId firstMap = pMapTable->GetMapInfo("oute3.odm");
    ASSERT_NE(firstMap, MAP_INVALID);
    const NPCProfessionChance &chance = pNPCStats->pProfessionChance[firstMap];
    ASSERT_GT(chance.total, 0);
    int sum = 0;
    for (NpcProfession profession : allMm6RandomNpcProfessions()) {
        ASSERT_GE(chance.chanceByProfession[profession], 0);
        sum += chance.chanceByProfession[profession];
    }
    EXPECT_EQ(sum, chance.total);
    EXPECT_EQ(chance.chanceByProfession[NPC_PROFESSION_MM6_LAST], 0);
    EXPECT_EQ(pNPCStats->pProfessionChance[MAP_EMERALD_ISLAND].total, chance.total);
    for (int draw : {0, chance.total - 1}) {
        NpcProfession profession = selectAdditionalNpcProfession(chance, GameTitle::MM6, draw);
        EXPECT_GE(std::to_underlying(profession), std::to_underlying(NPC_PROFESSION_FIRST));
        EXPECT_LT(std::to_underlying(profession), std::to_underlying(NPC_PROFESSION_MM6_LAST));
        EXPECT_GT(chance.chanceByProfession[profession], 0);
    }
}

TEST(MM6EncounterMonster, ResolvesCompactAndDisplayFamiliesWithoutGuessing) {
    MonsterStats stats{};
    stats.infos[static_cast<MonsterId>(1)].name = "First";
    stats.infos[static_cast<MonsterId>(1)].internalName = "FamilyA";
    stats.infos[static_cast<MonsterId>(2)].name = "Two Words";
    stats.infos[static_cast<MonsterId>(2)].internalName = "FamilyB";
    stats.infos[static_cast<MonsterId>(3)].name = "Third";
    stats.infos[static_cast<MonsterId>(3)].internalName = "FamilyC";
    stats.infos[static_cast<MonsterId>(4)].name = "First";
    stats.infos[static_cast<MonsterId>(4)].internalName = "OtherA";

    EXPECT_EQ(mm6::resolveEncounterMonster(GameTitle::MM6, stats, "Family A"), static_cast<MonsterId>(1));
    EXPECT_EQ(mm6::resolveEncounterMonster(GameTitle::MM6, stats, "FamilyB"), static_cast<MonsterId>(2));
    EXPECT_EQ(mm6::resolveEncounterMonster(GameTitle::MM6, stats, "TwoWords C"), static_cast<MonsterId>(3));
    EXPECT_EQ(mm6::resolveEncounterMonster(GameTitle::MM6, stats, "First A"), MONSTER_INVALID);
    EXPECT_EQ(mm6::resolveEncounterMonster(GameTitle::MM6, stats, "Missing A"), MONSTER_INVALID);
    EXPECT_EQ(mm6::resolveEncounterMonster(GameTitle::MM7, stats, "Family A"), MONSTER_INVALID);
}

TEST(MM6HeadlessFirstOutdoorResources, TitleIsolation) {
    const LoadingScreenResources mm6 = fullscreenLoadingResources(GameTitle::MM6, 1);
    EXPECT_EQ(mm6.background, "loading.pcx");
    EXPECT_TRUE(mm6.progress.empty());
    const LoadingScreenResources mm7First = fullscreenLoadingResources(GameTitle::MM7, 1);
    const LoadingScreenResources mm7Last = fullscreenLoadingResources(GameTitle::MM7, 5);
    EXPECT_EQ(mm7First.background, "loading1.pcx");
    EXPECT_EQ(mm7Last.background, "loading5.pcx");
    EXPECT_EQ(mm7First.progress, "loadprog");
    EXPECT_EQ(mm7Last.progress, "loadprog");
}

TEST(MM6HeadlessFirstOutdoorMapId, HirelingRuleIsolation) {
    // MM6's first outdoor map has numeric ID 15, reused by MM7's Shoals.
    EXPECT_FALSE(isHirelingsBlockedOnMap(GameTitle::MM6, MAP_SHOALS));
    EXPECT_FALSE(isHirelingsBlockedOnMap(GameTitle::MM6, MAP_LINCOLN));
    EXPECT_TRUE(isHirelingsBlockedOnMap(GameTitle::MM7, MAP_SHOALS));
    EXPECT_TRUE(isHirelingsBlockedOnMap(GameTitle::MM7, MAP_LINCOLN));
    EXPECT_FALSE(isHirelingsBlockedOnMap(GameTitle::MM7, MAP_HARMONDALE));
}

GAME_TEST(MM6Headless, MainMenuResourcesAvailable) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const MainMenuResources resources = mainMenuResources(engine->gameTitle());
    Sizei background = engine->resources()->iconSize(resources.background);
    EXPECT_GT(background.w, 0);
    EXPECT_GT(background.h, 0);
    for (const MainMenuButtonResource &button : resources.buttons) {
        Sizei size = engine->resources()->iconSize(button.icon);
        EXPECT_GT(size.w, 0) << button.icon;
        EXPECT_GT(size.h, 0) << button.icon;
    }
}

GAME_TEST(MM6Headless, HudNavigationResourcesAvailable) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const HudNavigationResources resources = hudNavigationResources(engine->gameTitle(),
        "IB-COMP-A", "ib-autout-a", "ib-autin-a");
    for (std::string_view name : {resources.compass, resources.zoomIn, resources.zoomOut}) {
        Sizei size = engine->resources()->iconSize(name);
        EXPECT_GT(size.w, 0) << name;
        EXPECT_GT(size.h, 0) << name;
    }
}

GAME_TEST(MM6Headless, HudFrameResourcesAvailable) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const HudFrameAssets frames = hudFrameAssets(GameTitle::MM6, {}, {}, {}, {});
    const std::array<std::pair<std::string_view, Sizei>, 4> expected = {{
        {frames.right, {172, 339}}, {frames.bottom, {469, 109}},
        {frames.top, {468, 8}}, {frames.left, {8, 344}}
    }};
    for (const auto &[name, size] : expected)
        EXPECT_EQ(engine->resources()->iconSize(name), size) << name;
}

GAME_TEST(MM6Headless, StatusStripAndMapArrowsAvailable) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    EXPECT_EQ(engine->resources()->iconSize(hudStatusBarIcon(GameTitle::MM6, {})), Sizei(483, 24));
    // These eight inherited minimap directions already exist in the MM6
    // archive; the dynamic top-frame state remains a separate open mapping.
    for (int i = 1; i <= 8; ++i) {
        const std::string icon = "MAPDIR" + std::to_string(i);
        EXPECT_EQ(engine->resources()->iconSize(icon), Sizei(7, 7)) << icon;
    }
}

GAME_TEST(MM6Headless, PortraitVitalBarsAndHitBoxes) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const HudVitalBarResources bars = hudVitalBarResources(GameTitle::MM6, {}, {}, {}, {});
    EXPECT_EQ(engine->resources()->iconSize(bars.green), Sizei(6, 78));
    EXPECT_EQ(engine->resources()->iconSize(bars.blue), Sizei(6, 78));
    EXPECT_EQ(game_ui_bar_green->size(), Sizei(6, 78));
    EXPECT_EQ(game_ui_bar_blue->size(), Sizei(6, 78));
    EXPECT_EQ(game_ui_bar_yellow, nullptr);
    EXPECT_EQ(game_ui_bar_red, nullptr);

    // The headless startup fixture has not entered the game-screen UI.
    // Construct this button group explicitly, then let the window clean it up.
    GUIWindow buttons(WINDOW_NULL, {0, 0}, {640, 480});
    buttons.CreateCharacterButtons();
    std::vector<Recti> actual;
    for (GUIButton *button : buttons.vButtons)
        if (button->uData == UIMSG_ShowStatus_ManaHP)
            actual.push_back(button->rect);
    const auto health = mm6HudHealthBarRects();
    const auto mana = mm6HudManaBarRects();
    ASSERT_EQ(actual.size(), 8);
    for (size_t i = 0; i < health.size(); ++i) {
        EXPECT_EQ(actual[2 * i], health[i]);
        EXPECT_EQ(actual[2 * i + 1], mana[i]);
        EXPECT_EQ(health[i].y + health[i].h, 461);
        EXPECT_EQ(mana[i].y + mana[i].h, 461);
    }
}

TEST(MM6HeadlessVitalBars, TitleIsolation) {
    const HudVitalBarResources mm6 = hudVitalBarResources(GameTitle::MM6,
        "ib-statB", "ib-statG", "ib-statY", "ib-statR");
    EXPECT_EQ(mm6.blue, "manafull");
    EXPECT_EQ(mm6.green, "hitsfull");
    EXPECT_TRUE(mm6.yellow.empty());
    EXPECT_TRUE(mm6.red.empty());
    const HudVitalBarResources mm7 = hudVitalBarResources(GameTitle::MM7,
        "ib-statB", "ib-statG", "ib-statY", "ib-statR");
    EXPECT_EQ(mm7.blue, "ib-statB");
    EXPECT_EQ(mm7.green, "ib-statG");
    EXPECT_EQ(mm7.yellow, "ib-statY");
    EXPECT_EQ(mm7.red, "ib-statR");
    // MM6's 16-bit HUD draw uses row 383, base x 93/102 and character
    // offsets 0/113/226/338. Keep every slot exact, especially the fourth.
    constexpr std::array<int, 4> characterOffset = {0, 113, 226, 338};
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(mm6HudHealthBarRects()[i], Recti(93 + characterOffset[i], 383, 6, 78));
        EXPECT_EQ(mm6HudManaBarRects()[i], Recti(102 + characterOffset[i], 383, 6, 78));
    }
}

TEST(MM6HeadlessStatusStrip, TitleIsolation) {
    EXPECT_EQ(hudStatusBarIcon(GameTitle::MM6, "IB-Foot-a.pcx"), "footer");
    EXPECT_EQ(hudStatusBarIcon(GameTitle::MM7, "IB-Foot-a.pcx"), "IB-Foot-a.pcx");
    EXPECT_EQ(hudStatusBarIcon(GameTitle::MM7, "IB-Foot-b.pcx"), "IB-Foot-b.pcx");
    EXPECT_EQ(hudStatusBarIcon(GameTitle::MM7, "IB-Foot-c.pcx"), "IB-Foot-c.pcx");
}

GAME_TEST(MM6Headless, FourBookHitBoxesAndActions) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_NE(pPrimaryWindow, nullptr);
    const auto expected = mm6HudBookControls();
    const std::array<GUIButton *, 4> actual = {pBtn_Quests, pBtn_Autonotes, pBtn_Maps, pBtn_Calendar};
    for (size_t i = 0; i < actual.size(); ++i) {
        ASSERT_NE(actual[i], nullptr);
        EXPECT_EQ(actual[i]->rect, expected[i].rect);
        EXPECT_EQ(actual[i]->msg, expected[i].message);
        EXPECT_EQ(actual[i]->action, expected[i].action);
        EXPECT_TRUE(actual[i]->vTextures.empty()); // Book art is baked into the MM6 frame.
        EXPECT_TRUE(actual[i]->Contains(expected[i].rect.center()));
        for (size_t j = 0; j < actual.size(); ++j)
            if (i != j)
                EXPECT_FALSE(actual[j]->Contains(expected[i].rect.center()));
    }
    EXPECT_EQ(pBtn_History, nullptr);
}

GAME_TEST(MM6Headless, BakedActionHitBoxesAndCalendarBackground) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_NE(pPrimaryWindow, nullptr);
    const auto expected = mm6HudActionControls();
    const std::array<GUIButton *, 4> actual = {
        pBtn_CastSpell, pBtn_Rest, pBtn_QuickReference, pBtn_GameSettings
    };
    for (size_t i = 0; i < actual.size(); ++i) {
        ASSERT_NE(actual[i], nullptr);
        EXPECT_EQ(actual[i]->rect, expected[i].rect);
        EXPECT_EQ(actual[i]->msg, expected[i].message);
        EXPECT_EQ(actual[i]->action, expected[i].action);
        EXPECT_TRUE(actual[i]->vTextures.empty());
        EXPECT_TRUE(actual[i]->Contains(expected[i].rect.center()));
        for (size_t j = 0; j < actual.size(); ++j)
            if (i != j)
                EXPECT_FALSE(actual[j]->Contains(expected[i].rect.center()));
    }
    EXPECT_EQ(game_ui_btn_cast, nullptr);
    EXPECT_EQ(game_ui_btn_rest, nullptr);
    EXPECT_EQ(game_ui_btn_quickref, nullptr);
    EXPECT_EQ(game_ui_btn_settings, nullptr);
    EXPECT_EQ(engine->resources()->iconSize(calendarBackgroundIcon(GameTitle::MM6)), Sizei(360, 300));
}

TEST(MM6HeadlessActionControls, TitleIsolation) {
    EXPECT_TRUE(useMm6HudActionControls(GameTitle::MM6));
    EXPECT_FALSE(useMm6HudActionControls(GameTitle::MM7));
    const auto controls = mm6HudActionControls();
    EXPECT_EQ(controls[0].message, UIMSG_SpellBookWindow);
    EXPECT_EQ(controls[0].action, INPUT_ACTION_OPEN_SPELLBOOK);
    EXPECT_EQ(controls[1].message, UIMSG_RestWindow);
    EXPECT_EQ(controls[1].action, INPUT_ACTION_REST);
    EXPECT_EQ(controls[2].message, UIMSG_QuickReference);
    EXPECT_EQ(controls[2].action, INPUT_ACTION_OPEN_QUICK_REFERENCE);
    EXPECT_EQ(controls[3].message, UIMSG_GameMenuButton);
    EXPECT_EQ(controls[3].action, INPUT_ACTION_INVALID);
    EXPECT_EQ(calendarBackgroundIcon(GameTitle::MM6), "time_bg");
    EXPECT_EQ(calendarBackgroundIcon(GameTitle::MM7), "sbdate-time");
}

TEST(MM6HeadlessBookControls, TitleIsolation) {
    EXPECT_TRUE(useMm6HudBookControls(GameTitle::MM6));
    EXPECT_FALSE(useMm6HudBookControls(GameTitle::MM7));
    const auto books = mm6HudBookControls();
    ASSERT_EQ(books.size(), 4);
    EXPECT_EQ(books[0].message, UIMSG_OpenQuestBook);
    EXPECT_EQ(books[1].message, UIMSG_OpenAutonotes);
    EXPECT_EQ(books[2].message, UIMSG_OpenMapBook);
    EXPECT_EQ(books[3].message, UIMSG_OpenCalendar);
    EXPECT_EQ(books[0].action, INPUT_ACTION_OPEN_QUESTS);
    EXPECT_EQ(books[1].action, INPUT_ACTION_OPEN_AUTONOTES);
    EXPECT_EQ(books[2].action, INPUT_ACTION_OPEN_MAP);
    EXPECT_EQ(books[3].action, INPUT_ACTION_OPEN_CALENDAR);
}

TEST(MM6HeadlessHudFrames, TitleIsolation) {
    const HudFrameAssets mm6 = hudFrameAssets(GameTitle::MM6, {}, {}, {}, {});
    EXPECT_EQ(mm6.right, "border1.pcx");
    EXPECT_EQ(mm6.bottom, "border2.pcx");
    EXPECT_EQ(mm6.top, "border3");
    EXPECT_EQ(mm6.left, "border4");
    const HudFramePositions mm6Positions = hudFramePositions(GameTitle::MM6);
    EXPECT_EQ(mm6Positions.right, Pointi(468, 141));
    EXPECT_EQ(mm6Positions.bottom, Pointi(0, 371));
    EXPECT_EQ(mm6Positions.top, Pointi(0, 0));
    EXPECT_EQ(mm6Positions.left, Pointi(0, 8));

    const HudFrameAssets mm7Neutral = hudFrameAssets(GameTitle::MM7,
        "ib-r-A.pcx", "ib-b-A.pcx", "ib-t-A.pcx", "ib-l-A.pcx");
    EXPECT_EQ(mm7Neutral.right, "ib-r-A.pcx");
    EXPECT_EQ(mm7Neutral.bottom, "ib-b-A.pcx");
    EXPECT_EQ(mm7Neutral.top, "ib-t-A.pcx");
    EXPECT_EQ(mm7Neutral.left, "ib-l-A.pcx");
    const HudFrameAssets mm7Good = hudFrameAssets(GameTitle::MM7,
        "ib-r-B.pcx", "ib-b-B.pcx", "ib-t-B.pcx", "ib-l-B.pcx");
    EXPECT_EQ(mm7Good.right, "ib-r-B.pcx");
    EXPECT_EQ(mm7Good.bottom, "ib-b-B.pcx");
    EXPECT_EQ(mm7Good.top, "ib-t-B.pcx");
    EXPECT_EQ(mm7Good.left, "ib-l-B.pcx");
    const HudFrameAssets mm7Evil = hudFrameAssets(GameTitle::MM7,
        "ib-r-C.pcx", "ib-b-C.pcx", "ib-t-C.pcx", "ib-l-C.pcx");
    EXPECT_EQ(mm7Evil.right, "ib-r-C.pcx");
    EXPECT_EQ(mm7Evil.bottom, "ib-b-C.pcx");
    EXPECT_EQ(mm7Evil.top, "ib-t-C.pcx");
    EXPECT_EQ(mm7Evil.left, "ib-l-C.pcx");
    const HudFramePositions mm7Positions = hudFramePositions(GameTitle::MM7);
    EXPECT_EQ(mm7Positions.right, Pointi(468, 0));
    EXPECT_EQ(mm7Positions.bottom, Pointi(0, 352));
    EXPECT_EQ(mm7Positions.top, Pointi(0, 0));
    EXPECT_EQ(mm7Positions.left, Pointi(0, 8));
}

TEST(MM6HeadlessHudResources, TitleIsolation) {
    const HudNavigationResources mm6 = hudNavigationResources(GameTitle::MM6,
        "IB-COMP-A", "ib-autout-a", "ib-autin-a");
    EXPECT_EQ(mm6.compass, "compass");
    EXPECT_EQ(mm6.zoomIn, "zoom+on");
    EXPECT_EQ(mm6.zoomOut, "zoom-on");

    const HudNavigationResources mm7Neutral = hudNavigationResources(GameTitle::MM7,
        "IB-COMP-A", "ib-autout-a", "ib-autin-a");
    EXPECT_EQ(mm7Neutral.compass, "IB-COMP-A");
    EXPECT_EQ(mm7Neutral.zoomIn, "ib-autout-a");
    EXPECT_EQ(mm7Neutral.zoomOut, "ib-autin-a");
    const HudNavigationResources mm7Good = hudNavigationResources(GameTitle::MM7,
        "IB-COMP-B", "ib-autout-B", "ib-autin-B");
    EXPECT_EQ(mm7Good.compass, "IB-COMP-B");
    EXPECT_EQ(mm7Good.zoomIn, "ib-autout-B");
    EXPECT_EQ(mm7Good.zoomOut, "ib-autin-B");
    const HudNavigationResources mm7Evil = hudNavigationResources(GameTitle::MM7,
        "IB-COMP-C", "ib-autout-C", "ib-autin-C");
    EXPECT_EQ(mm7Evil.compass, "IB-COMP-C");
    EXPECT_EQ(mm7Evil.zoomIn, "ib-autout-C");
    EXPECT_EQ(mm7Evil.zoomOut, "ib-autin-C");
}

TEST(MM6HeadlessMenuAssets, TitleSpecificLookup) {
    const MainMenuResources mm6 = mainMenuResources(GameTitle::MM6);
    const MainMenuResources mm7 = mainMenuResources(GameTitle::MM7);
    EXPECT_EQ(mm6.background, "title.pcx");
    EXPECT_EQ(mm7.background, "title.pcx");
    EXPECT_EQ(mm6.buttons[0].icon, "start00a");
    EXPECT_EQ(mm6.buttons[1].icon, "start00b");
    EXPECT_EQ(mm6.buttons[2].icon, "start00c");
    EXPECT_EQ(mm6.buttons[3].icon, "start00d");
    EXPECT_EQ(mm6.buttons[0].position, Pointi(482, 9));
    EXPECT_EQ(mm6.buttons[1].position, Pointi(482, 71));
    EXPECT_EQ(mm6.buttons[2].position, Pointi(482, 133));
    EXPECT_EQ(mm6.buttons[3].position, Pointi(482, 195));
    EXPECT_EQ(mm7.buttons[0].icon, "title_new");
    EXPECT_EQ(mm7.buttons[1].icon, "title_load");
    EXPECT_EQ(mm7.buttons[2].icon, "title_cred");
    EXPECT_EQ(mm7.buttons[3].icon, "title_exit");
    EXPECT_EQ(mm7.buttons[0].position, Pointi(495, 172));
    EXPECT_EQ(mm7.buttons[1].position, Pointi(495, 227));
    EXPECT_EQ(mm7.buttons[2].position, Pointi(495, 282));
    EXPECT_EQ(mm7.buttons[3].position, Pointi(495, 337));
}

TEST(MM6HeadlessRules, RejectsMissingRulesOption) {
    char app[] = "game-test";
    char gameFlag[] = "--game";
    char mm6[] = "mm6";
    char headless[] = "--headless";
    char dataFlag[] = "--data-path";
    char testFlag[] = "--test-path";
    char dot[] = ".";
    char *argv[] = {app, gameFlag, mm6, headless, dataFlag, dot, testFlag, dot};
    EXPECT_THROW(GameTestOptions::parse(8, argv), Exception);
}

TEST(MM6HeadlessRules, SeedsOnlyCompleteBoundedRuleSet) {
    MemoryFileSystem source("source");
    MemoryFileSystem destination("destination");
    for (std::string_view name : mm6HeadlessRuleNames)
        source.write(name, Blob::fromString("{}"));

    source.remove("spells.json");
    EXPECT_THROW(seedMm6HeadlessRules(source, &destination), Exception);
    EXPECT_FALSE(destination.exists("rules"));

    source.write("spells.json", Blob::fromString("{}"));
    source.write("creation.json", Blob::fromString(std::string(1024 * 1024 + 1, 'x')));
    EXPECT_THROW(seedMm6HeadlessRules(source, &destination), Exception);
    EXPECT_FALSE(destination.exists("rules"));

    source.write("creation.json", Blob::fromString("{}"));
    seedMm6HeadlessRules(source, &destination);
    for (std::string_view name : mm6HeadlessRuleNames) {
        Blob staged = destination.read("rules/" + std::string(name));
        EXPECT_EQ(staged.str(), "{}");
    }
    Blob original = source.read("creation.json");
    EXPECT_EQ(original.str(), "{}");
}
