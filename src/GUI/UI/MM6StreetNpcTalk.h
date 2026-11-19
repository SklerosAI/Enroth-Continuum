#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "Engine/Objects/NPCEnums.h"

#include "GUI/GUIDialogues.h"

class Character;
class GUIWindow;
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
 * @param[out] npc                      Street NPC, its `newsTopic` holds the npcnews.txt row from the first one.
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

/**
 * @offset 0x4852D0
 * @param character                     Character who does the talking.
 * @param isHired                       Whether a hired NPC has the given profession.
 * @return                              What the character's diplomacy takes off the reputation cost of begging,
 *                                      threatening and bribing, and off the price of a bribe.
 */
int diplomacyBonus(const Character &character, bool (*isHired)(NpcProfession));

/**
 * @param diplomacy                     Result of `diplomacyBonus`.
 * @param bribes                        Street NPCs the party has bribed so far.
 * @return                              Gold the next bribe costs.
 */
int bribePrice(int diplomacy, int bribes);

/**
 * @param talker                        Character who does the talking.
 * @return                              Gold the party's next bribe costs.
 */
int streetNpcBribePrice(const Character &talker);

/**
 * @param npc                           Street NPC.
 * @param action                        `DIALOGUE_BEG`, `DIALOGUE_THREAT` or `DIALOGUE_BRIBE`.
 * @return                              Whether the NPC's personality lets the action work, from npcBTB.txt.
 */
bool streetNpcAccepts(const NPCData &npc, DialogueId action);

/**
 * @param action                        `DIALOGUE_BEG`, `DIALOGUE_THREAT` or `DIALOGUE_BRIBE`.
 * @return                              Label of the action's topic button, a bribe shows its price.
 */
std::string streetNpcActionLabel(DialogueId action);

/**
 * @param npc                           Street NPC.
 * @param action                        `DIALOGUE_BEG`, `DIALOGUE_THREAT` or `DIALOGUE_BRIBE`.
 * @return                              npcBTB.txt template the NPC answers the action with.
 */
std::string_view streetNpcActionText(const NPCData &npc, DialogueId action);

/**
 * Begs, threatens or bribes a street NPC. When the NPC goes along with it, the party pays the gold and the
 * reputation, the NPC remembers it for the next greeting, and the window's Beg, Threat and Bribe topics turn into
 * the NPC's usual ones.
 *
 * @offset 0x4A3E19, 0x4A3F16, 0x4A4077
 * @param[in,out] npc                   Street NPC, not hired.
 * @param action                        `DIALOGUE_BEG`, `DIALOGUE_THREAT` or `DIALOGUE_BRIBE`.
 * @param[in,out] window                Conversation window.
 * @return                              Dialogue the window shows next, the action's answer or the greeting.
 */
DialogueId takeStreetNpcAction(NPCData *npc, DialogueId action, GUIWindow *window);

/**
 * Replaces the conversation window's topics with More Information, when the profession has a description, and Hire.
 *
 * @offset 0x4993C0
 * @param[in,out] window                Conversation window.
 * @param npc                           Street NPC, not hired.
 */
void showStreetNpcJoinTopics(GUIWindow *window, const NPCData &npc);

/**
 * @offset 0x48A01C
 * @param count                         Number of addressing awards the talking character has, at least one.
 * @return                              Which of them `%08` names in dialogue text, picked once per conversation.
 */
int addressingAwardChoice(int count);

/**
 * Makes the next `addressingAwardChoice` pick again. MM6 does it when a conversation with an NPC starts.
 *
 * @offset 0x43BE5F
 */
void forgetAddressingAwardChoice();

} // namespace mm6
