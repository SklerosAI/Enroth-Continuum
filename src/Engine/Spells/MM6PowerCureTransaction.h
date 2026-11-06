#pragma once

#include <array>

#include "Engine/GameTitle.h"
#include "Engine/Objects/Character.h"
#include "Engine/Spells/SpellEnums.h"

namespace mm6 {

enum class PowerCureCastRoute {
    Unsupported,
    Spellbook,
    Quickspell,
    Scroll,
    NpcOrTemple,
    Wand
};

struct PowerCureTransactionInput {
    GameTitle title = GameTitle::MM7;
    PowerCureCastRoute route = PowerCureCastRoute::Unsupported;
    SpellId spell = SPELL_NONE;
    Mastery mastery = MASTERY_NONE;
    int skillLevel = 0;
    int casterIndex = -1;
    int expectedCasterMana = -1;
    bool spellKnown = false;
    bool partyInvisible = true;
    bool hasHeldItem = true;
    bool turnBasedMode = true;
    bool underwater = true;
    bool hasActorTarget = true;
    std::array<int, 4> maxHealth = {};
};

struct PowerCureStateChange {
    int casterMana;
    std::array<int, 4> health;
    Duration casterRecovery;
};

// Stages one ordinary MM6 Power Cure cast on detached character records.
// The caller still owns the live queue, visuals, audio, save and final commit.
PowerCureStateChange stagePowerCureOnCharacters(const PowerCureTransactionInput &input,
                                                std::array<Character, 4> *characters);

}  // namespace mm6
