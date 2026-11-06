---
type: index
up: "[[Home_Enroth-Continuum]]"
tags: [handoff, agents, mm6]
---

# Agent Handoff

This page is the starting brief for future Codex agents working in parallel on Enroth Continuum; recheck its dated status against the authoritative files before acting.

## Read first

1. Run `git status --short --branch`, `git worktree list`, and `git log -1 --oneline` in the checkout you intend to use. The branch and worktree descriptions below were checked on 2026-11-05 and may drift.
2. Read [AGENTS.md](../AGENTS.md), [project/plan.md §18.1–18.2](../project/plan.md), [project/LOCAL_STATUS.md](../project/LOCAL_STATUS.md), [project/MM6_IMPLEMENTATION_CHECKLIST.md](../project/MM6_IMPLEMENTATION_CHECKLIST.md), and [project/MM7_AUDIT_CHECKLIST.md](../project/MM7_AUDIT_CHECKLIST.md).
3. Read [MM6 F2 Workstreams](MM6-F2-Workstreams.md), [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), and [Open Questions](Open-Questions.md) before taking an assignment. Use this wiki's [schema](SCHEMA_Enroth-Continuum.md) for durable findings. The tracked source and fresh checks outrank this page.

## Current phase and gates (2026-11-05)

| Area | Evidence-backed state | Next gate |
| --- | --- | --- |
| MM7 personal baseline | An independent arm64 MM7 source/app and profile have launched graphically, loaded an existing save, and returned to Atlas. The full campaign audit is still **planned**. | M7-A0 protects and rechecks this baseline before the separate M7-A1–A5 audit. |
| MM6 F0 | Separate MM7 baseline exists and has been live checked. | Keep it frozen while MM6 research proceeds. |
| MM6 F1/F2 | Private retained references, broad offline format inventories, guide indexing, and focused class/skill, save/delta, event/campaign, media/UI and text-table consumer specifications exist. A static scan finds direct literal loads for 24 of 30 retained text-table names; six have no literal load. This does not prove correct parsing or non-use. Detailed mechanic-to-data-to-engine reconciliation remains open. | Finish F2 coverage and resolve or explicitly label every uncertainty. |
| MM6 F3 | **Not performed.** | Run one deliberate read-only original-data preflight after F2 is broad enough; retain its detailed report locally and ignored. |
| MM6 F4–F6 | **Not gate-ready.** The current MM6 adapter renders title/party screens, but is not a playable engine. | After F3, complete offline implementation, then real graphical campaign/save checks, then a title-aware Atlas installation. |
| MM8 | Reserved separate checkout; no adapted engine or Play action. | Begin its own title-specific audit and implementation after the MM6 priority allows it. |

The [campaign status](Campaign-Status.md) summarizes observed play. A build, decoder, parser pass, or title screen never establishes campaign playability. Do not repeatedly launch the partial MM6 build to test isolated F2 assumptions; use the retained ignored references and focused offline checks. The plan permits targeted rereads after F3 when real evidence warrants them.

## Authority and source order

| Question | Start with | Supporting source |
| --- | --- | --- |
| Current gates and release of F4 | [plan §18.1](../project/plan.md), [MM6 checklist](../project/MM6_IMPLEMENTATION_CHECKLIST.md) | [guide reconciliation](../project/MM6_GUIDE_RECONCILIATION.md) |
| Verified Mac behavior and bundle/profile state | [local status](../project/LOCAL_STATUS.md), then fresh local checks | [launcher README](../launcher/README.md) |
| MM6 class, skill, creation IDs | [class/skill specification](../project/MM6_CLASS_SKILL_SPEC.md) | Retained private tables and guide page locators, never copied here |
| MM6 map deltas and save contract | [save specification](../project/MM6_SAVE_SPEC.md) | Retained private map probes and source consumers |
| MM6 event, dialogue and campaign behavior | [event/campaign specification](../project/MM6_EVENT_CAMPAIGN_SPEC.md) | [guide reconciliation](../project/MM6_GUIDE_RECONCILIATION.md) and retained private event probes |
| MM6 media and character interface | [media/UI specification](../project/MM6_MEDIA_UI_SPEC.md) | Retained private media indexes and [class/skill specification](../project/MM6_CLASS_SKILL_SPEC.md) |
| MM6 text-table loading and downstream use | [table-consumer specification](../project/MM6_TABLE_CONSUMER_SPEC.md) | Tracked aggregate-only [probe](../scripts/mm6_probe_table_consumers.py) and ignored local report |
| MM7 audit scope | [MM7 audit checklist](../project/MM7_AUDIT_CHECKLIST.md), [plan §18.2](../project/plan.md) | Frozen baseline and fresh local manifest |
| Attribution and legal boundary | [CREDITS.md](../CREDITS.md), [project/UPSTREAM.md](../project/UPSTREAM.md), [LICENSE](../LICENSE) | [source/privacy boundary](Source-and-Privacy-Boundary.md) |

