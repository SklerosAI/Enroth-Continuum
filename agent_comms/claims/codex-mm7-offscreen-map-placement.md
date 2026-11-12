# Codex — MM7 offscreen map-capture placement

- Owner: Codex, sole MM7 delivery owner.
- Branch/worktree: `codex/mm7-dark-chain` in `mm7-combat-fixes`.
- Files owned: `test/Bin/GameTest/GameTests_MM7OffscreenFrame.cpp`, focused test support if needed, and the MM7 offscreen-capture documentation.
- Purpose: diagnose the invalid Tatalia/Avlee/Shoals progress captures identified by the owner and make the diagnostic select a real target-map start decoration instead of retaining Emerald Island coordinates when `Party Start` is absent.
- Started: 2026-11-12 from integration `7058a30b5`.
- Boundary: inspect existing private PNGs, but do not track images or private asset details. Do not launch a visible or offscreen-GL process under today's headless-only instruction.
- Planned checks: source-backed placement test under the Null renderer, focused MM7 headless controls, `check_style`, and later hidden recapture only after the owner permits graphical/offscreen execution.
