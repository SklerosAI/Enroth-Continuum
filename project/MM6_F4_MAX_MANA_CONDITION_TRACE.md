# MM6 max-mana condition trace — F4

Status, 2026-11-06: the accepted F3-identity original executable's complete **outer** max-mana class dispatch and arithmetic were traced. Its nested actual-stat condition selection differs from the inherited fork. A bounded MM6-only condition-priority correction is implemented; item and skill helper equivalence, promotion behavior and post-entry sheet values remain open. MM7 and normal SaveLoad behavior are unchanged.

## Original outer formula and F3 join

The original function has four exits: class IDs 0–2 return zero; 3–5 and 9–11 use Personality; 6–8 and 12–14 use Intellect; 15–17 add the two **separate parameter bonuses**. For a mana class, it adds family SP base, class SP factor times actual level plus the selected stat bonus, Meditation/skill bonus, equipped-item bonus and signed full-mana bonus; a negative final result becomes zero. The six bases and eighteen factors come from the corrected private F3 report/profile manifest. No numeric array is copied here.

That outer expression matches the current MM6 `Character::GetMaxMana()` branch at the term level. The original parameter threshold and bonus tables match the inherited current arrays. Its seven four-column age-modifier rows and first seventeen condition-modifier columns also match the current tables. These checks were performed against the identity-matched staged executable and current source; they do **not** prove that every nested item, skill, age or condition helper has the same behavior in all states.

## Source-backed condition mismatch and bounded correction

The original actual-stat helper scans a fixed 17-condition priority order and uses the **first active condition**, or a neutral factor if none is active. The fork's inherited `GetActualStat()` multiplied every active condition for all titles. With two conditions, that can change actual Endurance, Personality or Intellect and hence maximum HP/SP. The original neutral slot corresponds to no condition; the inherited extra Zombie slot has a different meaning and cannot be reused as MM6's neutral slot.

`mm6::actualStatCondition()` now selects only original MM6 conditions in the proved order and returns `CONDITION_GOOD` as the neutral sentinel. It rejects foreign titles. `Character::GetActualStat()` uses this selection only when the engine title is MM6, reading the already-present modifier table for conditions 0–16; MM7 retains its prior `hasNone` and cumulative loop. Original severe-condition priority and neutral factors are 100%, so the MM6 path also handles combinations involving death/petrification/eradication without importing the MM7 Zombie column. This corrects one nested term shared by HP and SP without changing private class arrays or arithmetic in `GetMaxHealth()`/`GetMaxMana()`.

## Verification and remaining branches

A pure unit case checks neutral, all seventeen priority entries, simultaneous medium disease and drunkenness, and foreign-title rejection. A scoped headless MM6 GameTest keeps an owned Party snapshot and restores it even on exception; it uses an empty synthetic character with two active conditions, then verifies actual Personality/Endurance and resulting maximum SP/HP against the original condition factors and the private F3 class base/factor manifest. The test has no disk, UI or SaveLoad mutation. The production, unit and GameTest translation units passed arm64 syntax compilation with integrated build flags; standalone priority execution passed. The integrated linked unit suite passed **2/2** and monitored private GameTest passed **1/1** over 89 samples with no visible window or focus change.

Remaining proof before claiming complete max-mana fidelity: compare the original equipped-item bonus for mana, Meditation/mastery and actual-skill modifiers, the original actual-stat age/condition arithmetic under all statuses, and promoted-class/level behavior with the fork's helpers. Verify a nonzero post-entry sheet and save/reload across no-mana, single-stat and dual-stat classes. A result from the Quick Start finalizer alone does not prove subsequent map-entry state. Keep the original Quick Start route and live MM6 SaveLoad guarded.

See [creation foundation](MM6_F4_CREATION_RULES_FOUNDATION.md), [HP/SP initialization gate](MM6_F4_CREATION_HP_SP_INIT_GATE.md), and [F3 executable follow-up](MM6_F3_EXECUTABLE_FOLLOWUP.md).
