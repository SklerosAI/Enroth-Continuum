#pragma once

#include "Engine/GameTitle.h"
#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Spells/SpellEnums.h"

namespace mm6 {

enum class FirstAidCastRoute {
    Unsupported,
    Spellbook,
    Quickspell,
    Scroll,
    NpcOrTemple,
    Wand
};

// A read-only description of an ordinary spellbook cast. All booleans default
// to the conservative case; callers must establish each supported condition.
struct FirstAidPreflightInput {
    GameTitle title = GameTitle::MM7;
    FirstAidCastRoute route = FirstAidCastRoute::Unsupported;
    SpellId spell = SPELL_NONE;
    Mastery mastery = MASTERY_NONE;
    int skillLevel = 0;
    int casterIndex = -1;
    int targetCharacterIndex = -1; // -1 means the character picker has not resolved.
    int availableMana = -1;
    bool spellKnown = false;
    bool casterCanAct = false;
    bool casterInRecovery = true;
    bool casterCursed = true;
    bool partyInvisible = true;
    bool hasHeldItem = true;
    bool turnBasedMode = true;
    bool underwater = true;
    bool targetCanReceiveHealing = false;
};

enum class FirstAidPreflightState {
    NeedsCharacterTarget,
    InsufficientMana,
    InputsReady
};

struct FirstAidPreflightPlan {
    FirstAidPreflightState state;
    int casterIndex;
    int targetCharacterIndex;
    int manaCost;
    int recoveryTicks;
    int healAmount;
};

// This does not open the picker, debit mana, set recovery, heal, or authorize
// execution. Unsupported routes/conditions throw before returning a plan.
FirstAidPreflightPlan planFirstAidSpellbookCast(const FirstAidPreflightInput &input);

} // namespace mm6
