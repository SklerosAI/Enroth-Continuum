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

} // namespace mm6
