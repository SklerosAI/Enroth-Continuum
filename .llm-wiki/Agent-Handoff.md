---
type: index
up: "[[Home_Enroth-Continuum]]"
tags: [handoff, agents, mm6]
---

# Agent Handoff

This page is the starting brief for future Codex agents working in parallel on Enroth Continuum; recheck its dated status against the authoritative files before acting.

## Read first

1. Run `git status --short --branch`, `git worktree list`, and `git log -1 --oneline` in the checkout you intend to use. The branch and worktree descriptions below were checked on 2026-11-05 and may drift.
2. Read [AGENTS.md](../AGENTS.md), [project/plan.md §18.1–18.2](../project/plan.md), [project/LOCAL_STATUS.md](../project/LOCAL_STATUS.md), [project/MM6_IMPLEMENTATION_CHECKLIST.md](../project/MM6_IMPLEMENTATION_CHECKLIST.md), [project/MM6_F2_GATE_REVIEW.md](../project/MM6_F2_GATE_REVIEW.md), and [project/MM7_AUDIT_CHECKLIST.md](../project/MM7_AUDIT_CHECKLIST.md).
3. Read [Engine Architecture and Runtime Map](Engine-Architecture-and-Runtime-Map.md), [MM6 Source and Probe Index](MM6-Source-and-Probe-Index.md), [MM6 F2 Workstreams](MM6-F2-Workstreams.md), [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), and [Open Questions](Open-Questions.md) before taking an assignment. Future observation/control work also reads the [Agent Play Knowledge and Control Contract](Agent-Play-Knowledge-and-Control-Contract.md). The ignored local companion at `local-game-data/private-wiki/INDEX.md` holds exact owner-specific evidence; it is not Git backed up. Use this wiki's [schema](SCHEMA_Enroth-Continuum.md) for durable public findings. The tracked source and fresh checks outrank this page.

## Current phase and gates (2026-11-05)

| Area | Evidence-backed state | Next gate |
| --- | --- | --- |
| MM7 personal baseline | An independent arm64 MM7 source/app and profile have launched graphically, loaded an existing save, and returned to Atlas. The full campaign audit is still **planned**. | M7-A0 protects and rechecks this baseline before the separate M7-A1–A5 audit. |
| MM6 F0 | Separate MM7 baseline exists and has been live checked. | Keep it frozen while MM6 research proceeds. |
| MM6 F1/F2 | Focused creation, world/save, event/campaign, media/UI, table, monster, spell, service and NPC audits exist. All 43 active opcode values have a source-backed side-effect contract; the guide ledger maps 14 chapter families, 99 spell base costs and baseline recovery categories were reviewed, 460 individual item entries were visually identified, 581 item and 173 monster IDs have private review statuses, 77 NPC professions have private joins, and 32 campaign headings have candidate joins. The [F2 gate review](../project/MM6_F2_GATE_REVIEW.md) remains **open and unfrozen**: original opcode effects, exact spell delays, guide discrepancies, detailed item/combat semantics and campaign branch behavior remain unresolved. These source/retained-data leads do not prove runtime rules, saves, combat, UI or quest reachability. | Resolve or explicitly label domain gaps, then record an F2 freeze decision before F3. |
| MM6 F3 | **Not performed.** | Run one deliberate read-only original-data preflight after F2 is broad enough; retain its detailed report locally and ignored. |
| MM6 F4–F6 | **Not gate-ready.** The current MM6 adapter renders title/party screens, but is not a playable engine. | After F3, complete offline implementation, then real graphical campaign/save checks, then a title-aware Atlas installation. |
| MM8 | Reserved separate checkout; no adapted engine or Play action. | Begin its own title-specific audit and implementation after the MM6 priority allows it. |

The [campaign status](Campaign-Status.md) summarizes observed play. A build, decoder, parser pass, or title screen never establishes campaign playability. Do not repeatedly launch the partial MM6 build to test isolated F2 assumptions; use the retained ignored references and focused offline checks. The plan permits targeted rereads after F3 when real evidence warrants them.

## Authority and source order

