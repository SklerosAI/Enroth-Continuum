#pragma once

#include <functional>

class Character;
struct Party;

namespace mm6 {

/**
 * Applies the MM6 death penalties and the respawn placement to a party: hirelings leave, the death count goes up, a
 * week passes, gold and every buff and condition are lost, and the party stands at the New Sorpigal respawn point
 * with 1 HP each. An active Guardian Angel halves the gold instead, and at Expert or Master raises the party with half
 * or full health. Gold is written silently.
 *
 * @param[in,out] party                 Party none of whose members can act.
 * @param maxHealth                     Maximum health of a character, called once its buffs and conditions are gone.
 */
void applyPartyDeath(Party *party, const std::function<int(const Character &)> &maxHealth);

/**
 * MM6 counterpart of the game loop's `GAME_STATE_PARTY_DIED` handler. Plays the losing movie, applies
 * `applyPartyDeath`, moves the party to New Sorpigal if it died elsewhere and resumes play.
 */
void respawnDeadParty();

} // namespace mm6
