# MM6 baseline actor ID reconciliation — F3 supplement

Status: **original-content corroboration from retained extracts, 2026-11-06; no runtime behavior claim**. This supplement closes the specific F3 question about the 548-byte baseline actor record's monster-ID field and the presence of ID 173 in a shipped map delta. It does not complete the wider monster/combat row or the full F3 gate.

The [pinned MM6 `Creature.java` schema](https://github.com/angeld29/mm8leveleditor/blob/bf3b8c4a86100635a5fc6ff53fac6e4e0af5cb53/src/org/gamenet/application/mm8leveleditor/data/mm6/Creature.java#L30-L53) defines a 548-byte MM6 creature record, an **unsigned one-byte monster ID at offset 52**, and the adjacent one-byte level at offset 53. Its [record-field view](https://github.com/angeld29/mm8leveleditor/blob/bf3b8c4a86100635a5fc6ff53fac6e4e0af5cb53/src/org/gamenet/application/mm8leveleditor/data/mm6/Creature.java#L683-L701) independently labels offset 52 as `MONSTER_ID`. A separate 16-bit `MONSTER_ID_TYPE` field is listed at offset 118; it must not be substituted for the byte at 52. This is an external editor schema, not a trace of the shipped game's executable.

## Retained-data check against F3 original identity

The private offline script read only the 67 retained `.dlv`/`.ddm` extracts. Before interpreting each record, it required that the corresponding F3 content row had passed original-member capture, decoded-byte equality to the retained extract, and F2 catalog equality; it then rehashed the retained wrapper against the F3 and frozen F2 reports. It decoded the delta under bounded section rules, checked the decoded hash and exact 548-byte actor-section span against the F2 world probe, and checked every extracted ID against both the F2 spawn probe and the 1–173 monster text-table ID set. The matched original `dmonlist.bin` has 173 descriptor records. No original installation file was reopened and no MM6 runtime was launched.

| Check | Result |
| --- | ---: |
| Original-matched retained map deltas checked | 67/67 |
| Bounded baseline actor records | 342 |
| Distinct IDs at byte 52 | 24 |
| Records at byte 52 outside monster table IDs 1–173 | 0 |
| Baseline records whose byte 52 is ID 173 | **1** |
| Monster text and descriptor rows | 173 each |

The ID 173 record is in a shipped baseline delta whose original archive member was already byte-matched in F3. Thus the prior candidate is a **genuine source-data reference under the pinned actor schema**, not a guessed text-table tail or a retained-extract discrepancy. The exact map, record index, wrapper/decoded/record hashes, neighboring bytes, and all per-map actor IDs remain in ignored `local-game-data/mm6/research-2026-11-05/F3_ACTOR_ID_RECONCILIATION_2026-11-06.json`; the ignored reproduction script is beside it. The existing `monster-spawn-links-probe.json` provided an independent retained-only count, and this pass matched it record for record.

## F4/F5 decision and residual ambiguity

The current MM6 descriptor load path discards the final row. F4 should make the MM6 descriptor table addressable through **all 173 IDs**, use the title-specific one-byte actor field at offset 52 with bounds checks, and retain the separate offset-118 field without assigning it monster-ID meaning. An offline fixture should parse all 342 baseline records, confirm the single ID 173 lookup resolves, reject invalid/truncated actor records, and preserve unknown actor bytes when serializing map state.

This evidence does **not** show whether the shipped executable activates, draws, fights with, or suppresses that particular baseline actor, nor whether scripted/dynamic spawns use additional IDs. F5 should load its map, inspect the actor if reachable, and verify encounter and save/reload behavior. Other monster attack, loot, AI, frame and resistance semantics remain open under the [monster F2 packet](MM6_MONSTER_SPAWN_LINKS_F2.md) and the [F3 matrix](MM6_F3_PREFLIGHT_MATRIX.md). No F2 byte/schema assumption must be reopened for this ID field; the current final-descriptor discard is an F4 consumer correction.