| Question | Start with | Supporting source |
| --- | --- | --- |
| Current gates and release of F4 | [plan §18.1](../project/plan.md), [MM6 checklist](../project/MM6_IMPLEMENTATION_CHECKLIST.md) | [guide reconciliation](../project/MM6_GUIDE_RECONCILIATION.md) |
| Guide provenance and claim coverage | [guide F2 review ledger](../project/MM6_GUIDE_F2_REVIEW.md) | Tracked [coverage probe](../scripts/mm6_probe_guide_coverage.py) and private page-level notes; chapter indexing is not full semantic review |
| Verified Mac behavior and bundle/profile state | [local status](../project/LOCAL_STATUS.md), then fresh local checks | [launcher README](../launcher/README.md) |
| MM6 class, skill, creation IDs, item actions and potion effects | [creation audit](../project/MM6_CREATION_F2.md), [creation constant leads](../project/MM6_CREATION_CONSTANTS_F2.md), [class/skill specification](../project/MM6_CLASS_SKILL_SPEC.md), [rules closure packet](../project/MM6_RULES_F2_CLOSURE.md), [item-effect seams](../project/MM6_ITEM_EFFECT_SEAMS_F2.md) | Tracked [creation](../scripts/mm6_probe_creation_rules.py), [item-effect](../scripts/mm6_probe_item_effect_seams.py) and [potion-guide](../scripts/mm6_probe_potion_guide.py) probes, retained private tables and guide page locators |
| MM6 spells, scrolls, books, recovery and targeting | [spell effect audit](../project/MM6_SPELL_EFFECT_F2.md), [spell guide packet](../project/MM6_SPELL_GUIDE_CLAIMS_F2.md), [recovery ledger](../project/MM6_SPELL_RECOVERY_F2.md) | Tracked [spell-effect probe](../scripts/mm6_probe_spell_effects.py) and ignored per-ID guide/recovery reports |
| MM6 map deltas and save contract | [save specification](../project/MM6_SAVE_SPEC.md), [world closure packet](../project/MM6_WORLD_F2_CLOSURE.md), [world field map](../project/MM6_WORLD_FIELD_MAP_F2.md) | Tracked [world roundtrip](../scripts/mm6_probe_world_roundtrip.py) and [field](../scripts/mm6_probe_world_fields.py) probes, private reports and source consumers |
| MM6 event, dialogue and campaign behavior | [event/campaign specification](../project/MM6_EVENT_CAMPAIGN_SPEC.md), [opcode packet](../project/MM6_OPCODE_SEMANTICS_F2.md), [campaign closure packet](../project/MM6_CAMPAIGN_F2_CLOSURE.md) | [guide reconciliation](../project/MM6_GUIDE_RECONCILIATION.md), tracked [opcode](../scripts/mm6_probe_opcode_semantics.py) and [route](../scripts/mm6_probe_campaign_routes.py) probes; private reports |
| MM6 media, houses and character interface | [media/UI specification](../project/MM6_MEDIA_UI_SPEC.md), [media closure packet](../project/MM6_MEDIA_F2_CLOSURE.md) | Tracked [house-route](../scripts/mm6_probe_house_routes.py) and [media-consumer](../scripts/mm6_probe_media_consumers.py) probes, retained private indexes |
| MM6 text-table loading, awards and downstream use | [table-consumer specification](../project/MM6_TABLE_CONSUMER_SPEC.md), [table semantic packet](../project/MM6_TABLE_SEMANTIC_F2.md) | Tracked [table-consumer](../scripts/mm6_probe_table_consumers.py) and [table-semantic](../scripts/mm6_probe_table_semantics.py) probes; ignored local reports |
| MM6 monsters and combat | [monster/combat specification](../project/MM6_MONSTER_COMBAT_SPEC.md), [combat semantics packet](../project/MM6_COMBAT_SEMANTICS_F2.md), [effect seams](../project/MM6_MONSTER_EFFECT_SEAMS_F2.md), [spawn-link packet](../project/MM6_MONSTER_SPAWN_LINKS_F2.md) | Tracked [monster labels](../scripts/mm6_probe_monster_rules.py), [combat semantics](../scripts/mm6_probe_combat_semantics.py), [effect seams](../scripts/mm6_probe_monster_effect_seams.py) and [spawn links](../scripts/mm6_probe_monster_spawn_links.py) probes; ignored per-row/map reports |
| MM6 structural campaign links | [event/campaign specification](../project/MM6_EVENT_CAMPAIGN_SPEC.md) | Tracked [graph probe](../scripts/mm6_probe_quest_graph.py); its detailed graph stays ignored |
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
5. Before any F3 original-data preflight, close the [F2 gate review](../project/MM6_F2_GATE_REVIEW.md) with an explicit freeze decision and reconcile the draft [F3 preflight matrix](../project/MM6_F3_PREFLIGHT_MATRIX.md); neither document currently authorizes F3. With a clean tracked tree, run the [F2 freeze manifest helper](../scripts/mm6_f2_freeze_manifest.py) to store source/report hashes privately. After F3, record the reconciliation result locally, reconcile the [F4 offline build graph](../project/MM6_F4_BUILD_GRAPH.md), update only public high-level conclusions, and explicitly decide whether F4 can begin. F4, graphical F5, and Atlas F6 have their own gates; do not merge a partially working runtime into the protected MM7 baseline.