The owner's shipped GOG data settles exact shipped content and IDs, but stays private. Guides explain intended mechanics; guide claims must be reconciled with the shipped data and actual engine behavior. Source code shows what the engine currently does, not that the game is correct. Document a disagreement with its source, date, and confidence instead of silently choosing one.

## Checkout ownership and isolation

| Role | Branch checked on 2026-11-05 | Agent rule |
| --- | --- | --- |
| Active MM6 research/integration | `mm6-mac-implementation` in the main Enroth checkout | Integration owner controls shared plan, checklist, status, wiki log, and branch commits. Coordinate before touching those files. |
| Protected MM7 baseline | `mm7-isolated-runtime` in the `mm7-frozen` worktree | Read-only reference. Do not implement or rebase here. A future audit uses a new branch/worktree from a known baseline. |
| Reserved MM8 baseline | `mm8-isolated-runtime` in the `mm8-isolated` worktree | Do not treat it as an MM8 implementation. Keep it separate from MM6 work. |

Use `git worktree list` for actual checkout paths. Each parallel agent should own a distinct scope and, for code changes, its own branch/worktree; do not let two agents write one working tree or one checklist simultaneously. The active MM6 integrator selects reviewed commits or findings for the main branch. Never build/install an MM6 helper into the MM7 slot, mutate another title's profile, or run the single-title `launcher/build.sh` as a trilogy installer. The intended Atlas has separate `MM6.app`, `MM7.app`, and `MM8.app` helpers, with title-specific data and profiles; that assembler is still outstanding.

## Parallel integration protocol

1. Assign one owner for the MM6 checklist and F3 readiness decision. Give other agents the distinct [F2 workstreams](MM6-F2-Workstreams.md); agree on file ownership and output names before editing.
2. Each agent starts from a recorded SHA, reads current authoritative docs, performs read-only inspection and focused offline checks, and keeps proprietary evidence under ignored local storage. Do not make an unchecked `[x]` entry.
3. Each agent sends the integrator a compact evidence report using the standard below. Code/spec changes, if any, are isolated in an agent-owned branch with named commits and no unrelated files. Do not push private findings or raw reports.
4. The integrator compares overlapping IDs and assumptions across workstreams, resolves disagreements against sources, updates the public checklist/specs, and checks that every F2 category has a parser/consumer and an acceptance scenario or an explicit gap.
5. Before any F3 original-data preflight, freeze a reviewable F2 checklist. After F3, record the reconciliation result locally, update only public high-level conclusions, and explicitly decide whether F4 can begin. F4, graphical F5, and Atlas F6 have their own gates; do not merge a partially working runtime into the protected MM7 baseline.

## Evidence and reporting standard

For each finding report: **source and locator** (tracked file plus line/function, public guide page, or private report key); **method and source SHA/date**; **observed fact**; **current engine consumer**; **confidence/uncertainty**; **required change or acceptance check**; and **privacy classification**. Keep detailed per-ID tables, decoded data, hashes, saved-game bytes, screenshots containing original game content, and command logs in ignored local storage. Public notes can state aggregate coverage and semantic gaps without reproducing proprietary material. Never copy private GOG assets or guide passages into an agent message, public Git, GitHub, or this wiki.

Label evidence precisely: **inventory** means found/indexed; **parse** means bounded decoding; **semantic** means field/rule meaning checked; **runtime** means observed in the engine; **campaign** means progression and durable save/reload observed. Advance a checklist item only at the evidence level its wording requires. For live play, inspect the graphical view and controls, record the checkpoint and save/reload result, and keep any raw visual artifact private. For audio, distinguish device detection from actual playback.

## Immediate tasks

Continue the [F2 workstreams](MM6-F2-Workstreams.md) from the [structural findings](MM6-F2-Structural-Findings.md): classify Question and NPC-topic behavior, reconcile potion creation/UI rules, establish nonzero map-state/tail semantics and lossless saves, resolve media routing, and verify parsing and downstream use for every text-table family. Investigate the six names without a literal load for indirect consumers or evidenced non-use. Bring unresolved findings into [Open Questions](Open-Questions.md) and the authoritative checklist. Protect the MM7 baseline; the independent MM7 audit can start later in a separate branch without blocking MM6 research. Do not declare F2 complete, perform F3, begin F4, enable MM6/MM8 Play, or expose character transfer merely because the title screen or offline probes work.

See also: [MM6 F2 Workstreams](MM6-F2-Workstreams.md), [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), [Open Questions](Open-Questions.md), [Campaign Status](Campaign-Status.md), [Runtime Isolation](Runtime-Isolation.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
