#include "Engine/Objects/Chest.h"

#include <array>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Engine/Engine.h"
#include "Engine/AssetsManager.h"
#include "Engine/Tables/DecorationTable.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Image.h"
#include "Engine/Localization.h"
#include "Engine/Random/Random.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Item.h"
#include "Engine/Tables/ObjectTable.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Graphics/Sprites.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Party.h"
#include "Engine/MM6/MM6ChestOpenOwnerTransaction.h"
#include "Engine/Resources/ResourceManager.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/ChestTable.h"

#include "GUI/UI/UIChest.h"
#include "GUI/UI/UIStatusBar.h"
#include "GUI/UI/ItemGrid.h"

#include "Io/Mouse.h"

#include "Media/Audio/AudioPlayer.h"

#include "Library/Logger/Logger.h"

#include "Utility/Math/TrigLut.h"

std::vector<Chest> vChests;

namespace {

bool bindMm6Chests(mm6::ChestLiveBinding *binding, MM6IndoorSnapshot *indoorBase) {
    binding->title = GameTitle::MM6;
    binding->levelType = uCurrentlyLoadedLevelType;
    binding->chests = &vChests;
    binding->titleState = &mm6::chestTitleState();
    if (binding->levelType == LEVEL_OUTDOOR && pOutdoor) {
        binding->deltaBytes = &pOutdoor->mm6DeltaBytes;
    } else if (binding->levelType == LEVEL_INDOOR && pIndoor &&
               pIndoor->doorsData.size() <= std::numeric_limits<std::size_t>::max() / 2) {
        binding->deltaBytes = &pIndoor->mm6DeltaBytes;
        indoorBase->doorDataBytes = pIndoor->doorsData.size() * 2;
        indoorBase->sections.push_back({MM6IndoorSectionKind::DoorSlots, Blob(), 200});
        binding->indoorBase = indoorBase;
    } else {
        return false;
    }
    return true;
}

std::optional<mm6::ChestIconSize> mm6ChestIconSize(int id) {
    if (!pItemTable || id < 1 || id > 580)
        return std::nullopt;
    const auto &iconName = pItemTable->items[static_cast<ItemId>(id)].iconName;
    if (iconName.empty())
        return std::nullopt;
    const auto icon = engine->resources()->icon(iconName);
    if (icon.width() <= 0 || icon.height() <= 0)
        return std::nullopt;
    return mm6::ChestIconSize{icon.width(), icon.height()};
}

bool openMm6Chest(int chestId) {
    mm6::ChestLiveBinding binding;
    MM6IndoorSnapshot indoorBase;
    if (!bindMm6Chests(&binding, &indoorBase))
        return false;

    const Blob dchest = engine->resources()->eventsData("dchest.bin");
    const auto rawTypes = std::span(static_cast<const std::uint8_t *>(dchest.data()), dchest.size());
    const auto layout = mm6::chestTypeLayout(GameTitle::MM6, vChests[chestId].chestTypeId, rawTypes);
    if (!layout)
        return false;
    const auto background = engine->resources()->icon(layout->iconName);
    if (background.width() <= 0 || background.height() <= 0)
        return false;

    auto staged = mm6::stageChestOpenOwner(binding, chestId, rawTypes, mm6ChestIconSize);
    if (!staged)
        return false;

    // Construct the GUI before changing chest, sidecar or RNG owners. Its
    // constructor can allocate; the remaining owner publication cannot throw.
    std::unique_ptr<GUIWindow_Chest> chestWindow;
    if (pParty->hasActiveCharacter())
        chestWindow = std::make_unique<GUIWindow_Chest>(chestId);
    binding.chests->swap(staged->chests);
    binding.deltaBytes->swap(staged->deltaBytes);
    binding.titleState->lcgState = staged->nextLcgState;
    if (!chestWindow)
        return false;
    pGUIWindow_CurrentChest = std::move(chestWindow);
    pAudioPlayer->playUISound(SOUND_openchest0101);
    OpenedTelekinesis = false;
    return true;
}

void takeMm6ChestItem(int chestId, Pointi mousePos) {
    mm6::ChestLiveBinding binding;
    MM6IndoorSnapshot indoorBase;
    if (pParty->pPickedItem.itemId != ITEM_NULL || !bindMm6Chests(&binding, &indoorBase))
        return;
    const Blob dchest = engine->resources()->eventsData("dchest.bin");
    const auto rawTypes = std::span(static_cast<const std::uint8_t *>(dchest.data()), dchest.size());
    const auto layout = mm6::chestTypeLayout(GameTitle::MM6, vChests[chestId].chestTypeId, rawTypes);
    if (!layout)
        return;
    const Pointi inGrid = mousePos - Pointi(layout->offsetX, layout->offsetY);
    if (inGrid.x < 0 || inGrid.y < 0)
        return;
    const Pointi gridPoint = inGrid / 32;
    auto staged = mm6::stageChestOpenOwner(binding, chestId, rawTypes, mm6ChestIconSize,
                                           mm6::ChestInteractionAction::TakeGridItem, gridPoint);
    if (!staged)
        return; // An empty cell takes nothing, and neither does gold.

    Pointi offset = inGrid - gridPoint * 32;
    GraphicsImage *icon = assets->getIcon(staged->heldItem.GetIconName());
    offset -= Pointi(itemOffset(icon->width()), itemOffset(icon->height()));
    binding.chests->swap(staged->chests);
    binding.deltaBytes->swap(staged->deltaBytes);
    pParty->setHoldingItem(staged->heldItem, -offset);
}

} // namespace

