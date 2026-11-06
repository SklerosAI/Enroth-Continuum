# MM6 spell mana and recovery — F4 bounded rule seam

Status: **private F3-backed data and MM6 player-cast selection, offline checks only, 2026-11-06**. This does not establish spell effects or campaign playability.

## Source and private profile

The corrected F3 PE v2 report has a reserved row plus 99 MM6 spell rows. Its cost and Normal/Expert/Master recovery triples were matched to the accepted F3 gate report hash. The [F3 executable follow-up](MM6_F3_EXECUTABLE_FOLLOWUP.md) found a code consumer of the executable cost fields. For the single cost disagreement with retained `spells.txt`, this seam provisionally uses the executable row. Exact per-spell values remain in the ignored owner profile, not Git.

Run `scripts/mm6_f4_export_spell_rules.py` with the ignored corrected PE v2 report and `F3_GATE_REPORT_HASHES_2026-11-06.json`, writing to the MM6 profile's `rules/spells.json`. The exporter verifies all 99 ordered IDs, three rank cells per row, nonnegative signed-16-bit costs, positive signed-16-bit recovery and nonincreasing delay by mastery. It refuses overwrite and creates a mode-0600 file. The C++ loader validates the same bounded shape and report-hash syntax before replacing its table. MM6 initialization reads the manifest through the private user filesystem; MM7 does not load it.

## Dispatch boundary

The regular player-cast cost/recovery selection in `CastSpellInfo.cpp`, the quickspell mana gate and two spell-cost popups now read MM6's private table for Normal, Expert and Master. The inherited MM7 `pSpellDatas` remains unchanged. MM6 scroll/NPC override casts and debug Grandmaster casts fail closed at the central selection point because their cost/recovery policy is unverified. IDs outside 1–99 and ranks outside 1–3 are rejected. A focused native test covers every 99 × 3 lookup, invalid shapes/ranges, atomic failed reload, and the actual private profile manifest without printing its values.

This is a **rule-data seam**. The inherited numeric-ID effect switch, damage factors, target selection, spellbook mastery gate, scroll/book item mapping, event casts, monster casts, spell failure/cancellation, cost modifiers, recovery modifiers, portal/beacon book debits and timing, wand recovery, sprites and sounds remain unverified or unmapped for MM6. A player cast can still reach an unrelated inherited effect; do not use this seam as evidence that any spell works. F4 needs separate title-specific dispatch and offline effect checks for all 99 IDs. F5 needs visible casts and mana/recovery observations across schools and save/reload.
