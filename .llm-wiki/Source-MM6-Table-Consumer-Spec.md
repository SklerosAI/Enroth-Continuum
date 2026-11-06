---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, tables, f2]
---

# MM6 Text Table Consumer Specification Source

This F2 source compares retained MM6 text-table names with literal `eventsData("name")` calls in the current C++ tree.

Of 30 retained table names, 24 have a direct literal load and six do not. This is a narrow static call-site inventory. A literal load does not prove correct MM6 parsing, row coverage or gameplay semantics; absence of a literal call does not prove a table unused. Indirect loads and original-game behavior still need review. Each table requires a verified schema, downstream title-specific consumers, and an acceptance scenario or evidenced unused/reserved classification before the F2 specification is complete.

A later retained-table review found a complete symmetric two-axis `useitems.txt` matrix in the MM6 herb/bottle band, but the result-token meanings and original interaction behavior are unknown. The current MM6 item loader has no path for that matrix, and the inherited potion UI still uses MM7 rules. A separate source trace found the current MM6 NPC initializer leaves a dismissal-text field unset while the dialogue UI reads it; the proper MM6 text source and actual displayed behavior remain unverified. Overlapping autonote variants and other no-literal-load tables also need consumer and provenance checks. These are implementation leads, not proved original-game defects.

The [potion-guide probe](../scripts/mm6_probe_potion_guide.py) matches 25 unordered numeric-result input pairs against the owned guide chart, with two result disagreements. The guide does not settle the five marker-token meanings, failure effects or original runtime behavior. A primary MMExtension reference also describes dismissal text as MM7-only, strengthening the need to trace MM6's intended flow rather than filling the inherited field by assumption.

The [30-family parser/consumer packet](../project/MM6_TABLE_SEMANTIC_F2.md) adds selected cross-table reference-presence checks. It also establishes a deterministic full-row awards mismatch: all 100 numeric MM6 award rows have an empty priority cell, and the shared loader skips rows with that cell empty. This does not establish how awards should be ordered or displayed, or whether any award route is reachable in the partial runtime.

Authority: [project/MM6_TABLE_CONSUMER_SPEC.md](../project/MM6_TABLE_CONSUMER_SPEC.md) and the reproducible [table-consumer probe](../scripts/mm6_probe_table_consumers.py), reviewed 2026-11-05. Detailed source-line maps, matrix cells and source fingerprints stay only in ignored local research reports; do not copy them to this wiki or public Git.

See also: [MM6 Table Semantic F2 Source](Source-MM6-Table-Semantic-F2.md), [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), [Open Questions](Open-Questions.md), [Agent Handoff](Agent-Handoff.md).
