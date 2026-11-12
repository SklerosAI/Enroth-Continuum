# Codex — MM7 Three Paintings route

- Owner: Codex, sole MM7 delivery owner.
- Branch/worktree: `codex/mm7-dark-chain` in `mm7-combat-fixes`.
- Files owned: a focused Three Paintings GameTest, its CMake registration, the MM7 quest catalog/wiki status, and the resulting handoff.
- Purpose: cover Visconti's real offer and missing-item refusal, physically acquire the Angel, Archibald and Roland paintings from their authored wall interactions, and verify item, quest, gold and experience transitions on return.
- Started: 2026-11-12 from neutral integration `450a44d9c`.
- Boundary: headless Null-renderer work only today; do not run visible or offscreen-OpenGL processes or track private game data.
- Planned checks: focused Three Paintings route, nearby side-quest controls, changed-file lint, and full `check_style` at the transaction boundary.
- Completed: 2026-11-12. Integrated on neutral `master` at `427f261fb`.
- Result: Visconti's actual offer/refusal, all three authored wall events, taken bits, item consumption, 50,000 gold and 50,000 XP per character are covered. Production Save and separate normal Load preserved the completed state with unchanged slot bytes.
- Verification: focused route 1/1, regional side/promotion gate 12/12, post-MM6-reconciliation capture/side gate 13/13, exact writer/reader 1/1 each, both admission failures, changed-file lint and full `check_style` 157/157.
