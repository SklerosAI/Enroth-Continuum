#pragma once

#include <span>

#include "Engine/Evt/EvtContext.h"
#include "Engine/Evt/EvtInstruction.h"

class Character;

namespace mm6 {

/**
 * @param characters                    Party members whose inventories are counted, equipped items included.
 * @return                              Circus prize score tested by MM6 EVT variable 224. Items 470, 471 and 477 are
 *                                      worth 1, 3 and 5 points. The item on the cursor doesn't count.
 */
int circusPrizeScore(std::span<const Character> characters);

/**
 * Recognizes the reward of a shipped Circus prize trade, global events 129 to 134. Each event checks the party
 * members in turn for a Golden Pyramid (129 to 131) or a Keg of Wine (132 to 134), takes it from the first who has
 * one and gives a random armor, weapon or accessory at level 6 for a pyramid or level 4 for a keg.
 *
 * @param context                       Running event.
 * @param instructions                  The running event's script.
 * @param reward                        GiveItem about to run.
 * @return                              Whether `reward` is the trade's random item and the script is the shipped one.
 */
bool isCircusPrizeTrade(const EvtContext &context, std::span<const EvtInstruction> instructions, const EvtInstruction &reward);

} // namespace mm6