bool Chest::open(int uChestID, Pid objectPid) {
    double dir_x;
    double dir_y;
    double length_vector;
    int pDepth;
    bool flag_shout;
    SpriteId pSpriteID[4];
    Vec3f pOut;
    int yawAngle = 0;
    int pitchAngle = 0;
    SpriteObject pSpellObject;

    if (uChestID < 0 || uChestID >= static_cast<int>(vChests.size()))
        return false;
    if (engine->gameTitle() == GameTitle::MM6)
        return openMm6Chest(uChestID);
    Chest *chest = &vChests[uChestID];

    if (!chest->Initialized() || engine->config->gameplay.ChestTryPlaceItems.value() == 1)
        Chest::PlaceItems(uChestID);

    if (!pParty->hasActiveCharacter())
        return false;
    flag_shout = false;
    if (chest->Trapped() && engine->_currentLoadedMapId != MAP_INVALID) {
        if (pParty->activeCharacter().GetDisarmTrap() < 2 * pMapTable->pInfos[engine->_currentLoadedMapId].disarmDifficulty) {
            pSpriteID[0] = SPRITE_TRAP_FIRE;
            pSpriteID[1] = SPRITE_TRAP_LIGHTNING;
            pSpriteID[2] = SPRITE_TRAP_COLD;
            pSpriteID[3] = SPRITE_TRAP_BODY;
            int pRandom = grng->random(4); // Not sure if this should be grng or vrng, so we'd rather err on the side of safety.
            int objId = objectPid.id();

            Vec3f objectPos;
            if (chest->position) {
                objectPos = *chest->position;
            } else if (objectPid.type() == OBJECT_Decoration) {
                objectPos = pLevelDecorations[objId].vPosition +
                    Vec3f(0, 0, pDecorationTable->decoration(pLevelDecorations[objId].uDecorationDescID)->uDecorationHeight / 2);
            } else if (objectPid.type() == OBJECT_Face) {
                if (uCurrentlyLoadedLevelType == LEVEL_OUTDOOR) {
                    objectPos = pOutdoor->face(objectPid).boundingBox.center();
                } else {
                    objectPos = pIndoor->faces[objId].boundingBox.center();
                }
            }

            dir_x = (double)pParty->pos.x - (double)objectPos.x;
            dir_y = (double)pParty->pos.y - (double)objectPos.y;
            length_vector = std::sqrt(dir_x * dir_x + dir_y * dir_y);
            if (length_vector <= 1.0) {
                yawAngle = 0;
                pitchAngle = 0;
            } else {
                // sprite should be rotated towards the party and moved slightly forward
                yawAngle = TrigLUT.atan2(dir_x, dir_y);
                pitchAngle = 256;
            }

            pDepth = 96;
            if (length_vector < pDepth) {
                pDepth = length_vector;
            }
            pOut = objectPos + Vec3f::fromPolar(pDepth, yawAngle, pitchAngle);

            pSpellObject.containing_item.Reset();
            pSpellObject.spell_skill = MASTERY_NONE;
            pSpellObject.spell_level = 0;
            pSpellObject.uSpellID = SPELL_NONE;
            pSpellObject.field_54 = 0;
            pSpellObject.spriteId = pSpriteID[pRandom];
            pSpellObject.uObjectDescID = pObjectTable->ObjectIDByItemID(pSpellObject.spriteId);

            // adjust height to account for different sprite sizes and offset
            SpriteFrame *frame = pSpellObject.spriteFrame();
            if (frame->flags & SPRITE_FRAME_CENTER) {
                // centering
                pOut += Vec3f(0, 0, frame->sprites[0]->height() / 4);
            } else {
                pOut -= Vec3f(0, 0, (frame->sprites[0]->height() - 64) / 2);
            }
            pSpellObject.vPosition = pOut;

            pSpellObject.uAttributes = SPRITE_IGNORE_RANGE | SPRITE_NO_Z_BUFFER;
            pSpellObject.uSectorID = pIndoor->GetSector(pSpellObject.vPosition);
            pSpellObject.timeSinceCreated = 0_ticks;
            pSpellObject.spell_caster_pid = Pid();
            pSpellObject.spell_target_pid = Pid();
            pSpellObject.uFacing = 0;
            pSpellObject.Create(0, 0, 0, 0);
            // TODO(Nik-RE-dev): chest is originator in this case
            pAudioPlayer->playSound(SOUND_fireBall, SOUND_MODE_UI);
            pSpellObject.explosionTraps();
            chest->flags &= ~CHEST_TRAPPED;
            if (pParty->hasActiveCharacter() && !OpenedTelekinesis) {
                pParty->setDelayedReaction(SPEECH_TRAP_EXPLODED, pParty->activeCharacterIndex());
            }
            OpenedTelekinesis = false;
            return false;
        }
        chest->flags &= ~CHEST_TRAPPED;
        flag_shout = true;
    }
    pAudioPlayer->playUISound(SOUND_openchest0101);
    if (flag_shout == true) {
        if (!OpenedTelekinesis) {
            pParty->activeCharacter().playReaction(SPEECH_TRAP_DISARMED);
        }
    }
    OpenedTelekinesis = false;

    pGUIWindow_CurrentChest = std::make_unique<GUIWindow_Chest>(uChestID);

    return true;
}

