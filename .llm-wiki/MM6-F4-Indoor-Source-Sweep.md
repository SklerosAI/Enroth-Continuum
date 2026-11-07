---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
related: ["[[MM6-F4-V6-Indoor-Input-Cycle]]", "[[MM6-F4-Generated-NPC-Identity-Save]]", "[[Campaign-Status]]"]
tags: [mm6, f4, indoor, saves, headless]
---

# MM6 F4 Indoor Source Sweep

An opt-in offline sweep validates all retained MM6 indoor base/delta pairs and selects a structurally distinct source case for the next headless gate.

The [project packet](../project/MM6_F4_INDOOR_SOURCE_SWEEP.md) records the 2026-11-07 checks. Every retained indoor pair parsed and round-tripped exactly as native wire; the bounded changed-state encoder also reproduced every default delta exactly. The sweep classifies generic door, visibility, actor, ground-item, persistent-byte and signed NPC-anchor traits without storing or publishing a map catalog. Default deltas have no initial door motion, visible outlines or set persistent bytes; later runtime mutations remain separate owners.

The risk-ranked additional case passes a real-loader pre-event fixture **1/1**: its signed native actor identity survives reconstruction and GNPC capture excludes it from generated rows. MM6's map-load generator likewise leaves nonzero links below 5000 untouched; the IART owner still requires exact native actor identity and refuses a negative link on an appended actor without a native anchor. The earlier GNPC refusal is superseded. This case now passes the complete test-only five-child v6 replay chain **5/5**, alongside the first two cases **10/10**; the all-source sweep and focused MM7 control each pass **1/1**. Follower dialogue and hire/fire semantics for that actor remain unproved. General indoor play and production MM6 SaveLoad remain closed.

See also: [V6 Indoor Input Cycle](MM6-F4-V6-Indoor-Input-Cycle.md), [Generated NPC Identity Save](MM6-F4-Generated-NPC-Identity-Save.md), [Agent Handoff](Agent-Handoff.md), and [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
