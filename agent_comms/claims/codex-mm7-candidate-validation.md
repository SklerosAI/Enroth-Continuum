# Codex — MM7 candidate validation

- Owner: Codex subagent `/root/mm7_candidate`.
- Branch/worktree: `codex/mm7-candidate-validation` in `mm7-death-regression/Enroth-Continuum`.
- Files owned: this claim and `agent_comms/handoffs/2026-11-10-codex-mm7-candidate-validation.md`. No runtime source files are claimed; coordinate before a concrete fix.
- Purpose: build the current integrated MM7 source into a staged Atlas and validate isolated launch and durable Save/Load without changing the installed app or normal profile.
- Started: 2026-11-10.
- Conflict check: current integration and existing claims reviewed; the raid lane owns campaign tests and logic, while this lane owns only staging and isolated runtime checks.
- Planned checks: local-only network-denied arm64 build, title lock and signature checks, bounded headless launch, fresh-process disposable MM7 Save/Load, no private files in Git.
- Handoff target: primary Codex agent, then visible candidate gate when display time is available.
