# Current work board

Update this board only for active, cross-agent work. Link detailed evidence in
the matching claim or handoff. Keep this page concise.

| Lane | Owner | Scope | State | Next coordination point |
| --- | --- | --- | --- | --- |
| MM6 lead and integration | Claude | MM6 plan, file ownership, one-at-a-time lane integration, F4b Save/Load and play gates | Active | Integration order and gate in `messages/2026-11-07T22-23-28Z--from-claude-mm6-lead--to-codex--mm6-assignment.md` |
| MM6 lanes | Claude lanes | See `project/SESSION_HANDOFF_2026-11-07.md` | E, T, S, I, H merged. A and B still running. New lanes G (item generator), P (transition prerequisites) and M (shops) running. | Lanes merge as they complete. Merges are announced in `agent_comms/live/` |
| MM6 C1 transition dialog, C2 party death model | Codex | Assigned in `messages/2026-11-07T22-23-28Z--from-claude-mm6-lead--to-codex--mm6-assignment.md` | Merged as `21b0c04e8`, `363d5eee3` (C1) and `d251707cb` (C2) | Live Accept waits for the transition wiring lane. C2 `Game.cpp` dispatch after Lane B. |
| MM7 maintenance and MM7 combat audit fixes | Codex | Portraits, Atlas mod controls, staged helper, atomic save helper and validator, artifact fix, chest guard, and now the shared combat files for MM7-path audit fixes | Active | SaveLoad/UISaveLoad stay with MM6 Lane B until handed off |
