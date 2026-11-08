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

} // namespace mm6
