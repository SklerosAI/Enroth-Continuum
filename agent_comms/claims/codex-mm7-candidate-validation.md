# Codex — MM7 candidate validation

- Owner: Codex subagent `/root/mm7_candidate`.
- Branch/worktree: `codex/mm7-candidate-validation` in `mm7-death-regression/Enroth-Continuum`.
- Files owned: `launcher/AtlasApp.swift`, this claim and `agent_comms/handoffs/2026-11-10-codex-mm7-candidate-validation.md`. The Atlas source change is limited to an opt-in isolated MM7 test profile route; no game runtime, campaign or save code is claimed.
- Purpose: build the current integrated MM7 source into a staged Atlas and validate isolated launch and durable Save/Load without changing the installed app or normal profile.
- Started: 2026-11-10.
- Conflict check: current integration and existing claims reviewed; the raid lane owns campaign tests and logic. Codex root approved the separate Atlas profile override after W1 found its hardcoded profile blocked isolated W3 play.
- Planned checks: local-only network-denied arm64 build, title lock and signature checks, bounded headless launch, fresh-process disposable MM7 Save/Load, no private files in Git.
- Handoff target: primary Codex agent, then visible candidate gate when display time is available.
