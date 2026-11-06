---
type: index
up: "[[Agent-Handoff]]"
tags: [mm6, research, parallel]
---

# MM6 F2 Workstreams

These are separable research assignments for three or four parallel agents during MM6 F2; the integration owner alone reconciles them into the shared checklist.

All lanes work from the retained, ignored local reference set and public source documents. Each lane traces shipped evidence to current C++ consumers and testable behavior without editing the protected MM7 checkout or publishing private data. Existing checked inventory items do not imply semantic or gameplay completion. Start with [Agent Handoff](Agent-Handoff.md) for branch and reporting rules.

| Lane | Exclusive primary scope | Public deliverable to integration owner | Cross-lane boundary |
| --- | --- | --- | --- |
| A — party, items and combat rules | Class/skill IDs, creation and Quick Start, promotions, stats, spells, items, equipment, treasure, alchemy and monster combat. Reconcile the [creation audit](../project/MM6_CREATION_F2.md), rules closure, spell audit and table semantic packet: settle starting arrays, distinct Quick Start, starter-item collisions, potion markers, spell effects/ranges and skipped awards. | A gap matrix by subsystem, with source locator, consumer, semantic uncertainty, and an acceptance scenario for each rule family. Exact per-ID tables stay ignored. | World persistence needed by this lane is a question for B; quest-driven promotions and award use go to C. |
| B — world and persistence | Indoor/outdoor base and delta layouts, transitions, chests/doors/map state, reset/respawn, MM6 save container and profile isolation. Reconcile the [save spec](../project/MM6_SAVE_SPEC.md), [world closure packet](../project/MM6_WORLD_F2_CLOSURE.md) and [world field map](../project/MM6_WORLD_FIELD_MAP_F2.md): offline wire sections round trip, while nonzero meanings, chest grid/tail split and C++ reconstruction remain open. | A field/consumer/round-trip and save/reload obligation matrix, listing unknown meanings and failure cases. | Event variable effects go to C; rule effects on saved character state go to A. |
| C — events and campaign | EVT framing, opcodes and variable IDs, NPC dialogue, shops/house/travel links, quest flags, promotions as event chains, map/STR pairing, and ending reachability. Reconcile the [opcode packet](../project/MM6_OPCODE_SEMANTICS_F2.md) and campaign closure with the event spec and guide: register is structurally complete, while seven high-conflict operations, variable effects, Question/topics, transport and finale still need semantics. Graph and route probes are link indexes, not route proofs. | A campaign dependency and event-execution gap matrix, with every critical route and ending given an acceptance scenario or explicit unresolved link. | Character mechanics go to A; map serialization goes to B; media triggers go to D. |
| D — presentation and media (fourth agent when available) | MM6 UI and input screens, portraits/voices/icons/fonts, videos, sound effects, music/house animation references, Mac graphical and audio acceptance scenarios, and Atlas title identity. Reconcile the [media closure packet](../project/MM6_MEDIA_F2_CLOSURE.md) and media/UI spec, including inherited portrait name gaps, possible voice-ID gaps, movie suffix, house descriptor and type fallthrough; classify active house services with C. | A resource-to-consumer and visible/audible scenario matrix. Mark what can be checked offline now and what must wait for F5. | Do not install or change Atlas Play actions; hand engine/launcher integration requirements to the owner. |

This Codex session allows four active agents total, including the integrator. That means at most three worker agents can run A–C concurrently; the integrator can retain D or rotate a worker onto D when a slot frees. If future capacity changes, assign D to a fourth worker with its own file ownership. Do not subdivide a lane across concurrent writers without a file-level ownership agreement. The integrator keeps [MM6 implementation checklist](../project/MM6_IMPLEMENTATION_CHECKLIST.md), [plan](../project/plan.md), and [local status](../project/LOCAL_STATUS.md) authoritative.

## Handoff packet from each lane

- Recorded checkout branch/SHA and source dates; checked `git status` before and after.
- What was inspected, what is confirmed, what is inferred, what remains unknown, and the exact evidence level (inventory, parse, semantic, runtime, or campaign).
- Tracked source locators and names of ignored local reports, without reproducing the reports, raw game strings, asset lists, guide text, saves, screenshots, or logs.
- A parser/consumer map and an acceptance scenario for every covered family; call out any uncovered family.
- Cross-lane questions routed to the named lane; proposed edits isolated to owned files or a separate branch, with focused validation and no mixed-purpose commit.

The integration owner compares all four packets before F3, checks contradictions and boundary coverage, and explicitly closes the [F2 gate review](../project/MM6_F2_GATE_REVIEW.md) before preparing the single consolidated original-file preflight specified in [plan §18.1](../project/plan.md). The current review is open; no agent can independently declare F2 or F3 complete from its own lane.

See also: [Agent Handoff](Agent-Handoff.md), [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), [Open Questions](Open-Questions.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md), [Campaign Status](Campaign-Status.md).
