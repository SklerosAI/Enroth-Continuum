# Current work board — 2026-11-10

The owner's [delivery order](../.llm-wiki/Current-Delivery-Order.md) is MM7
working, MM6 working, full MM7 audit, full MM6 audit. This board records
current lanes; older messages and handoffs remain historical evidence.
Claude Code is the planning and coordination lead. The stage 1 gate rows
(W1-W6), defect queue and limits are in [PLAN.md](PLAN.md).

| Lane | Owner | Scope | State | Next coordination point |
| --- | --- | --- | --- | --- |
| MM7 raid route (W2) | Codex subagent | Disabled raider-kill GameTest and quest completion behavior | Active in a separate worktree | Report branch/commit, focused checks and any shared-file conflict before integration |
| MM7 candidate validation (W1, then W3 and W5) | Codex subagent | Isolated current build, Save/quit/relaunch/Load gate and safe Atlas cutover preparation | Active in a separate worktree | Preserve installed baseline and normal profile; coordinate before visible focus or shared SaveLoad edits |
| MM7 integration and priority record | Codex root | Review lanes, integrate one at a time, maintain the four-stage delivery order | Active | Share reviewed outcomes through `agent_comms/` and update this board |
| Planning and coordination lead | Claude Code | Sequence the MM7 working gate and later MM6/audit work, review lane output before integration | Active, coordination files only | Requests to Codex in `messages/2026-11-10T14-56-39Z--from-claude--to-codex--mm7-first-lead-plan.md` |
| MM7 campaign path (W4) | Claude Code | Main-quest transition coverage map, then input-driven campaign-path GameTests | Read-only mapping now, code lane after lane A frees | Takes `GameTests_MM7Campaign.cpp` only after lane A's handoff releases it |
| MM6 implementation | Unassigned | Existing first-map progress and Notes worktree retained | Queued after MM7 working gate | Resume from `project/MM6_RESUME_HANDOFF.md` after MM7 acceptance |
