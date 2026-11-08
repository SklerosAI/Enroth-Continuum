---
type: synthesis
up: "[[Campaign-Status]]"
related: ["[[MM7-Level-Seven-Artifacts]]", "[[Agent-Handoff]]"]
tags: [mm7, combat, spells, items, audit]
---

# MM7 Combat Audit Fixes

Five ranked MM7 gameplay defects have focused headless regressions and fixes integrated on `mm6-mac-implementation` at `f77d8de31`. The monster second-spell ability and Energy parser fixes are also integrated. The event Day of the Gods correction is verified on `codex/mm7-combat-fixes` and awaits integration.

| Defect | Verified change |
| --- | --- |
| Break Weapon | An equipped weapon is eligible for the special attack. An unarmed MM7 target returns safely without sampling an empty list. |
| Lich Mind and Body immunity | Incoming Mind and Body damage is zero for an MM7 lich. Fire damage and non-lich Mind damage remain nonzero. |
| Divine Restoration | The potion clears the target's removable conditions while retaining the drinker's conditions. |
| Flaming archer projectile | The MM7 `FireAr` token resolves to a flaming arrow while the older `ARROWF` spelling still resolves. |
| Divine Intervention | Higher Light skill shortens the spell's base recovery rather than replacing it with a longer delay. |
| Monster second-spell ability | Projectile, Meteor Shower, Sparks and Sharpmetal sprites carry the selected spell ability. The MM6 single-projectile path keeps its existing first-slot stamp. |
| Monster Energy attacks | MM7 `Ener` in either attack field parses as Energy (12), while `Earth` remains Earth (3). The separate MM6 parser is unchanged. |
| Event Day of the Gods | MM7 event Master and Grandmaster power now follows the ordinary cast: `4 * level + 10` and `5 * level + 10`; the existing `4 * level` and `5 * level` hour durations remain. MM6 refuses this event source before the shared case. |

Each of the five branch defect tests failed on the preceding source. The fixed MM7 combat GameTest suite passed **6/6** under network denial,
including the unarmed Break Weapon control. Three MM6 combat children passed separately in fresh processes.
The three integrated monster-spell GameTests failed on the preceding source and passed **3/3** after its fix; three MM6 combat children also passed.

On the combined source, the full MM6 first-map gate passed its seven-child chain, retry fault children,
paperdoll, normal Save/Load and production load/resave, plus MM7 controls. The full MM7 headless suite
passed **467 tests with 2 skipped** out of 469. These were network-denied headless runs; the MM6 gate
overlapped other agents' GameTests but ran its own children sequentially.

The Energy parser's synthetic MM7 primary and secondary `Ener` assertions failed before repair and passed with Earth and MM6 controls afterward. The Day of the Gods event test failed at level 10 on the two swapped power values, then passed with both duration assertions. On the combined Energy and Day of the Gods source, the full MM6 first-map gate passed its seven-child chain, retry faults, paperdoll, normal Save/Load, production load/resave, and MM7 controls. The full network-denied MM7 headless suite passed **469 tests with 2 skipped** out of 471. The Day of the Gods branch and this audit update still await integration.

The Energy correction intentionally changed `Issues.Issue403_970`: the energy-only integration head failed its old RNG checkpoints at 4100 ms, while a controlled one-line reversal to the old Earth mapping passed. A retrace of the public fixture changed only 108 RNG checkpoints and was byte-stable on repeat. The test still asserts Shoals-to-Lincoln transition and all four Armor Class values. This is a source-caused trace update, not a workaround for a private-data mismatch.

The integrated monster ability stamp changed two existing MM7 trace outcomes. `Issues.Issue1524` needed
new RNG states, and `Issues.Issue1972` needed new end HP. The two JSON retraces were stable on repeat.
The corrected slot-two damage reduced the party's total HP by 689 rather than the old first-slot result
of 934. The `Issue1972` independent damage floor now requires at least 650 damage; its map and 24-meteor
checks remain. The parser test checks the token rather than observing an archer fire in live combat.
No graphical acceptance or installed helper update follows from these tests.

Title checks retain MM6's existing Break Weapon and Divine Intervention behavior. The MM6 direct-use guard blocks the
Divine Restoration potion before the shared case. If MM6 Break Weapon is admitted for an unarmed target, its old
empty-list path can still assert. Claude owns that MM6 rule. MM7 lich Spirit immunity remains an unresolved rule
decision, so this fix covers only Mind and Body. The monster second-spell tests check projectile ability stamps; Meteor Shower positioning remains open. Other ranked combat defects in the
[audit results](../project/MM7_AUDIT_RESULTS_2026-11-07.md) remain open.

See also: [Campaign Status](Campaign-Status.md), [MM7 Level-Seven Artifacts](MM7-Level-Seven-Artifacts.md), [Agent Handoff](Agent-Handoff.md).
