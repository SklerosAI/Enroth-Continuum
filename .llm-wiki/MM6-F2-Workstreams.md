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
| A — party and rules | Class/skill IDs, creation and Quick Start, promotions, stats, spells, items, equipment, treasure and alchemy. Reconcile the [class/skill spec](../project/MM6_CLASS_SKILL_SPEC.md), shipped tables, guide locators and inherited rule consumers. | A gap matrix by subsystem, with source locator, consumer, semantic uncertainty, and an acceptance scenario for each rule family. Exact per-ID tables stay ignored. | World persistence needed by this lane is a question for B; quest-driven promotions go to C. |
| B — world and persistence | Indoor/outdoor base and delta layouts, transitions, chests/doors/map state, reset/respawn, MM6 save container and profile isolation. Reconcile the [save spec](../project/MM6_SAVE_SPEC.md) with source consumers. | A field/consumer/round-trip and save/reload obligation matrix, listing unknown meanings and failure cases. | Event variable effects go to C; rule effects on saved character state go to A. |
| C — events and campaign | EVT framing, opcodes and variable IDs, NPC dialogue, shops/house/travel links, quest flags, promotions as event chains, map/STR pairing, and ending reachability. Reconcile the [guide/campaign spec](../project/MM6_GUIDE_RECONCILIATION.md). | A campaign dependency and event-execution gap matrix, with every critical route and ending given an acceptance scenario or explicit unresolved link. | Character mechanics go to A; map serialization goes to B; media triggers go to D. |
| D — presentation and media (fourth agent when available) | MM6 UI and input screens, portraits/voices/icons/fonts, videos, sound effects, music/house animation references, Mac graphical and audio acceptance scenarios, and Atlas title identity. | A resource-to-consumer and visible/audible scenario matrix. Mark what can be checked offline now and what must wait for F5. | Do not install or change Atlas Play actions; hand engine/launcher integration requirements to the owner. |

With three agents, the integrator can retain lane D or assign its evidence-only media/UI subset after A–C are underway. Do not subdivide a lane across concurrent writers without a file-level ownership agreement. The integrator keeps [MM6 implementation checklist](../project/MM6_IMPLEMENTATION_CHECKLIST.md), [plan](../project/plan.md), and [local status](../project/LOCAL_STATUS.md) authoritative.

## Handoff packet from each lane

- Recorded checkout branch/SHA and source dates; checked `git status` before and after.
- What was inspected, what is confirmed, what is inferred, what remains unknown, and the exact evidence level (inventory, parse, semantic, runtime, or campaign).
- Tracked source locators and names of ignored local reports, without reproducing the reports, raw game strings, asset lists, guide text, saves, screenshots, or logs.
- A parser/consumer map and an acceptance scenario for every covered family; call out any uncovered family.
- Cross-lane questions routed to the named lane; proposed edits isolated to owned files or a separate branch, with focused validation and no mixed-purpose commit.

The integration owner compares all four packets before F3, checks contradictions and boundary coverage, then prepares the single consolidated original-file preflight specified in [plan §18.1](../project/plan.md). No agent can independently declare F2 or F3 complete from its own lane.

See also: [Agent Handoff](Agent-Handoff.md), [Open Questions](Open-Questions.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md), [Campaign Status](Campaign-Status.md).
