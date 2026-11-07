# Codex MM7 maintenance handoff — 2026-11-07

- Owner: Codex. Recipients: Claude MM6 integrator and future MM7 maintainers.
- Branch/worktree: `mm6-mac-implementation` in the main checkout. Code commits:
  `c11c2c1f9`, `f70627f9c`, `f3f08539c`.
- Owned source: `src/Media/MediaPlayer.cpp`,
  `test/Bin/GameTest/GameTests_2500.cpp`, `launcher/AtlasApp.swift`,
  `launcher/Atlas/index.html`, `src/Engine/AssetsManager.cpp`,
  `src/Engine/Tables/ItemTable.cpp`, and `src/GUI/UI/UIPopup.cpp`.
- Completed: restored MM7 house pause on missing or disabled video; added the
  Atlas Finder action for the local MM7 mod directory, PNG re-encoding for
  portrait imports, title checks on portrait/armor hooks, clearing of stale
  armor preferences and fields, correct launcher message after abnormal MM7
  exit, Grandmaster spellbook indent reset and AC tooltip display when base
  AC is zero but bonus AC is positive.
- Checks: Swift type check and Atlas inline JavaScript syntax passed. The MM7
  GameTest build passed; `Prs.Pr2956`, `Issues.Issue840` and `Prs.Pr2354`
  passed 3/3 together. A temporary Atlas opened its new mod-folder control.
  The rebuilt Release `MM7.app` links Apple's OpenGL framework with no
  XQuartz dynamic library. Staged and installed bundles passed code-signature
  verification. The installed Atlas launched MM7 to the title menu and
  returned normally. The previous installed app bundle is retained under
  ignored `local-game-data/app-backups/`.
- Remaining: MM7 save atomicity and corrupt-save handling involve `SaveLoad*`
  and `UISaveLoad.cpp`, which the active MM6 normal-save lane owns. Combat
  defects in `Character.cpp` and `CastSpellInfo.cpp` likewise overlap the
  MM6 spells lane. Coordinate ownership after those lanes hand off. The
  installed helper still uses cached media-library archives; source-owned
  dependency cutover, mod expression portraits and mod error display remain
  open. The tooltip and Grandmaster rows need visual comparison.
- MM6 read-only overlap review: five tracked paths currently have changes in
  multiple MM6 lanes: `src/Application/Game.cpp`,
  `src/Engine/Evt/EvtInterpreter.cpp`, `src/Engine/MM6/CMakeLists.txt`,
  `test/Bin/GameTest/CMakeLists.txt`, and
  `test/Bin/GameTest/GameTests_MM6V5AfterActionFreshChildReplay.cpp`. The
  last path deserves a semantic review when first-map and inventory lanes
  integrate. No Claude worktree was edited by this lane.
- Privacy: no private game data, saves, logs, mod files or detailed asset
  inventories were added to Git. Captures and the previous app remain local
  and ignored.
