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
| Planning and coordination lead | Claude Code | Sequence the MM7 working gate and later MM6/audit work, review lane output before integration | Active, coordination files only | Requests to Codex in `messages/2026-11-10T14-56-39Z--from-claude--to-codex--mm7-first-lead-plan.md` |
| MM7 campaign path (W4) | Claude Code planning lead | Main-quest transition coverage map and input-driven campaign-path GameTests | Read-only mapping complete; independent Codex Light scope active | Review the next Light-path gate and coordinate later W4 scopes |
| MM7 castle opening (W4 O5–O7) | Codex subagent | First Butler visit, goblin completion, exit and hand-in | Integrated on `master`; Castle and raid controls 2/2 | Start the next independent W4 scope; Castle test file released |
| MM7 castle rebuild (W4 O8–O10) | Codex subagent | Butler repair, seven rescues, king hand-in and missing-rescue control | Interaction test integrated; three related controls 3/3 before checkpoint refactor | Rerun one-missing case on integrated checkpoint code when runtime authorization resumes |
| MM7 castle save checkpoint (W4) | Codex subagent | Positive rebuild Save and fresh-process Load in marked temporary profile | Integrated writer and fresh reader 1/1 each; one-missing case 1/1; old writer and reader 1/1 each; slot digest unchanged | Full campaign path and visible candidate remain open |
| MM7 scavenger path (W4 O2) | Codex subagent | Six Judge deliveries and ship offer versus one missing | Integrated on `master` at `2bdbde2fc`; focused path 1/1 and ship controls 2/2 | Real item acquisition and post-departure fresh-process Save/Load remain open |
| MM7 Light city/Walls path (W4 L1–L4) | Codex subagent | Input-driven Light choice, city/throne and Walls hand-in in new test file | Integrated on `master`; positive and dawn cases 2/2, with related controls 4/4 on integrated source | Fresh-process chapter checkpoint in progress; successful no-kill route is not blocked by the group-1 check |
| MM7 Walls protected group (W4 L1–L4) | Codex subagent | Fresh-entry group-population gate and source-correct negative branch | Corrected interpretation: group-1 kill is a failure branch; fresh entry and local base/delta have no group-1 actor/spawn, so negative branch may be unreachable | Finish narrow original-semantics audit; do not claim a player-facing blocker or fabricate a qualifying actor |
| MM7 endgame audit recorder | Codex root + subagent | Atlas recording toggle, chosen local/external folder, passive video/log/save collector | Integrated on `master`; hardened after independent review; separate staged Atlas signed and headlessly checked; synthetic script gates passed | First real recording, screen permission, external-drive removal and Atlas lifecycle require later graphical authorization; installed Atlas unchanged |
| MM6 implementation | Unassigned | Existing first-map progress and Notes worktree retained | Queued after MM7 working gate | Resume from `project/MM6_RESUME_HANDOFF.md` after MM7 acceptance |
