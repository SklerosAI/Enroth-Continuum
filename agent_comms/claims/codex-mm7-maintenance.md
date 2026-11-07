# Codex — MM7 maintenance

- Owner: Codex
- Branch/worktree: `mm6-mac-implementation` main integration checkout
- Files owned: MM7-only maintenance files unless an active MM6 claim owns a
  shared file; current committed change was `src/Media/MediaPlayer.cpp` and
  its focused test support.
- Purpose: repair isolated MM7 regressions, verify the Release helper no
  longer links XQuartz, and prepare reviewable follow-up fixes.
- Started: 2026-11-07
- Conflict check: do not edit `Character.cpp`, `CastSpellInfo.cpp`, or other
  shared MM6-lane files until their owner has handed them off.
- Planned checks: focused MM7 GameTests, title-isolation control, and linkage
  inspection before staging an app.
- Handoff target: `agent_comms/handoffs/`

