#pragma once

#include <filesystem>

#include "Engine/MapEnums.h"

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
 * Enters a castle whose door asks before it lets the party in, and answers yes.
 *
 * @param game                          Engine controller of the running test.
 * @param map                           Outdoor map of the castle.
 * @param eventId                       Local event of the castle door.
 */
void enterMM6CampaignChainCastle(EngineController &game, MapId map, int eventId);

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
