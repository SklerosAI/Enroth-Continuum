# Codex — MM7 maintenance

- Owner: Codex
- Branch/worktree: `mm6-mac-implementation` main integration checkout
- Files owned for current MM7 follow-up: `src/Engine/Mods/PrivateMods.{h,cpp}`,
  `src/Engine/AssetsManager.{h,cpp}`, `src/Engine/Objects/TalkAnimation.h`,
  MM7 portrait draws in `UIGame.cpp`, `UIPopup.cpp`, `UIPartyCreation.cpp`, and
  `launcher/AtlasApp.swift` / `launcher/Atlas/index.html`. The portrait slice
  was integrated at `b6a1515c3`; coordinate before further edits to these paths.
- Purpose: repair isolated MM7 regressions, verify the Release helper no
  longer links XQuartz, and prepare reviewable follow-up fixes.
- Started: 2026-11-07
- Conflict check: do not edit `Character.cpp`, `CastSpellInfo.cpp`, or other
  shared MM6-lane files until their owner has handed them off.
- Planned checks: focused MM7 GameTests, title-isolation control, and linkage
  inspection before staging an app.
- Handoff target: `agent_comms/handoffs/`

