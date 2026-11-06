---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, spells, casting]
---

# MM6 F4 Player Cast Transaction

The [source-order audit](../project/MM6_F4_PLAYER_CAST_TRANSACTION.md) records book, quickspell, scroll/NPC/temple/wand, target selection, cancel, pre-effect, success and cue paths. It separates inherited engine behavior from the F3-matched MM6 mana/recovery and reviewed First Aid target/heal claims.

The pure `MM6FirstAidPreflight` helper composes private rules into a read-only plan for a narrow normal spellbook cast. It refuses unverified contexts and reports missing target or mana without mutation. `InputsReady` means only that this input snapshot meets the planner's bounds. The outer MM6 queue/executor effect guard remains closed, and no live cast was tested.

A detached `MM6FirstAidTransaction` now validates an `InputsReady` plan against four copied characters and stages mana debit, capped HP gain and caster recovery together. Its tests cover self-targeting, rejection after mana or target state changes, and a private-backed rule-plan-to-stage round trip. The caller still has to supply the current MM6 maximum HP, choose and cancel a target, order cues and commit to live state.

The detached transaction accepts conscious, nonzombie targets with positive HP. Inherited `Character::Heal` can wake an unconscious target and uses half maximum HP for a zombie, while the private MM6 First Aid guide/table claims do not settle these condition cases. Verify original MM6 behavior before widening the transaction or releasing the live effect guard; the current MM6 maximum-HP formula is provisional.

A detached Power Cure transaction now composes the private F3-matched spell mana/recovery row with the reviewed party-heal rule on four copied characters. It preflights all targets and caller-supplied maximum HP before any mana, health or recovery mutation. The bounded case requires four positive, condition-free targets, an ordinary known MM6 spellbook cast and an unchanged eligible caster. Synthetic tests check all-target atomicity and MM7 rejection; a private-backed test checks the rule-to-character-state result. The live ID-77 queue/effect guard remains closed. Condition-specific healing, max-HP authority, audiovisual cues, queue cleanup, final live commit and save/reload still need proof.

The largest transaction risks are target-picker relocation/drop of a held item, the inherited 300-tick recovery write on target selection, cursed failure order, invisible-party reset, healing side effects, cue order, and save/reload state. MM6 `Game::onEscape()` now calls the explicit queue-cancel helper before closing the picker; its live cues and recovery effect remain unobserved. The character target UI has indices 0–3, and MM6 `spellTargetPicked` now rejects out-of-range indices before changing its queued spell. A future execution path must still prove cancellation/failure atomicity before lifting an ID-specific guard.

The ignored companion `local-game-data/private-wiki/mm6/F4_PLAYER_CAST_TRANSACTION_2026-11-06.md` records the private F3 identity and guide locator without copying their values here.

See also: [MM6 F4 Spell Dispatch](MM6-F4-Spell-Dispatch.md), [MM6 F4 Spell Rules](MM6-F4-Spell-Rules.md), [MM6 F4 Offline Seams](MM6-F4-Offline-Seams.md).