bool Chest::ChestUI_WritePointedObjectStatusString() {
    Chest *chest = &vChests[pGUIWindow_CurrentChest->chestId()];

    Pointi mousePos = mouse->position();
    // TODO(captainurist): need to use mouse->pickedItemOffset here?
    Pointi inventoryPos = mapToInventoryGrid(mousePos, chestTable[chest->chestTypeId].inventoryOffset);

    InventoryEntry entry = chest->inventory.entry(inventoryPos);
    if (entry) {
        engine->_statusBar->setPermanent(entry->GetDisplayName());
        uLastPointedObjectID = Pid::dummy();
        return 1;
    } else {
        return 0;
    }
}

void Chest::PlaceItems(int uChestID) { // only used for setup
    Chest &chest = vChests[uChestID];

    char chest_cells_map[144];   // [sp+Ch] [bp-A0h]@1

    int uChestArea = chestTable[chest.chestTypeId].size.w * chestTable[chest.chestTypeId].size.h;
    memset(chest_cells_map, 0, 144);
    // fill cell map at random positions
    for (int items_counter = 0; items_counter < uChestArea; ++items_counter) {
        // get random position in chest
        int random_chest_pos = 0;
        do {
            random_chest_pos = grng->random(256);
        } while (random_chest_pos >= uChestArea);
        // if this pos occupied move to next
        while (chest_cells_map[random_chest_pos]) {
            ++random_chest_pos;
            if (random_chest_pos == uChestArea) random_chest_pos = 0;
        }
        chest_cells_map[random_chest_pos] = items_counter;
    }

    Sizei chestSize = chest.inventory.gridSize();
    for (InventoryEntry entry : chest.inventory.entries()) {
        if (entry.zone() != INVENTORY_ZONE_STASH)
            continue;

        assert(!isRandomItem(entry->itemId) && "Checking that generated items are valid");

        Sizei itemSize = entry->inventorySize();
        for (int i = 0; i < uChestArea; i++) {
            Pointi pos(chest_cells_map[i] % chestSize.w, chest_cells_map[i] / chestSize.w);

            if (chest.inventory.canAdd(pos, itemSize)) {
                // TODO(captainurist): this is a weird place to call postGenerate, and we won't call it for items that
                //                     end up buried. But it's here b/c we don't want to break the traces, at least now.
                //                     Redo this properly and write a small test.
                entry->postGenerate(ITEM_SOURCE_CHEST);
                if (chest.flags & CHEST_OPENED)
                    entry->SetIdentified();
                entry = chest.inventory.add(pos, chest.inventory.take(entry));
                break;
            }
        }

        if (entry.zone() == INVENTORY_ZONE_STASH)
            MM_TRACE("Cannot place item with id {} in the chest!", std::to_underlying(entry->itemId));
    }

    chest.SetInitialized(true);
}

