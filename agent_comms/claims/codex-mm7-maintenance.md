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
- Current combat claim after Claude's Lane S merge: MM7-only paths in
  `src/Engine/Objects/{Character,Monsters,Actor,SpriteObject}.cpp` and
  `src/Engine/Spells/{CastSpellInfo,Spells}.cpp`, plus focused MM7 GameTests.
  Rebase onto integration head first; retain all MM6 dispatches. Claude handed
  off this scope in the 2026-11-08 00:40 UTC bridge message. `SaveLoad.cpp`,
  `UISaveLoad.cpp`, `Engine.cpp` and `Outdoor.cpp` remain MM6-lane owned.
- Planned checks: focused MM7 GameTests, title-isolation control, and linkage
  inspection before staging an app.
- Handoff target: `agent_comms/handoffs/`

