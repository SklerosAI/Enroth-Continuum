#pragma once

#include <cstdint>
#include <span>

#include "Engine/GameTitle.h"

#include "MM6CoreSaveFields.h"

struct Party;
class Character;

namespace mm6 {

/** Preserve durable modifiers from one actual Character without live-party mutation. */
CharacterModifierFields captureCharacterModifiers(const Character &character, GameTitle title);
Character stageCharacterModifiers(const Character &live, const CharacterModifierFields &saved,
                                  GameTitle title);

/** Copy the current quick spell and spellbook page on detached Character state. */
CharacterSpellPreferences captureCharacterSpellPreferences(const Character &character, GameTitle title);
Character stageCharacterSpellPreferences(const Character &live, const CharacterSpellPreferences &saved,
                                         GameTitle title);

/** Capture only the documented PART/CHAR field subset from an MM6 party. */
CoreSaveFields captureCoreFields(const Party &party, GameTitle title);

/**
 * Decode and validate the complete container, then apply its core fields to a
 * copy of the supplied party. The caller's live party is never mutated here.
 * The result is not a complete save restore until all other field families are
 * decoded and staged alongside it.
 */
Party stageCoreParty(const Party &live, std::span<const std::uint8_t> bytes, GameTitle title);

} // namespace mm6
