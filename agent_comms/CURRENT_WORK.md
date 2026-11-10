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
| MM7 campaign path (W4) | Claude Code | Main-quest transition coverage map, then input-driven campaign-path GameTests | Read-only mapping complete; code lane available | May take `GameTests_MM7Campaign.cpp` now that W2 is integrated |
| MM7 castle opening (W4 O5–O7) | Codex subagent | First Butler visit, goblin completion, exit and hand-in | Integrated on `master`; Castle and raid controls 2/2 | Start the next independent W4 scope; Castle test file released |
| MM7 castle rebuild (W4 O8–O10) | Codex subagent | Butler repair, seven rescues, king hand-in and missing-rescue control | Interaction test integrated; three related controls 3/3 before checkpoint refactor | Rerun one-missing case on integrated checkpoint code when runtime authorization resumes |
| MM7 castle save checkpoint (W4) | Codex subagent | Positive rebuild Save and fresh-process Load in marked temporary profile | Integrated writer and fresh reader 1/1 each; one-missing case 1/1; old writer and reader 1/1 each; slot digest unchanged | Full campaign path and visible candidate remain open |
| MM7 scavenger path (W4 O2) | Codex subagent | Six Judge deliveries and ship offer versus one missing | Integrated on `master` at `2bdbde2fc`; focused path 1/1 and ship controls 2/2 | Real item acquisition and post-departure fresh-process Save/Load remain open |
| MM6 implementation | Unassigned | Existing first-map progress and Notes worktree retained | Queued after MM7 working gate | Resume from `project/MM6_RESUME_HANDOFF.md` after MM7 acceptance |
