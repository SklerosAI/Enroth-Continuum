#pragma once

#include <utility>

#include "NPCEnums.h"

#include "Utility/Segment.h"

inline Segment<MerchantPhrase> allMerchantPhrases() {
    return {MERCHANT_PHRASE_NOT_ENOUGH_GOLD, MERCAHNT_PHRASE_STOLEN_ITEM};
}

inline Segment<NpcProfession> allNpcProfessions() {
    // This inherited range is used by MM7's NPC distribution table, whose
    // next row after Hunter is a Totals footer. MM6's npcprof loader handles
    // its larger 77-row table separately.
    return {NPC_PROFESSION_FIRST, NPC_PROFESSION_HUNTER};
}

inline Segment<NpcProfession> allMm6RandomNpcProfessions() {
    // MM6's terminal row has no numeric Chance field and is not sampled.
    return {NPC_PROFESSION_FIRST, static_cast<NpcProfession>(std::to_underlying(NPC_PROFESSION_MM6_LAST) - 1)};
}
