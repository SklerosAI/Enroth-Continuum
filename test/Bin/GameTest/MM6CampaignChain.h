#pragma once

#include <filesystem>
#include <string_view>

#include "Engine/MapEnums.h"
#include "Engine/Objects/ItemEnums.h"

class EngineController;

/**
 * Makes a validated disposable profile the MM6 save root of a campaign chain chapter. Must run on the game thread.
 *
 * @param root                          Canonical owner-only `mm6-restart-chain-*` profile directory.
 */
void setMM6CampaignChainProfile(const std::filesystem::path &root);

/**
 * @return                              Profile root of the running chapter, empty outside a chain run.
 */
const std::filesystem::path &mm6CampaignChainProfile();

/**
 * Checks that an MM6 campaign chain chapter runs headless against the marked disposable profile it was started with.
 */
void assertMM6CampaignChainProfile();

/**
 * Loads a chapter's save through the main menu's Load screen.
 *
 * @param game                          Engine controller of the running test.
 * @param chapter                       Chapter whose slot to load.
 */
void loadMM6CampaignChainSlot(EngineController &game, int chapter);

/**
 * Saves the game from the Save menu into a new slot, which must become the chapter's slot, and checks that the slot
 * lists and reads back as a loadable MM6 save.
 *
 * @param game                          Engine controller of the running test.
 * @param chapter                       Chapter whose slot to write. Slots of all earlier chapters must exist.
 */
void saveMM6CampaignChainSlot(EngineController &game, int chapter);

/**
 * Teleports in front of an outdoor face of a map event and presses Space. Loads the map first when the party is
 * elsewhere.
 *
 * @param game                          Engine controller of the running test.
 * @param map                           Outdoor map of the event.
 * @param eventId                       Local event of the face.
 */
void useMM6CampaignChainEventFace(EngineController &game, MapId map, int eventId);

/**
 * Teleports in front of a wall face of an event on the current indoor map and presses Space.
 *
 * @param game                          Engine controller of the running test.
 * @param eventId                       Local event of the face. Its first face with a horizontal normal and a floor in
 *                                      front of it is used.
 */
void useMM6CampaignChainIndoorEventFace(EngineController &game, int eventId);

/**
 * Clicks the first wall face of an event on the current indoor map that the party sees from 150 units in front of it.
 *
 * @param game                          Engine controller of the running test.
 * @param eventId                       Local event of the face.
 */
void clickMM6CampaignChainIndoorEventFace(EngineController &game, int eventId);

/**
 * Teleports next to a pressure plate of an event on the current indoor map and walks onto it.
 *
 * @param game                          Engine controller of the running test.
 * @param eventId                       Local event of the plate. The party walks towards its first floor face that is
 *                                      a pressure plate and stops on any floor face of the event.
 */
void walkOntoMM6CampaignChainIndoorEventPlate(EngineController &game, int eventId);

/**
 * Types an answer into the open Question prompt of an event and presses Enter.
 *
 * @param game                          Engine controller of the running test.
 * @param answer                        Lowercase letters, digits and spaces. MM6 compares answers ignoring case.
 */
void answerMM6CampaignChainQuestion(EngineController &game, std::string_view answer);

/**
 * Picks an item up from the open chest and puts it into the first character's backpack.
 *
 * @param game                          Engine controller of the running test.
 * @param chestId                       Open chest.
 * @param itemId                        Item to take, must be in the chest.
 */
void takeMM6CampaignChainChestItem(EngineController &game, int chestId, ItemId itemId);

/**
 * Attacks a monster on the current indoor map until it dies, standing next to it and aiming at it.
 *
 * @param game                          Engine controller of the running test.
 * @param actorId                       Monster to kill.
 */
void killMM6CampaignChainActor(EngineController &game, int actorId);

/**
 * Loots a corpse on the current indoor map by pressing Space next to it, once it has settled. Kills a living monster
 * that stands in the way.
 *
 * @param game                          Engine controller of the running test.
 * @param actorId                       Dead monster.
 */
void lootMM6CampaignChainCorpse(EngineController &game, int actorId);

/**
 * Picks an item up off the floor of the current indoor map by clicking it.
 *
 * @param game                          Engine controller of the running test.
 * @param itemId                        Item to take, must lie on the floor.
 */
void pickUpMM6CampaignChainFloorItem(EngineController &game, ItemId itemId);

/**
 * Clicks a decoration on the current indoor map, standing next to it.
 *
 * @param game                          Engine controller of the running test.
 * @param decorationId                  Decoration to click.
 */
void useMM6CampaignChainDecoration(EngineController &game, int decorationId);

/**
 * Enters a castle whose door asks before it lets the party in, and answers yes.
 *
 * @param game                          Engine controller of the running test.
 * @param map                           Outdoor map of the castle.
 * @param eventId                       Local event of the castle door.
 */
void enterMM6CampaignChainCastle(EngineController &game, MapId map, int eventId);

/**
 * Answers yes to the open dungeon entrance or exit prompt and waits out the loading screen.
 *
 * @param game                          Engine controller of the running test.
 */
void confirmMM6CampaignChainTransition(EngineController &game);

/**
 * Goes down to the Oracle through the High Council's door on `outc2`, which the council's approval opens.
 *
 * @param game                          Engine controller of the running test.
 */
void enterMM6CampaignChainOracle(EngineController &game);

/**
 * Picks the way to another map in the open house screen and confirms it.
 *
 * @param game                          Engine controller of the running test.
 * @param map                           Map the way leads to.
 */
void takeMM6CampaignChainHouseExit(EngineController &game, MapId map);

/**
 * Picks a person in the open house screen by name.
 *
 * @param game                          Engine controller of the running test.
 * @param npcId                         NPC to talk to.
 */
void talkToMM6CampaignChainNpc(EngineController &game, int npcId);

/**
 * Clicks the topic of the NPC the party talks to whose script is the given global event.
 *
 * @param game                          Engine controller of the running test.
 * @param eventId                       Global event behind the topic.
 * @return                              Whether the NPC offered the topic.
 */
bool selectMM6CampaignChainTopic(EngineController &game, int eventId);

/**
 * Presses Escape until the party is back in the game view.
 *
 * @param game                          Engine controller of the running test.
 */
void leaveMM6CampaignChainHouse(EngineController &game);
