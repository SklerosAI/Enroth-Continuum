# Codex — MM7 Missing Contestants quest

- Owner: Codex, sole MM7 delivery owner.
- Branch/worktree: `codex/mm7-dark-chain` in `mm7-combat-fixes`.
- Files owned: new Missing Contestants GameTest file, its `CMakeLists.txt` entry, and exact MM7 disposable-profile gate rows in `GameTestMain.cpp` only.
- Purpose: cover the real Markham offer/return, missing-shield refusal, physical Contestant Shield acquisition, item/state/reward contract and a fresh-process Save/Load boundary.
- Started: 2026-11-12 from integration `2d3f76c2b`.
- Conflict check: Claude owns MM6; this lane is MM7-only and does not change shared runtime code.
- Planned checks: focused ordinary route, separate-process writer/reader in a fresh marked profile, relevant regional controls, `check_style`, and reconciliation with neutral `master` before integration.
- Privacy: private game data and disposable saves remain local and untracked.
