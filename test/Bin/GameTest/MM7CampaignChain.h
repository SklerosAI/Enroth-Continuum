#pragma once

#include <functional>
#include <utility>

#include "Engine/Data/HouseEnums.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/ItemEnums.h"
#include "Library/Geometry/Vec.h"

class EngineController;
struct BLVFace;

/**
 * @param face                          Clickable door face.
 * @return                              Position 150 units in front of `face` and the yaw that looks at it.
 */
std::pair<Vec3f, int> approachCampaignChainFace(const BLVFace &face);

/**
 * Checks that a campaign chain chapter runs headless against the marked disposable profile it was started with.
 */
void assertCampaignChainProfile();

/**
 * Loads a chapter's save through the main menu's Load screen.
 *
 * @param game                          Engine controller of the running test.
 * @param chapter                       Chapter whose slot to load.
 */
void loadCampaignChainSlot(EngineController &game, int chapter);

/**
 * Saves the game to a chapter's slot and checks that the slot is a structurally valid MM7 save.
 *
 * @param game                          Engine controller of the running test.
 * @param chapter                       Chapter whose slot to write.
 */
void saveCampaignChainSlot(EngineController &game, int chapter);

/**
 * Clicks a clickable face of an indoor event from every spot nearby where the party can stand and see it.
 *
 * @param game                          Engine controller of the running test.
 * @param map                           Current indoor map.
 * @param eventId                       Event of the faces to click.
 * @param clicks                        Clicks per spot before trying the next one.
 * @param done                          Stops the clicking once it returns true.
 * @return                              Whether `done` returned true.
 */
bool clickCampaignChainEventFace(EngineController &game, MapId map, int eventId, int clicks, const std::function<bool()> &done);

/**
 * Opens an indoor chest by clicking it and takes items out of it with Space until the party has `itemId`.
 *
 * @param game                          Engine controller of the running test.
 * @param map                           Current indoor map.
 * @param eventId                       Event of the chest's faces.
 * @param itemId                        Item to take.
 */
void lootCampaignChainChest(EngineController &game, MapId map, int eventId, ItemId itemId);

/**
 * Walks up to a clickable door face of an indoor event, presses Space and accepts the map transition it offers.
 *
 * @param game                          Engine controller of the running test.
 * @param map                           Current indoor map.
 * @param eventId                       Event of the door face.
 */
void useCampaignChainTransition(EngineController &game, MapId map, int eventId);

/**
 * Teleports to the Celeste town portal arrival point, walks up to a house door, enters and picks an NPC.
 *
 * @param game                          Engine controller of the running test.
 * @param houseId                       House behind the door.
 * @param npcId                         NPC to talk to.
 */
void openCampaignChainCelesteHouse(EngineController &game, HouseId houseId, int npcId);

/**
 * Teleports to the Castle Lambent throne room door, enters and picks an NPC.
 *
 * @param game                          Engine controller of the running test.
 * @param npcId                         NPC to talk to.
 */
void openCampaignChainThroneRoom(EngineController &game, int npcId);

/**
 * Clicks the dialogue topic of an NPC's scripted event.
 *
 * @param game                          Engine controller of the running test.
 * @param npcId                         NPC in the open dialogue.
 * @param eventId                       Event behind the topic.
 * @return                              Whether the NPC offered the topic.
 */
bool selectCampaignChainNpcEvent(EngineController &game, int npcId, int eventId);

/**
 * Presses Escape until the party is back in the game view.
 *
 * @param game                          Engine controller of the running test.
 */
void leaveCampaignChainHouse(EngineController &game);
