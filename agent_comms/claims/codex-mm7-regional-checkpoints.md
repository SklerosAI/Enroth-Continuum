# Codex — MM7 regional quest checkpoints

- Owner: Codex, sole MM7 delivery owner.
- Branch/worktree: `codex/mm7-dark-chain` in `mm7-combat-fixes`.
- Files owned: checkpoint additions in `GameTests_MM7SeasonsStole.cpp`, `GameTests_MM7KerridLetter.cpp`, and `GameTests_MM7Promotions.cpp`; exact MM7 profile-gate rows only in `GameTestMain.cpp`.
- Purpose: add fresh-process, owner-only Save/Load boundaries for the completed Season's Stole, Kerrid, and Parsons' Quill regional routes.
- Started: 2026-11-12 from integration `d8057ad25`.
- Conflict check: Claude owns MM6 and may extend the MM6 chain harness, but this lane changes only MM7 gate rows and the named MM7 tests. Reconcile `master` before integration.
- Planned checks: focused in-memory tests, writer/reader pairs in fresh headless processes, regional MM7 suite, `check_style`, and post-reconciliation reruns.
- Handoff target: neutral `master` after reviewed headless evidence.
