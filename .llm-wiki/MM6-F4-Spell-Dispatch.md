---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, spells, effects]
---

# MM6 F4 Spell Dispatch

The [F4 effect packet](../project/MM6_F4_SPELL_EFFECT_DISPATCH.md) records a checked boundary for every regular MM6 spell effect while the title-specific effects remain unimplemented.

The retained F3-matched spell and guide reports cover all 99 IDs, but inherited case presence and matching names do not establish the MM6 target or effect. Every regular MM6 ID now rejects at player queue and executor, event, monster and damage entry points before the inherited effect path; a functional wand rejects before charge or item loss. Bow and blaster player projectiles remain separate non-spell actions. MM7 dispatch remains unchanged. The native guard tests cover all IDs and sources.

The [First Aid effect slice](../project/MM6_F4_FIRST_AID_EFFECT.md) adds a checked private three-rank fixed-heal rule and a character-only MM6 branch for one spell. It is compiled and unit-tested, but the outer queue guard still blocks live casting until target UI, recovery, failure, shared healing semantics and audiovisual side effects are verified. Exact values and the reviewed guide locator remain in ignored local data.

The [player-cast transaction audit](MM6-F4-Player-Cast-Transaction.md) traces the input, picker, cancellation, executor, failure and cue order. A read-only First Aid preflight now composes the private cost and heal rules for a narrow spellbook input snapshot; it does not enable live casting.

The [Power Cure effect slice](../project/MM6_F4_POWER_CURE_EFFECT.md) adds a checked private base-plus-skill party-heal rule and bounded MM6 branch. Its queue/executor guard remains closed for the same transaction reasons, and the inherited Grandmaster learning gate is not used for MM6. Both effect slices are compiled code with private-backed tests, not observed casts.

The [timed cure cohort](../project/MM6_F4_TIMED_CURE_COHORT.md) groups six one-character, one-condition effects under a common private three-rank age-window manifest. Two IDs collide with unrelated inherited MM7 effects, so MM6 has numeric-ID interception before that switch and title-specific target routing. The shared helper validates all inputs and removes only a recent matching condition; absent, expired and unrelated conditions remain. Awaken, dead/eradicated cures, and poison/disease severity groups remain separate open work. The outer cast guard still blocks all six live casts.

The [Shield/Heroism cohort](../project/MM6_F4_COMBAT_BUFF_COHORT.md) adds two private-backed character buff plans with one-member Normal and party-wide Expert/Master scope. The profile carries their shared duration and Heroism power; Shield's effect slot is active without scalar power. The retained/guide disagreements for Haste, Stone Skin and Bless, plus unresolved MM6 resistance channels for the five protection spells, keep those outside this cohort. The inherited missile and damage consumers remain provisional. The live cast guard still blocks both spells.

The ignored private companion page `local-game-data/private-wiki/mm6/F4_SPELL_DISPATCH_2026-11-06.md` has per-ID retained/source/guide joins and source fingerprints. It is a research handoff, not a validated effect specification. Resolve rank-specific targets, effects, damage, visuals, sound and persistence before allowing individual IDs; event, monster, scroll and wand sources need separate checks. Runtime rejection is an exception and F5 graphical play has not occurred.

See also: [MM6 F4 Offline Seams](MM6-F4-Offline-Seams.md), [MM6 F4 Spell Rules](MM6-F4-Spell-Rules.md), [MM6 F4 Spell Items](MM6-F4-Spell-Items.md), [Private Companion Wiki](Private-Companion-Wiki.md).
