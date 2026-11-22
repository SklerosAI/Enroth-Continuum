#pragma once

struct Party;

namespace mm6 {

/**
 * Full rest and heal of a party that isn't the live one, as the MM6 border confirmation applies it to its party copy.
 * Buffs, conditions, health, mana and the no-rest counter of `party` change as if it were the live party, while the
 * live party stays untouched.
 *
 * @param party                         Party to rest.
 */
void restAndHealDetachedParty(Party &party);

/**
 * MM6's check behind "There are hostile enemies near!" when the party tries to rest.
 *
 * @offset 0x42A680
 * @return                              Whether a monster that isn't friendly, dying, dead or removed stands closer to
 *                                      the party than 2560 units indoors or 5120 outdoors, by MM6's octagonal distance.
 */
bool hostileMonstersNearParty();

/**
 * Notes that the party has rested, as MM6.exe flags it in its travel rest (0x4962CC), which the coach, the boat, a
 * border trip and training go through, and when the rest screen closes (0x42BC4B).
 */
void notePartyRested();

/**
 * MM6's check after a rest, which MM6.exe makes on every pass over its messages while the game screen shows
 * (0x410808). If the party rested since the last check, Prince Nicolai is hired and the "Entertain Nicolai" quest bit
 * is set, runs global event 20, in which he disappears.
 */
void runPartyRestedEvent();

} // namespace mm6