void Chest::toggleFlag(int uChestID, ChestFlag uFlag, bool bValue) {
    if (uChestID >= 0 && uChestID <= 19) {
        if (bValue) {
            vChests[uChestID].flags |= uFlag;
        } else {
            vChests[uChestID].flags &= ~uFlag;
        }
    }
}

void Chest::OnChestLeftClick() {
    if (engine->gameTitle() == GameTitle::MM6) {
        takeMm6ChestItem(pGUIWindow_CurrentChest->chestId(), mouse->position());
        return;
    }
    int uChestID = pGUIWindow_CurrentChest->chestId();
    Chest *chest = &vChests[uChestID];

    Pointi mousePos = mouse->position();
    Pointi inventoryPos = mapToInventoryGrid(mousePos + mouse->pickedItemOffset, chestTable[chest->chestTypeId].inventoryOffset, &pParty->pPickedItem);

    if (pParty->pPickedItem.itemId != ITEM_NULL) {  // item held
        std::optional<Pointi> pos;
        if (chest->inventory.canAdd(inventoryPos, pParty->pPickedItem)) {
            pos = inventoryPos;
        } else {
            pos = chest->inventory.findSpace(pParty->pPickedItem);
        }

        if (pos) {
            chest->inventory.add(*pos, pParty->takeHoldingItem());
        }
    } else {
        if (InventoryEntry entry = chest->inventory.entry(inventoryPos)) {
            Item item = chest->inventory.take(entry);

            if (item.isGold()) {
                pParty->partyFindsGold(item.goldAmount, GOLD_RECEIVE_SHARE);
            } else {
                Pointi offset = mousePos + mouse->pickedItemOffset - chestTable[chest->chestTypeId].inventoryOffset - (inventoryPos * 32);

                GraphicsImage *tex = assets->getIcon(item.GetIconName());
                offset -= Pointi(itemOffset(tex->width()), itemOffset(tex->height()));

                pParty->setHoldingItem(item, -offset);
            }

            if (engine->config->gameplay.ChestTryPlaceItems.value() == 2)
                Chest::PlaceItems(uChestID);
        }
    }
}

void Chest::GrabItem(bool all) {  // new function to grab items from chest using spacebar
    if (engine->gameTitle() == GameTitle::MM6)
        return;
    if (pParty->pPickedItem.itemId != ITEM_NULL || !pParty->hasActiveCharacter()) {
        return;
    }

    int InventSlot;
    int grabcount = 0;
    int goldcount = 0;
    int goldamount = 0;

    int chestId = pGUIWindow_CurrentChest->chestId();
    Chest *chest = &vChests[chestId];

    for (InventoryEntry entry : chest->inventory.entries()) {
        if (entry->isGold()) {
            pParty->partyFindsGold(entry->goldAmount, GOLD_RECEIVE_SHARE);
            goldamount += entry->goldAmount;
            goldcount++;
        } else {  // this should add item to invetory of active char - if that fails set as holding item and break
            std::optional<Pointi> pos;
            if (pParty->hasActiveCharacter())
                pos = pParty->activeCharacter().inventory.findSpace(*entry);

            if (pos) {  // can place
                pParty->activeCharacter().inventory.add(*pos, *entry);
                grabcount++;
                engine->_statusBar->setEvent(LSTR_YOU_FOUND_AN_ITEM_S, pItemTable->items[entry->itemId].unidentifiedName);
            } else {  // no room so set as holding item
                pParty->setHoldingItem(*entry);
                chest->inventory.take(entry);
                pParty->activeCharacter().playReaction(SPEECH_NO_ROOM);
                break;
            }
        }
        chest->inventory.take(entry);
        if (all == false)  // only grab 1 item
            break;
    }

    if (engine->config->gameplay.ChestTryPlaceItems.value() == 2)
        Chest::PlaceItems(chestId);

    if (grabcount > 1 || goldcount > 1) {  // found items
        engine->_statusBar->setEvent(fmt::format("You found {} item(s) and {} Gold!", grabcount, goldamount));
    }
    if (grabcount == 0 && goldcount == 0) {
        engine->_statusBar->setEvent(LSTR_NOTHING_HERE);
    }
}

