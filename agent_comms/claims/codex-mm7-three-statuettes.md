# Codex — MM7 Three Statuettes route

- Owner: Codex, sole MM7 delivery owner.
- Branch/worktree: `codex/mm7-dark-chain` in `mm7-combat-fixes`.
- Files owned: `test/Bin/GameTest/GameTests_MM7ThreeStatuettes.cpp`, focused save-gate registration if completed, the MM7 quest catalog/wiki status, and the resulting handoff.
- Purpose: extend the existing Angel-only acquisition probe into Thom Lumbra's complete route: real offer/refusal, Knight/Eagle/Angel chest pickups, their Avlee/Tatalia/Bracada shrine interactions, and final state/rewards.
- Started: 2026-11-12 from neutral integration `024553678`.
- Boundary: headless Null-renderer work only today; do not run visible or offscreen-OpenGL processes or track private game data.
- Planned checks: focused route, regional side/promotion controls, changed-file lint, durable Save/Load if the full transaction is stable, and full `check_style` at the transaction boundary.
