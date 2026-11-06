---
type: synthesis
up: "[[MM6-F4-Creation-Rules]]"
extends: "[[MM6-F4-Creation-HP-SP-Init-Gate]]"
tags: [mm6, f4, creation, mana, conditions]
---

# MM6 F4 Max-Mana Condition Trace

The [source packet](../project/MM6_F4_MAX_MANA_CONDITION_TRACE.md) maps the original max-mana outer expression and a nested condition-selection correction.

The original class dispatch returns zero for IDs 0–2, uses Personality for 3–5 and 9–11, Intellect for 6–8 and 12–14, and the sum of separate stat bonuses for 15–17. Its family base, class factor, actual level, Meditation, item and full-mana terms and zero floor match the current MM6 `GetMaxMana()` structure. The parameter-bonus array, age modifiers and active condition columns agree with current source; private six-family bases and eighteen factors remain in the user profile.

The original actual-stat helper selects the first active condition in its 17-condition priority order, with a neutral result when none applies. The inherited helper multiplied all active conditions. `mm6::actualStatCondition()` now supplies the original title-checked selection to `Character::GetActualStat()` only for MM6; MM7 retains its previous branch. A pure test covers all priority entries and neutral/foreign-title cases; a scoped private headless test checks two simultaneous conditions against resulting actual stats and max HP/SP while restoring its Party snapshot.

The exact item-bonus and Meditation/helper semantics, promoted levels and post-entry character-sheet/save values remain open. This is a bounded condition correction, not a claim that the entire max-mana formula is verified in live MM6 gameplay. Quick Start and normal MM6 SaveLoad remain guarded.

See also: [Creation Rules](MM6-F4-Creation-Rules.md), [Creation HP/SP Init Gate](MM6-F4-Creation-HP-SP-Init-Gate.md), [Quick Start](MM6-F4-Quick-Start.md).
