#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "GUI/GUIDialogues.h"

struct NPCData;

namespace mm6 {

/**
 * What MM6's conversation window offers a street NPC that isn't hired, chosen when the conversation starts.
 */
enum class StreetNpcTalkMode {
    STREET_NPC_TALK_TOPICS, // The weekday topic, Join and the news.
    STREET_NPC_TALK_FAME_TOO_LOW, // No topics, the NPC turns the party away on the right panel.
    STREET_NPC_TALK_REPUTATION, // Beg, Threat and Bribe.
};
using enum StreetNpcTalkMode;

/**
 * @offset 0x43BE50
 * @param npc                           Street NPC, not hired.
 * @param fame                          Party fame.
 * @param reputation                    Effective party reputation, positive is good.
 * @return                              What the conversation window offers.
 */
StreetNpcTalkMode streetNpcTalkMode(const NPCData &npc, int fame, int reputation);

/**
 * @param mode                          Result of `streetNpcTalkMode`.
 * @return                              Topics of the conversation window's first screen.
 */
std::vector<DialogueId> streetNpcTopics(StreetNpcTalkMode mode);

/**
 * Picks the news topic of a street NPC from the npcnews.txt rows of the current map, once per NPC.
 *
 * @offset 0x43BC20
 * @param npc[out]                      Street NPC, its `newsTopic` holds the npcnews.txt row from the first one.
 * @param mapFileName                   File name of the current map.
 */
void pickStreetNpcNews(NPCData *npc, std::string_view mapFileName);

/**
 * @param npc                           Street NPC.
 * @param dayOfMonth                    Zero-based day of the month.
 * @return                              Label of the profession's topic of the day, from proftext.txt.
 */
std::string_view streetNpcWeekdayTopic(const NPCData &npc, int dayOfMonth);

/**
 * @param npc                           Street NPC.
 * @param dayOfMonth                    Zero-based day of the month.
 * @return                              Text of the profession's topic of the day, from proftext.txt.
 */
std::string_view streetNpcWeekdayText(const NPCData &npc, int dayOfMonth);

/**
 * @param npc                           Street NPC after `pickStreetNpcNews`.
 * @return                              Label of the NPC's news, from npcnews.txt.
 */
std::string_view streetNpcNewsTopic(const NPCData &npc);

/**
 * @param npc                           Street NPC after `pickStreetNpcNews`.
 * @return                              Text of the NPC's news, from npcnews.txt.
 */
std::string_view streetNpcNewsText(const NPCData &npc);

/**
 * @offset 0x43AEB3
 * @param npc                           Street NPC, not hired.
 * @param mode                          Result of `streetNpcTalkMode`.
 * @param reputation                    Effective party reputation, positive is good.
 * @return                              npcBTB.txt template the NPC greets the party with before a topic is chosen,
 *                                      for the NPC's personality.
 */
std::string_view streetNpcGreeting(const NPCData &npc, StreetNpcTalkMode mode, int reputation);

} // namespace mm6
