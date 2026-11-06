# MM6 F3 original-file gate review

Status: **F3 accepted for offline F4 implementation, 2026-11-06**. This is a source-data and specification gate, not evidence of a playable MM6 build. The original game was not launched in F3. F5 graphical, audible, campaign and save/load checks remain mandatory before an MM6 Play action.

## Frozen source and private evidence

The F2 source baseline is commit `a8593659e96e89b7d2f8cf0446061a46779612b1`. Its ignored freeze manifest fingerprints 94 tracked and 185 ignored research/source inputs. The read-only F3 session matched **23/23** original file identities, inventoried the original archive/media directories with zero bounds/name problems, and compared **348 targeted original members byte for byte** with the retained F1 extracts. The authoritative PE join is the private `F3_PE_RECONCILIATION_2026-11-05_v2.json`; its unsuffixed predecessor is superseded. Exact hashes, names, rows, addresses, offsets and source data remain in the ignored F3 reports and `local-game-data/private-wiki/mm6/F3_FAMILY_SIGNOFF_2026-11-05.md`. Later targeted original reads were hash-checked against the frozen reports; post-freeze source commits were never treated as a new freeze.

The [content](MM6_F3_CONTENT_RECONCILIATION.md), [PE](MM6_F3_PE_RECONCILIATION.md), [executable follow-up](MM6_F3_EXECUTABLE_FOLLOWUP.md), [media](MM6_F3_MEDIA_RECONCILIATION.md), [portrait/voice](MM6_F3_PORTRAIT_VOICE_RECONCILIATION.md), [actor-ID](MM6_F3_ACTOR_ID_RECONCILIATION.md) and [save-boundary](MM6_F3_SAVE_BOUNDARY.md) packets record the public evidence and its limits. Private report SHA-256 values are recorded in the dated ignored signoff so later agents can detect stale or replaced reports without publishing proprietary-derived rows.

## Family decisions

| F3 family | Exit decision and F4/F5 obligation |
| --- | --- |
| Archives and directories | Confirmed identity, bounded directory structure and membership. F4 must preserve **original Games LOD order** for index-based travel; normalized F1 `.entries` order is unsuitable. F5 renders representative resources. |
| 67 base/delta map pairs | All 134 original members match retained bytes. Two source-native checksum anomalies need narrow, hash-identified recovery; opaque nonzero state and transitions require F4 fixtures and F5 save/reload. |
| 30 text tables and six nonliteral roles | Original bytes/shapes confirmed. Each role needs an explicit F4 consumer or reserved decision; password/autonote/diagnostic provenance remains a named ambiguity. F5 checks notes, news, awards and combinations. |
| Creation, classes and skills | Executable static arrays and class mana-attribute groups are mapped. Quick Start fixture, remaining HP/SP math, trainer rules and starter state remain named F4 tasks; F5 checks creation, promotion and reload. |
| Items and spells | Original table bytes and 99 spell-delay triples are mapped. The executable contains a code-consumed spell-point cost that conflicts with one text row; F4 uses the documented provisional executable source with a disputed-case fixture, and F5 measures actual debit. Effects and item use remain explicit F4/F5 obligations. |
| Monsters and binary tables | Eleven binary payloads match; the pinned 548-byte actor schema and original-matched deltas show one baseline ID 173 reference. F4 retains all 173 descriptors and tests that lookup. AI, attacks, loot, frames and whether that actor appears remain F4/F5 questions. |
| EVT/STR and campaign | Original active resource bytes match retained copies; the inherited parser's 26-map rejection is a known implementation defect, not unexplained file drift. F4 must parse every active resource, classify extra pairs, implement typed variables/side effects and prove scenario routes. F5 checks quests and ending. |
| Houses, travel and finale | Static house/transport joins and original directory indexing are mapped. Executable-consumed weekday flags resolve the provisional source for one text conflict; blank/special slots, fares, quest gates and terminal behavior need F4 policy and F5 observation. |
| Services and screens | Covered table/font identities are confirmed; training, temple, NPC, paperdoll and screen consumers remain named implementation/visual tasks. F4 needs a per-screen/service fixture ledger; F5 inspects transactions and persistence. |
| NWC/Prima claims | The original data underlying F2 guide ledgers is byte-matched. Guide claims with conflicting or absent effect evidence remain claims, with a private F4 per-claim bridge to original-matched resources and F5 outcome checks. |
| Video, audio, portraits and voices | Original VID/SND framing and music streams pass static checks. All 636 vanilla portrait frames match retained PNG pixels; face order and portrait-frame table are identified. F4 must map expression and voice selectors, classify exceptions and decode/render media; F5 must see and hear representative output. |
| Whole `.ec6` state | All eleven logical state families have a private section/owner/bound/acceptance register. `.ec6` is new, so no original file can prove its wire schema. F3 accepts this **logical-family granularity**; exact per-field types, lengths, defaults, codecs and malformed-input tests are a blocking F4 save-codec lock, with F5 durable save/reload. Native `.mm6` import is outside this milestone. |

No checked family has an unexplained original-to-retained content discrepancy. The unresolved items above are **named behavior, source-consumer or new-format design questions** with specific F4/F5 routes. This satisfies the F3 exit condition in [plan §18.1](plan.md) while keeping those rows open in the implementation checklist. A later contradictory original or runtime observation reopens the relevant assumption; it does not license silent fallback to MM7 rules.

## F4 start decision

Offline MM6 work may proceed against the retained ignored reference set. Before any partial MM6 launch, require a native arm64 build, full active resource parsing, title-specific runtime paths, `.ec6` round-trip and malformed-save checks, and the offline scenario coverage in the [F4 build graph](MM6_F4_BUILD_GRAPH.md). MM7's source, installed helper and saves remain protected; the MM8 checkout remains reserved. No MM6 Atlas Play action is authorized by this gate.