void GenerateItemsInChest() {
    MapData *mapData = &pMapTable->pInfos[engine->_currentLoadedMapId];
    for (int i = 0; i < 20; ++i) {
        for (InventoryEntry entry : vChests[i].inventory.entries()) {
            if (!isRandomItem(entry->itemId))
                continue;

            ItemTreasureLevel resultTreasureLevel = grng->randomSample(
                RemapTreasureLevel(randomItemTreasureLevel(entry->itemId), mapData->mapTreasureLevel));

            if (resultTreasureLevel == ITEM_TREASURE_LEVEL_7) {
                // TODO(captainurist): GenerateArtifact calls Reset on failure, this messes up inventory state. Rewrite properly.
                Item item;
                if (item.GenerateArtifact()) {
                    *entry = item;
                } else {
                    vChests[i].inventory.take(entry);
                }
                continue;
            }

            int itemCount = grng->randomInSegment(1, 5);
            for (int k = 0; k < itemCount; k++) {
                Item item;
                int whatToGenerateProb = grng->random(100);
                if (whatToGenerateProb < 20) {
                    // Do nothing.
                } else if (whatToGenerateProb < 60) {  // generate gold
                    item.generateGold(resultTreasureLevel);
                } else {
                    pItemTable->generateItem(resultTreasureLevel, RANDOM_ITEM_ANY, &item);
                }
                if (item.itemId != ITEM_NULL) {
                    if (entry) {
                        *entry = item;
                        entry = {};
                    } else {
                        vChests[i].inventory.stash(item);
                    }
                }
            }
            if (entry) // We didn't generate anything.
                vChests[i].inventory.take(entry);
        }
    }
}

mm6::ChestGenerationRoute mm6::chestGenerationRoute(GameTitle title) {
    return title == GameTitle::MM7 ? ChestGenerationRoute::InheritedMM7 : ChestGenerationRoute::Blocked;
}

std::optional<Segment<ItemTreasureLevel>> mm6::chestTreasureLevelRangeForMarker(
    int markerId, MapTreasureLevel mapTreasureLevel) {
    // Original MM6 table at VA 0x4c03a8 in the accepted F3 executable.
    // Columns use the zero-based map treasure level from MM6 mapstats.txt.
    static constexpr std::array<std::array<Segment<int>, 7>, 7> ranges = {{
        {{{1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}}},
        {{{1, 1}, {1, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}}},
        {{{1, 2}, {2, 2}, {2, 3}, {3, 3}, {3, 3}, {3, 3}, {3, 3}}},
        {{{2, 2}, {2, 3}, {3, 3}, {3, 4}, {4, 4}, {4, 4}, {4, 4}}},
        {{{2, 3}, {3, 3}, {3, 4}, {4, 4}, {4, 5}, {5, 5}, {5, 5}}},
        {{{3, 3}, {3, 4}, {4, 4}, {4, 5}, {5, 5}, {5, 6}, {6, 6}}},
        {{{7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}}}
    }};

    const int markerLevel = -markerId;
    const int mapLevel = std::to_underlying(mapTreasureLevel);
    if (markerLevel < 1 || markerLevel > 7 || mapLevel < 0 || mapLevel > 6)
        return std::nullopt;
    Segment<int> range = ranges[markerLevel - 1][mapLevel];
    return Segment{static_cast<ItemTreasureLevel>(range.front()),
                   static_cast<ItemTreasureLevel>(range.back())};
}

void mm6::generateChestItemsForTitle(GameTitle title) {
    if (chestGenerationRoute(title) != ChestGenerationRoute::InheritedMM7)
        throw std::runtime_error("MM6 chest random-item generation is not implemented");
    GenerateItemsInChest();
}

void UpdateChestPositions() {
    std::unordered_map<int, std::vector<Vec3f>> pointsByChestId;

    auto processEvent = [&](int eventId, const Vec3f &position) {
        // Can there be two EVENT_OpenChest in a single script, with different chests? If no, then we can
        // break out of the loop below early. If yes... Well. This should work.
        if (engine->_localEventMap.hasEvent(eventId))
            for (const EvtInstruction &event : engine->_localEventMap.function(eventId))
                if (event.opcode == EVENT_OpenChest)
                    pointsByChestId[event.data.chest_id].push_back(position);
    };

    if (uCurrentlyLoadedLevelType == LEVEL_OUTDOOR) {
        for (const BLVFace &face : pOutdoor->faces)
            if (face.eventId)
                processEvent(face.eventId, face.boundingBox.center());
    } else {
        for (const BLVFace &face : pIndoor->faces)
            if (int eventId = face.eventId)
                processEvent(eventId, face.boundingBox.center());
    }

    for (const auto &[chestId, points] : pointsByChestId) {
        Vec3f center = std::accumulate(points.begin(), points.end(), Vec3f()) / points.size();

        bool isChestLike = true;
        for (const Vec3f &point : points) {
            if ((point - center).lengthSqr() > 256 * 256) {
                // Wormhole chest detected. 256 is half the size of the ODM tile.
                isChestLike = false;
                break;
            }
        }

        if (isChestLike)
            vChests[chestId].position = center;
    }
}
