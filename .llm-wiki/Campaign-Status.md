---
type: synthesis
up: "[[index_Enroth-Continuum]]"
tags: [mm6, mm7, mm8, status]
---

# Campaign Status

The campaigns are at different readiness levels as of the 2026-11-07 local status note; later runtime checks may supersede this page.

| Game | Observed state | Remaining evidence |
| --- | --- | --- |
| MM7 | Native arm64 Atlas launch, Emerald Island graphical view, save/reload, and private portrait and armor AC override checks passed. The separate MM7 baseline was also checked with an existing save. A new staged helper and Atlas built both from a clean tracked-source extraction and a fresh public SklerosAI clone under network denial, and focused headless MM7/Russian-text checks passed. A separately rebuilt hidden OpenGL GameTest rendered one first-map world/HUD frame with no visible window or focus change. A local-only headless GameTest saved to an isolated disk profile and reloaded it through the normal Load menu in a fresh process; the staged candidate has not had visible save/reload acceptance. | Full campaign, quest, map, ending, and broader mod coverage remain to be audited. |
| MM6 | F2 coverage and the [F3 source-data gate](../project/MM6_F3_GATE_REVIEW.md) are complete. A disposable seven-process v5 first-map cycle passed headlessly after live capture extraction. All 15 outdoor source pairs decode; the second outdoor map loads and produces a separate v5 test candidate. Three distinct five-child disposable indoor v6 cycles now pass bounded real-loop turn/walk, post-input save and fresh-child replay; production gameplay admission remains closed. A detached border Party checkpoint and second-outdoor no-write candidate pass focused headless checks. A versioned outdoor v6 visited-map section round-trips two map capsules in memory; no v6 file or production checkpoint is written. An all-source indoor codec sweep passes; a risk-ranked additional case retains signed native actor identity and passes the third test-only chain. Follower dialogue semantics remain open. Active skill Compare/Set and a fail-closed manual/quick-save guard are integrated. | Production MM6 SaveLoad, general indoor gameplay and later-map replay, campaign behavior, graphical F5 and Atlas Play remain open. The disposable cycles are not a playable campaign. |
| MM8 | Private data staged and detected by Atlas. | No adapted MM8 engine or Play action yet. |

The status above is an evidence summary, not a full compatibility claim. For exact counts, parser observations, and known gaps, read the current [local status](../project/LOCAL_STATUS.md) and title-specific implementation documents. The Atlas should expose each Play action only after that title's runnable session and save/load checks.

Source: [project/LOCAL_STATUS.md](../project/LOCAL_STATUS.md), [project/MM6_F3_GATE_REVIEW.md](../project/MM6_F3_GATE_REVIEW.md), [project/MM7_AUDIT_CHECKLIST.md](../project/MM7_AUDIT_CHECKLIST.md), reviewed 2026-11-07.

See also: [Runtime Isolation](Runtime-Isolation.md), [MM7 Offscreen Frame Capture](MM7-Offscreen-Frame-Capture.md), [Project Overview](Project-Overview.md), [Local Status Source](Source-Local-Status.md), [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), [Agent Handoff](Agent-Handoff.md), [Open Questions](Open-Questions.md).
