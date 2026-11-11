# Current work board — 2026-11-10

The owner's [delivery order](../.llm-wiki/Current-Delivery-Order.md) is MM7
working, MM6 working, full MM7 audit, full MM6 audit. This board records
current lanes; older messages and handoffs remain historical evidence.
Claude Code is the planning and coordination lead. The stage 1 gate rows
(W1-W6), defect queue and limits are in [PLAN.md](PLAN.md).

| Lane | Owner | Scope | State | Next coordination point |
| --- | --- | --- | --- | --- |
| MM7 raid route (W2) | Codex subagent | Harmondale raid GameTest and shop reopening | Integrated on `master`; campaign GameTests 6/6 | `GameTests_MM7Campaign.cpp` released for W4 |
| MM7 candidate validation (W1, W3) | Codex subagent | Current staged build and isolated visible Save/Load | W1 passed; W3 partial; final staged Atlas rebuilt and signed | Owner authorized headless checks only; visible smoke, audible sound, ordinary NPC and monster fight remain open |
| MM7 integration and priority record | Codex root | Review lanes, integrate one at a time, maintain the four-stage delivery order | Active | Share reviewed outcomes through `agent_comms/` and update this board |
| Planning and coordination lead | Claude Code | Sequence the working gates, review and integrate lane output while Codex is paused | Active | `PLAN.md` |
| MM7 campaign path (W4) | Claude Code | Continuous chain C1-C7 on the integrated harness | C0, harness, Xenofex fight and quest items integrated at `56b5968ab`. C1-C3 lane active | C4-C7 after C1-C3 lands |
| MM7 castle opening (W4 O5–O7) | Codex subagent | First Butler visit, goblin completion, exit and hand-in | Integrated on `master`; Castle and raid controls 2/2 | Start the next independent W4 scope; Castle test file released |
| MM7 castle rebuild (W4 O8–O10) | Codex subagent | Butler repair, seven rescues, king hand-in and missing-rescue control | Interaction test integrated; three related controls 3/3 before checkpoint refactor | Rerun one-missing case on integrated checkpoint code when runtime authorization resumes |
| MM7 castle save checkpoint (W4) | Codex subagent | Positive rebuild Save and fresh-process Load in marked temporary profile | Integrated writer and fresh reader 1/1 each; one-missing case 1/1; old writer and reader 1/1 each; slot digest unchanged | Full campaign path and visible candidate remain open |
| MM7 scavenger path (W4 O2) | Codex subagent | Six Judge deliveries and ship offer versus one missing | Integrated on `master` at `2bdbde2fc`; focused path 1/1 and ship controls 2/2 | Real item acquisition and post-departure fresh-process Save/Load remain open |
| MM7 Light city/Walls path (W4 L1–L4) | Codex subagent | Input-driven Light choice, city/throne and Walls hand-in in new test file | Integrated on `master`; positive and dawn cases 2/2, with related controls 4/4 on integrated source | Continue to advisor tasks; successful no-kill route is not blocked by the group-1 check |
| MM7 Light chapter save checkpoint (W4) | Codex subagent | Production Save after Walls hand-in and normal Load in a fresh headless process | Integrated writer and reader passed 1/1 each; slot digest unchanged; route/dawn controls 2/2 | Later Light chapter checkpoint after Overthruster remains open |
| MM7 Light advisors (W4 L5) | Codex subagent | Altar pieces, wine cellar and soul jars dialogue hand-ins | Integrated on `master`; focused advisor checks 3/3 and combined Light controls 7/7 | Item acquisition and Xenofex remain open |
| MM7 Light assassination (W4 L6–L7) | Codex subagent | Robert's advisor gate, quest request, Pit door and cube hand-in | Integrated on `master`; focused cases 2/2 and combined Light controls 7/7 | Pit combat, cube acquisition and uninterrupted chapter path remain open |
| MM7 Light Xenofex (W4 L8) | Codex subagent | Quest gate, alive/dead map-leave and throne hand-in | Integrated on `master`; focused cases 2/2 and combined Light controls 9/9 | Travel and combat remain open; the kill is a fixture |
| MM7 Overthruster (W4 L10–L14) | Codex subagent | Wetsuit, Shoals/Lincoln, retrieval and return path | Integrated on `master`; four new cases and two controls pass 6/6, with Xenofex controls 8/8 | Natural L8-to-L9 link, retrieved-item L15 ending and fresh-process checkpoint remain open |
| MM7 Pit cube acquisition (W4 L7) | Codex subagent | Input-driven carrier fight, corpse loot and advisor hand-in | Integrated on `master`; new case plus no-cube control 2/2 | Party stats/position are fixtures; natural room walking and continuous chapter remain open |
| MM7 Light final-task link (W4 L9/L15) | Codex subagent | Natural topic unlock, no-item control and finale | Integrated on `master`; 3/3 with Xenofex/ending controls and 7/7 with Overthruster cases | Held Overthruster is a fixture; link retrieved item through fresh-process checkpoint |
| MM7 Overthruster fresh checkpoint (W4) | Codex subagent | Production Save after retrieval and normal Load in fresh headless process | Integrated on `master`; writer and reader 1/1 each, slot unchanged, focused controls 6/6 | Earlier chapter state is fixtured; continuous route and installed-app Save/Load remain open |
| MM7 W4 Walls seam persistence | Codex subagent | Post-Walls NPC house/topic state before Save and after fresh Load | Clean branch `codex/mm7-light-npc-persistence` ready; route/advisor controls 5/5, writer and reader 1/1 each; no save defect | Claude review requested before integration; correct stale wiki reset claim |
| MM7 W4 Tolberti-to-Xenofex seam | Codex subagent | Produce NPC80's house/topic through input, then Save/fresh Load and request Xenofex | Active in separate worktree, disjoint from Walls and Claude scroll files | Review exact marked-profile gate and fixture boundaries before integration |
| MM7 cancelled scroll defect (W6) | Claude Code subagent | Item-, actor- and character-target scrolls kept when targeting is cancelled | Integrated on `master` (`7b3b7b074`, `ece2203eb`, `374017cdf`) | Other targeting exits that still use the scroll go to the stage 3 queue |
| MM7 W4 path review | Claude Code lead | Audit chapter seams, test assertions and disposable-profile harness | Review committed at `a9af27ae6`; F1 is a possible player blocker, F2-F4 need stronger tests | Codex owns F1; queue F2-F4 after a code lane frees |
| MM7 Walls protected group (W4 L1–L4) | Codex subagent | Fresh-entry group-population and source semantics | Investigation complete; group-1 check is an unreachable negative branch in tested local data, with no source-backed runtime fix | Retain handoff; revisit only if another owned edition or original-game observation contradicts it |
| MM7 endgame audit recorder | Codex root + subagent | Atlas recording toggle, chosen local/external folder, passive video/log/save collector | Integrated on `master`; signed candidate installed headlessly after owner authorization; installed hashes and signature verified | First real recording, screen permission, external-drive removal and Atlas lifecycle require later graphical authorization |
| MM6 implementation | Unassigned | Existing first-map progress and Notes worktree retained | Queued after MM7 working gate | Resume from `project/MM6_RESUME_HANDOFF.md` after MM7 acceptance |

## Active lanes at Codex's return (2026-11-10 22:10 local)

| Checkout | Branch | Owner | Scope |
| --- | --- | --- | --- |
| `mm7-load-admission` | `claude/mm7-chain-c1-c3` | Claude | Chain C1-C3, follow-ups |
| `mm7-death-regression` | `claude/mm7-chain-c4-c7` | Claude | Chain C4-C7 to the Light ending |
| `mm7-rolling-replay` | `claude/mm7-recorder-caps` | Claude | Recorder storage caps, reinstall |
| `mm7-monster-combat` | `claude/mm6-new-game` | Claude | MM6 production New Game (M1) |
| `mm7-save-durability` | `claude/mm6-map-sweep` | Claude | MM6 map sweep probe |
| `mm7-mod-access` | `claude/mm6-reputation-var` | Claude | MM6 reputation variable |
| `openenroth-upstream` | `upstream-playtest` | Claude | Upstream build checkout |
| `mm7-combat-fixes`, `mm7-save-atomic` | free | Codex | See the welcome-back message |