## Evidence and reporting standard

For each finding report: **source and locator** (tracked file plus line/function, public guide page, or private report key); **method and source SHA/date**; **observed fact**; **current engine consumer**; **confidence/uncertainty**; **required change or acceptance check**; and **privacy classification**. Keep detailed per-ID tables, decoded data, hashes, saved-game bytes, screenshots containing original game content, and command logs in ignored local storage. Public notes can state aggregate coverage and semantic gaps without reproducing proprietary material. Never copy private GOG assets or guide passages into an agent message, public Git, GitHub, or this wiki.

Label evidence precisely: **inventory** means found/indexed; **parse** means bounded decoding; **semantic** means field/rule meaning checked against evidence; **offline implementation** means title-specific effects executed in focused tests; **runtime** means seen/heard in the installed engine; **campaign** means progression and durable save/reload observed. Advance a checklist item only at the evidence level its wording requires. The [future agent-play contract](Agent-Play-Knowledge-and-Control-Contract.md) requires the same distinction for each rule a controller may use. For live play, inspect the graphical view and controls, record the checkpoint and save/reload result, and keep any raw visual artifact private. For audio, distinguish device detection from actual playback.

## Immediate tasks

Continue the [F2 workstreams](MM6-F2-Workstreams.md) from the [structural findings](MM6-F2-Structural-Findings.md): reconcile all active opcode side effects and MM6 variable dispatch; move beyond the [guide ledger](../project/MM6_GUIDE_F2_REVIEW.md)'s chapter locators to claim-level checks; resolve the [combat packet](../project/MM6_COMBAT_SEMANTICS_F2.md)'s identity, resistance, AI, projectile and loot questions; classify NPC/transport/finale routes; settle spell, award, potion, creation, portrait/voice and nonzero world-state semantics. Do not reinstate the old 14×10 delta-grid assertion. Probes establish static leads only. Bring unresolved findings into [Open Questions](Open-Questions.md) and the checklist. Protect MM7. Do not declare F2 complete, perform F3, begin F4, enable MM6/MM8 Play, or expose transfer merely because the title screen or offline probes work.

See also: [Engine Architecture and Runtime Map](Engine-Architecture-and-Runtime-Map.md), [MM6 Source and Probe Index](MM6-Source-and-Probe-Index.md), [Private Companion Wiki](Private-Companion-Wiki.md), [Agent Play and Observation Roadmap](Agent-Play-and-Observation-Roadmap.md), [Agent Play Knowledge and Control Contract](Agent-Play-Knowledge-and-Control-Contract.md), [MM6 F2 Workstreams](MM6-F2-Workstreams.md), [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), [Open Questions](Open-Questions.md), [Campaign Status](Campaign-Status.md), [Runtime Isolation](Runtime-Isolation.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
