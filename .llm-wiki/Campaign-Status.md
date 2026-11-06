---
type: synthesis
up: "[[index_Enroth-Continuum]]"
tags: [mm6, mm7, mm8, status]
---

# Campaign Status

The campaigns are at different readiness levels as of the 2026-11-06 local status note; later runtime checks may supersede this page.

| Game | Observed state | Remaining evidence |
| --- | --- | --- |
| MM7 | Native arm64 Atlas launch, Emerald Island graphical view, save/reload, and private portrait and armor AC override checks passed. The separate MM7 baseline was also checked with an existing save. | Full campaign, quest, map, ending, and broader mod coverage remain to be audited. |
| MM6 | F2 source coverage is frozen and the [F3 original-file gate](../project/MM6_F3_GATE_REVIEW.md) passed for offline F4. An earlier partial native build rendered title and party-creation screens; it was not launched in F3. | Complete MM6-specific F4 runtime, then graphical F5 maps, rules, events, save/load, audio and ending. No MM6 Play action yet. |
| MM8 | Private data staged and detected by Atlas. | No adapted MM8 engine or Play action yet. |

The status above is an evidence summary, not a full compatibility claim. For exact counts, parser observations, and known gaps, read the current [local status](../project/LOCAL_STATUS.md) and title-specific implementation documents. The Atlas should expose each Play action only after that title's runnable session and save/load checks.

Source: [project/LOCAL_STATUS.md](../project/LOCAL_STATUS.md), [project/MM6_F3_GATE_REVIEW.md](../project/MM6_F3_GATE_REVIEW.md), [project/MM7_AUDIT_CHECKLIST.md](../project/MM7_AUDIT_CHECKLIST.md), reviewed 2026-11-06.

See also: [Runtime Isolation](Runtime-Isolation.md), [Project Overview](Project-Overview.md), [Local Status Source](Source-Local-Status.md), [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), [Agent Handoff](Agent-Handoff.md), [Open Questions](Open-Questions.md).
