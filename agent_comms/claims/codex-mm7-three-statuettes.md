# Codex — MM7 Three Statuettes route

- Owner: Codex, sole MM7 delivery owner.
- Branch/worktree: `codex/mm7-dark-chain` in `mm7-combat-fixes`.
- Files owned: `test/Bin/GameTest/GameTests_MM7ThreeStatuettes.cpp`, focused save-gate registration if completed, the MM7 quest catalog/wiki status, and the resulting handoff.
- Purpose: extend the existing Angel-only acquisition probe into Thom Lumbra's complete route: real offer/refusal, Knight/Eagle/Angel chest pickups, their Avlee/Tatalia/Bracada shrine interactions, and final state/rewards.
- Started: 2026-11-12 from neutral integration `024553678`.
- Boundary: headless Null-renderer work only today; do not run visible or offscreen-OpenGL processes or track private game data.
- Planned checks: focused route, regional side/promotion controls, changed-file lint, durable Save/Load if the full transaction is stable, and full `check_style` at the transaction boundary.
- Completed: 2026-11-12. Integrated on neutral `master` at `a21c0dff8`.
- Result: Thom Lumbra's offer/refusal, all three authored dungeon chests and outdoor shrine faces, placement bits, item consumption, 50,000 gold, 50,000 XP per character and the all-character award are covered. Production Save and separate normal Load preserved the completed state with unchanged slot bytes.
- Verification: focused route 1/1, regional side/promotion gate 12/12, exact writer/reader 1/1 each, both admission failures, changed-file lint and full `check_style` 157/157.
