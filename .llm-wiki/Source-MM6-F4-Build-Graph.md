---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, implementation, planning]
---

# MM6 F4 Build Graph Source

The draft [F4 build graph](../project/MM6_F4_BUILD_GRAPH.md) orders offline engine work by dependency: immutable title boundary, resources/tables, MM6 IDs and model, world state, events, rules/combat, interface/services/media, `.ec6` persistence and integrated campaign readiness. Each seam names an offline acceptance before graphical F5 play. It is a planning artifact; F4 has not started and the graph must be reconciled with the F3 original-file report.

The main integration rule is to keep MM6-specific IDs, assets, saves and profiles behind title boundaries while preserving the frozen MM7 baseline. Parser success, a native build and a title screen are not campaign playability claims. The first integrated MM6 launch should follow offline coverage of all active records and title-isolation checks.

Authority: [project/MM6_F4_BUILD_GRAPH.md](../project/MM6_F4_BUILD_GRAPH.md), drafted 2026-11-05. See also: [Agent Handoff](Agent-Handoff.md), [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), [Campaign Status](Campaign-Status.md).
