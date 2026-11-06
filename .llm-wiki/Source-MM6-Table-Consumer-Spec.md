---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, tables, f2]
---

# MM6 Text Table Consumer Specification Source

This F2 source compares retained MM6 text-table names with literal `eventsData("name")` calls in the current C++ tree.

Of 30 retained table names, 24 have a direct literal load and six do not. This is a narrow static call-site inventory. A literal load does not prove correct MM6 parsing, row coverage or gameplay semantics; absence of a literal call does not prove a table unused. Indirect loads and original-game behavior still need review. Each table requires a verified schema, downstream title-specific consumers, and an acceptance scenario or evidenced unused/reserved classification before the F2 specification is complete.

Authority: [project/MM6_TABLE_CONSUMER_SPEC.md](../project/MM6_TABLE_CONSUMER_SPEC.md) and the reproducible [table-consumer probe](../scripts/mm6_probe_table_consumers.py), reviewed 2026-11-05. The detailed source-line map and source fingerprints are written only to the ignored local research report; do not copy them to this wiki or public Git.

See also: [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), [Open Questions](Open-Questions.md), [Agent Handoff](Agent-Handoff.md).
