# MM6 F4 player-cast transaction audit

Status: **read-only First Aid spellbook preflight implemented; live MM6 effect gate closed** (2026-11-06). This packet distinguishes retained MM6 rule data from inherited OpenEnroth transaction behavior. No MM6 cast or graphical play was observed.

## Current source order

| Step | Current source | State or cue touched | MM6 proof and gap |
| --- | --- | --- | --- |
| Select from book | `Game.cpp` `UIMSG_SelectSpell` / `UIMSG_CastSpellFromBook` | Checks learned flag (or debug override), closes book, resumes timer, queues next-frame cast. | The learned flag is an inherited consumer; MM6 learning rules remain closed. |
| Quick cast | `KeyboardInputHandler.cpp` `INPUT_ACTION_QUICK_CAST`, then `Game.cpp` `UIMSG_CastQuickSpell` | Reads checked MM6 mana table, may fall back to attack, posts next-frame message; Game checks underwater and recovery. | Cost data are F3 matched; fallback, input timing and attack side effects are not proved as MM6 behavior. |
| Scroll, NPC, temple, wand | `CastSpellInfo.cpp` `pushScrollSpell` / `pushNPCSpell` / `pushTempleSpell`; `Character.cpp` wand/scroll callers | Supply overrides or consume/use item in source-specific paths. | Their override, item and failure transactions need independent review. Preflight rejects these routes. |
| Queue and target picker | `CastSpellInfo.cpp` `pushSpellOrRangedAttack` and `TargetedSpellUI_Character` | MM6 First Aid selects a character-only picker. Queue guard currently rejects before queue mutation. Without guard, opening the picker moves a held item into inventory or drops it (`Party.cpp::placeHeldItemInInventoryOrDrop`). | F3-matched guide target is one member. Held-item relocation or drop on a canceled cast is not established for MM6. |
| Cancel or choose | `CastSpellInfo.cpp` `cancelSpellCastInProgress`, `spellTargetPicked`; `Game.cpp` `onEscape` / `UIMSG_CastSpell_TargetCharacter` | Explicit cancel clears queue/UI/cursor/status and has no explicit mana debit. MM6 Escape now invokes it before closing the picker, so a pending targeted spell ID is removed. MM6 target index rejects values outside 0–3 before queue mutation. Choosing a valid target still sets caster recovery to 300 ticks before execution. | Exact MM6 cancel cues/recovery behavior and target-pick recovery remain unproved. |
| Executor pre-effect | `CastSpellInfo.cpp` `castSpell` | MM6 guard rejects before invisibility clearing. If enabled, executor clears invisibility, computes private mana/recovery, checks mana, then checks a cursed 50% failure branch; that branch sets failure recovery, plays failure cues, spends mana and exits. | MM6 mana/recovery table is F3 matched. Invisibility, curse chance/order, status and cue policy are inherited, not verified. |
| Success and cues | `CastSpellInfo.cpp` First Aid case and default tail; `Character.cpp::Heal`, `setSpellRecovery` | Case heals first. Default tail then spends mana, sets recovery (and portrait reaction), plays spell sound and clears queue. `Heal` may wake an unconscious target, caps at current max HP and ignores dead/eradicated. | Fixed three-rank First Aid heal and one-member target have retained/guide support. Shared max-HP, condition, cue and persistence semantics still require MM6 verification. |

## Read-only First Aid plan

`MM6FirstAidPreflight` composes the checked F3-matched 99-spell mana/recovery manifest with the ignored, guide-checked First Aid amount. It accepts only an ordinary book route for MM6 spell ID 68, mastery Normal/Expert/Master, an indexed caster and character target 0–3, positive skill, known spell, able and recovered caster, sufficient nonnegative mana, and a target whose condition has already been checked as healable. It refuses held items, curse, invisibility, underwater and turn-based contexts because their current transaction paths have unproved MM6 side effects. It reports `NeedsCharacterTarget`, `InsufficientMana`, or `InputsReady` without touching queue, UI, party, items, mana, conditions, recovery or cues. `InputsReady` is an input snapshot, **not authorization to execute**.

Synthetic native tests cover all three ranks, missing-target and mana states, title/ID/index/rank/route rejection, and representative unverified contexts. A private-backed test composes both ignored manifests without publishing values. The current player queue and executor guards continue to reject every regular MM6 spell before this plan is used in live play.

## Gate to execution

Prove or replace target-picker and cancellation behavior, including the side effects of the newly wired MM6 Escape cleanup; held-item policy, target-pick recovery, caster and target condition semantics, success/failure order, cue order and durable save state. Then add a real queue-to-executor transaction test that asserts no mutation on cancellation/failure and the correct single commit on success. Check scroll, NPC, temple, wand, quickspell, event and monster routes separately. Keep MM7 inherited behavior unchanged.

See also: [First Aid effect](MM6_F4_FIRST_AID_EFFECT.md), [spell dispatch](MM6_F4_SPELL_EFFECT_DISPATCH.md), [spell mana/recovery](MM6_F4_SPELL_MANA_RECOVERY.md), [F3 gate](MM6_F3_GATE_REVIEW.md).
