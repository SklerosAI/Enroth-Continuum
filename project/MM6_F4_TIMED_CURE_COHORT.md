# MM6 F4 timed single-condition cure cohort

Status: **six title-specific condition effects implemented behind the closed cast gate** (2026-11-06). The cohort consists of retained MM6 IDs 40, 49, 57, 60, 64 and 67: petrification, curse, fear, paralysis, insanity and weakness. This is a bounded condition-state rule, not live spell casting.

Update, 2026-11-07: ID 67, Cure Weakness, now passes the cast gate from the spellbook and quick cast (`MM6CombatHeadless.DISABLED_CureWeaknessFromSpellbook`). The other five IDs stay behind the gate. See the [spell dispatch packet](MM6_F4_SPELL_EFFECT_DISPATCH.md).

## Cohort proof

Each ID has a one-character target and the same three mastery-specific, per-skill condition-age window in the F3-matched retained spell table and the visually reviewed private guide matrix. `scripts/mm6_f4_export_timed_cure_rules.py` checks all six identities, target kinds, condition terms, rank windows, guide review flags and the accepted F3 `spells.txt` source hash before writing one mode-0600 private `rules/timed-cures.json` profile manifest. The exact windows, per-ID guide locators and source transcriptions remain in ignored local data. The native loader checks the exact six-ID set, source fingerprints, three bounded ordered windows and atomic replacement on failure.

The MM6 helper maps each ID to one condition, validates the ID, Normal/Expert/Master rank, stored skill level, current time and condition onset before mutation, and removes the condition only when its age is strictly inside that rank's window. It leaves absent, too-old and unrelated conditions unchanged. IDs 57 and 60 collide with inherited MM7 spell meanings, so MM6 execution branches before the inherited effect switch and MM6 target routing overrides the inherited actor target. MM7 continues using its original numeric dispatch.

The wider timed-condition group was reviewed and narrowed. Awaken has a party target and a normal-sleep distinction. Raise Dead and Resurrection change health and weakness as well as the condition. Poison and disease each have three severity states whose combined removal semantics remain unproved. These five spells stay closed. Other healing spells are outside this condition-age cohort.

## Execution boundary and verification

The outer MM6 guard still rejects all six IDs before queueing or effect execution. The helper and switch branch are compiled and tested, but no live MM6 cast can reach them. Before opening the gate, verify target UI and cancellation, held-item movement, provisional target-pick recovery, curse failure, mana/recovery, condition timestamp provenance, visual/audio cues and save/reload. The branch currently calls the shared condition storage; its original MM6 condition ordering and boundary behavior still need F5 observation or independent executable proof.

Focused native tests cover all six conditions across three ranks, the exact age boundary, unrelated and absent conditions, invalid ID/rank/skill/clock, malformed atomic reload and the private F3-matched manifest. No original installation was reread and no partial MM6 game was launched.

See also: [effect dispatch boundary](MM6_F4_SPELL_EFFECT_DISPATCH.md), [First Aid slice](MM6_F4_FIRST_AID_EFFECT.md), [Power Cure slice](MM6_F4_POWER_CURE_EFFECT.md), [F3 gate](MM6_F3_GATE_REVIEW.md).
