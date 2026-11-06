#pragma once

#include "Engine/PartyEnums.h"
#include "Utility/IndexedBitset.h"

// Raw MM6 EVT variable 16 is a party quest bit. Text in quests.txt is a
// presentation concern: shipped scripts also use rows with empty text.
class MM6QuestBitState {
 public:
    explicit MM6QuestBitState(IndexedBitset<QBIT_FIRST, QBIT_LAST> &bits) : _bits(bits) {}

    bool compare(int rawId, int value) const;
    bool add(int rawId, int value);
    bool subtract(int rawId, int value);
    bool set(int rawId, int value);

 private:
    IndexedBitset<QBIT_FIRST, QBIT_LAST> &_bits;

    static QuestBit checkedBit(int rawId, int value);
};
