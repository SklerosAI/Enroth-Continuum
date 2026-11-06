---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, spells, effects]
---

# MM6 F4 Spell Dispatch

The [F4 effect packet](../project/MM6_F4_SPELL_EFFECT_DISPATCH.md) records a checked boundary for every regular MM6 spell effect while the title-specific effects remain unimplemented.

The retained F3-matched spell and guide reports cover all 99 IDs, but inherited case presence and matching names do not establish the MM6 target or effect. Every regular MM6 ID now rejects at player queue and executor, event, monster and damage entry points before the inherited effect path; a functional wand rejects before charge or item loss. Bow and blaster player projectiles remain separate non-spell actions. MM7 dispatch remains unchanged. The native guard tests cover all IDs and sources.

The ignored private companion page `local-game-data/private-wiki/mm6/F4_SPELL_DISPATCH_2026-11-06.md` has per-ID retained/source/guide joins and source fingerprints. It is a research handoff, not a validated effect specification. Resolve rank-specific targets, effects, damage, visuals, sound and persistence before allowing individual IDs; event, monster, scroll and wand sources need separate checks. Runtime rejection is an exception and F5 graphical play has not occurred.

See also: [MM6 F4 Offline Seams](MM6-F4-Offline-Seams.md), [MM6 F4 Spell Rules](MM6-F4-Spell-Rules.md), [MM6 F4 Spell Items](MM6-F4-Spell-Items.md), [Private Companion Wiki](Private-Companion-Wiki.md).
