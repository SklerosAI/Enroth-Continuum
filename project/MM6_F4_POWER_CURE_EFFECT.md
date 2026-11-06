# MM6 F4 Power Cure effect slice

Status: **private-backed party-heal runtime branch implemented, live casting still blocked** (2026-11-06). This is a bounded effect rule for spell ID 77, not a working MM6 spell.

The retained `spell-effect-probe.json` describes party-wide healing with a base and per-skill term. Its `spells.txt` hash matches the accepted F3 original-file reconciliation. The ignored guide matrix has a matching party target and formula on a visually inspected guide page. `scripts/mm6_f4_export_power_cure_rules.py` checks those joins, then writes a mode-0600 `rules/power-cure.json` in the local MM6 profile. The exact coefficients and guide transcription remain private. The native loader checks the ID, source fingerprints and bounded positive coefficients before atomically replacing the rule.

The inherited case uses a different skill coefficient. The new MM6 branch rejects an actor target, looks up the amount for a bounded stored skill level and one of the three MM6 mastery ranks, then applies that amount to every party character. The shared `Character::Heal` routine remains the health mutation seam. MM7 continues using its inherited branch.

The outer MM6 spell-effect guard still rejects ID 77 before queueing or execution. The branch is compiled and its formula is unit-tested, but **no live MM6 cast can reach it**. Before opening that gate, verify the full target, mana/recovery, curse/failure, party health/status, visual/audio and save transaction. The inherited spellbook Grandmaster requirement is not an MM6 learning rule; book learning remains closed. Scroll, event, monster and NPC casting remain closed. No game was launched and the original installation was not reread.

See also: [First Aid slice](MM6_F4_FIRST_AID_EFFECT.md), [effect boundary](MM6_F4_SPELL_EFFECT_DISPATCH.md), [mana/recovery](MM6_F4_SPELL_MANA_RECOVERY.md), [F3 gate](MM6_F3_GATE_REVIEW.md).
