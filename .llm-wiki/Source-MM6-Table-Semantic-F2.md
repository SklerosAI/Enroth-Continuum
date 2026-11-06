---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, tables, awards]
---

# MM6 Table Semantic F2 Source

The [table semantic packet](../project/MM6_TABLE_SEMANTIC_F2.md) maps all 30 retained text-table families to their immediate current parser and a downstream consumer or an explicit missing path. Its tracked [probe](../scripts/mm6_probe_table_semantics.py) checks row shapes and selected cross-table references. The checked NPC house, profession and topic references, house exits and loaded message-scroll range resolve to retained rows. Reference presence does not establish column meaning, active use, event dispatch or correct UI behavior.

The later [six-table role packet](../project/MM6_UNLOADED_TABLE_ROLES_F2.md) source-classifies every file without a literal C++ load. Pinned MMExtension identifies `npcbtb.txt` as the MM6 13-personality beg/bribe/threat matrix and `proftext.txt` as seven weekday profession-news topic/text pairs; `useitems.txt` is a 29×29 combination matrix. The original consumer/provenance of `passwords.txt`, plural `autonotes.txt` and `errorlog.txt` remains explicit F3 work. Profession news must not be treated as MM7 dismissal text. Exact rows and disagreements stay in the ignored report.

All 100 numeric MM6 `awards.txt` rows have an empty priority cell. The F2 inherited loader dropped them; the [F4 award loader](MM6-F4-Awards.md) now keeps their text under checked IDs with neutral storage priority and guards unresolved formatting. Original ordering and live award effects remain unverified.

Authority: [project/MM6_TABLE_SEMANTIC_F2.md](../project/MM6_TABLE_SEMANTIC_F2.md), read with the [six-table role packet](../project/MM6_UNLOADED_TABLE_ROLES_F2.md), [table-consumer specification](../project/MM6_TABLE_CONSUMER_SPEC.md) and [implementation checklist](../project/MM6_IMPLEMENTATION_CHECKLIST.md). Detailed rows, IDs, source hashes and exceptions stay in ignored local reports. F2 is frozen for F3; original use and runtime effects remain open.

See also: [MM6 Text Table Consumer Specification Source](Source-MM6-Table-Consumer-Spec.md), [MM6 Source and Probe Index](MM6-Source-and-Probe-Index.md), [Open Questions](Open-Questions.md).
