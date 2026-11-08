---
type: synthesis
up: "[[Campaign-Status]]"
related: ["[[MM7-Level-Seven-Artifacts]]", "[[Agent-Handoff]]"]
tags: [mm7, combat, spells, items, audit]
---

# MM7 Combat Audit Fixes

Five ranked MM7 gameplay defects have focused headless regressions and fixes on `codex/mm7-combat-fixes`, pending integration.

| Defect | Verified change |
| --- | --- |
| Break Weapon | An equipped weapon is eligible for the special attack. An unarmed MM7 target returns safely without sampling an empty list. |
| Lich Mind and Body immunity | Incoming Mind and Body damage is zero for an MM7 lich. Fire damage and non-lich Mind damage remain nonzero. |
| Divine Restoration | The potion clears the target's removable conditions while retaining the drinker's conditions. |
| Flaming archer projectile | The MM7 `FireAr` token resolves to a flaming arrow while the older `ARROWF` spelling still resolves. |
| Divine Intervention | Higher Light skill shortens the spell's base recovery rather than replacing it with a longer delay. |

Each defect test failed on the preceding source. The fixed MM7 GameTest suite passed **6/6** under network denial,
including the unarmed Break Weapon control. Three MM6 combat children passed separately in fresh processes.
The full MM6 first-map gate and its MM7 control are still pending on this branch. The parser test checks the token
rather than observing an archer fire in live combat. No graphical acceptance or installed helper update follows from
these tests.

Title checks retain MM6's existing Break Weapon and Divine Intervention behavior. The MM6 direct-use guard blocks the
Divine Restoration potion before the shared case. If MM6 Break Weapon is admitted for an unarmed target, its old
empty-list path can still assert. Claude owns that MM6 rule. MM7 lich Spirit immunity remains an unresolved rule
decision, so this fix covers only Mind and Body. Other ranked combat defects in the
[audit results](../project/MM7_AUDIT_RESULTS_2026-11-07.md) remain open.

See also: [Campaign Status](Campaign-Status.md), [MM7 Level-Seven Artifacts](MM7-Level-Seven-Artifacts.md), [Agent Handoff](Agent-Handoff.md).
