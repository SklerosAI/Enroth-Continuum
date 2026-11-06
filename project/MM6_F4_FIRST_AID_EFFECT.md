# MM6 F4 First Aid effect slice

Status: **private-backed fixed-heal branch and detached character-state transaction implemented, live casting still blocked** (2026-11-06). This is one bounded spell-effect category, not general MM6 spell support.

## Evidence chain

The retained `spell-effect-probe.json` identifies spell ID 68 and its three rank clauses; its `spells.txt` hash matches the accepted F3 original-file reconciliation. The ignored private guide matrix ties the same ID to a visually checked guide page, one party member as target and matching Normal/Expert/Master healing amounts. `scripts/mm6_f4_export_first_aid_rules.py` verifies those joins and writes a mode-0600 `rules/first-aid.json` in the local MM6 profile. The exact effect amounts and guide transcription remain private. The native loader checks format, ID, fingerprints, three bounded ordered integers and atomic replacement on failure.

The prior inherited case scaled healing with skill and also permitted an actor target. The MM6 branch now reads the private three-rank amount and calls the target character's healing method only for a bounded party-member index with no actor target. MM6 target selection is separately marked as character-only. MM7 keeps the inherited case and target routing.

## Execution boundary

A [detached First Aid transaction](MM6_F4_PLAYER_CAST_TRANSACTION.md) now takes an `InputsReady` plan and four copied `Character` records. It validates title, indices, mana, recovery, conscious target and supplied maximum HP before changing any copy; it stages mana debit, bounded HP gain and caster recovery together. It accepts self-targeting. The caller must supply a current MM6 maximum HP and own the final live commit, target picker, cues and queue cleanup. Three focused tests cover success, self-targeting, late invalid state without mutation and a private-backed rule-plan-to-character-stage path.

The outer MM6 effect guard still rejects ID 68 before the queue or effect switch. The new branch is therefore compiled and tested as an effect rule, but **is not reachable in live MM6 casting**. Removing that guard requires a verified transaction for target selection, held-item handling, the provisional 300-tick target-pick recovery, curse failure, mana/recovery application, status conditions, visual and audio cues, cancel behavior and persistence. The branch currently uses the shared `Character::Heal` semantics; MM6-specific maximum HP, unconscious/dead/zombie and overflow behavior must be checked before enabling it. Scroll, event, monster, NPC and wand casts remain closed.

The detached stage is narrower than the inherited live `Character::Heal`: it accepts only a conscious, nonzombie target with positive HP. The inherited method can wake an unconscious character once HP rises above zero and halves a zombie's maximum HP. The private First Aid guide/table evidence fixes target scope and rank amounts but does not establish condition-specific behavior. These cases therefore need original MM6 behavior evidence before the stage can replace the live method; a successful normal-target test does not prove revival or zombie handling. `GetMaxHealth()` also still uses the provisional MM6 creation formula.

Focused native tests use deliberately synthetic amounts for all three ranks and malformed/reload edges. A private-backed test reads the ignored manifest and checks the runtime lookup; the exporter checks retained and visually reviewed guide claims without copying their values into Git. No original installation was reread and no partial MM6 game was launched.

See also: [effect dispatch boundary](MM6_F4_SPELL_EFFECT_DISPATCH.md), [spell mana/recovery](MM6_F4_SPELL_MANA_RECOVERY.md), [F3 gate](MM6_F3_GATE_REVIEW.md), [implementation checklist](MM6_IMPLEMENTATION_CHECKLIST.md).
