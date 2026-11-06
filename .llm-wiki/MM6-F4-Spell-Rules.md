---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, spells, mana, recovery]
---

# MM6 F4 Spell Rules

The [F4 spell packet](../project/MM6_F4_SPELL_MANA_RECOVERY.md) records the private F3-backed mana and recovery seam and its cast-route limits.

The accepted corrected PE report provides 99 ID-ordered spell rows with Normal, Expert and Master cost/recovery triples. A checked exporter writes them to owner-only `rules/spells.json` in the MM6 profile; the engine validates the manifest before loading. The one executable/text cost disagreement uses the executable row provisionally because the F3 follow-up found an executable cost consumer. MM6 regular player-cast selection, the quickspell mana gate and cost popups read this title-specific table, while MM7's static spell data remains untouched.

This does not resolve the inherited spell effect switch. The [effect-dispatch boundary](MM6-F4-Spell-Dispatch.md) now rejects all 99 MM6 regular spells before inherited player, event, monster and damage effects. [Spell-item IDs](MM6-F4-Spell-Items.md) have a bounded MM6 mapping, and a private class/school availability gate exists, but book learning remains closed until per-spell required mastery is established. Scroll/NPC override casts and debug Grandmaster casts fail closed; portal/beacon book debits, dynamic modifiers, failure timing, sound and visual effects require separate work. The native tests cover all 297 table cells and malformed or out-of-range input, including the private manifest. No graphical cast or campaign outcome has been verified.

See also: [MM6 F4 Offline Seams](MM6-F4-Offline-Seams.md), [MM6 F4 Spell Dispatch](MM6-F4-Spell-Dispatch.md), [MM6 F4 Map Bytes](MM6-F4-Map-Bytes.md), [Agent Handoff](Agent-Handoff.md).
