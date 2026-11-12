# Codex — MM7 offscreen map-capture placement

- Owner: Codex, sole MM7 delivery owner.
- Branch/worktree: `codex/mm7-dark-chain` in `mm7-combat-fixes`.
- Files owned: `test/Bin/GameTest/GameTests_MM7OffscreenFrame.cpp`, focused test support if needed, and the MM7 offscreen-capture documentation.
- Purpose: diagnose the invalid Tatalia/Avlee/Shoals progress captures identified by the owner and make the diagnostic select a real target-map start decoration instead of retaining Emerald Island coordinates when `Party Start` is absent.
- Started: 2026-11-12 from integration `7058a30b5`.
- Boundary: inspect existing private PNGs, but do not track images or private asset details. Do not launch a visible or offscreen-GL process under today's headless-only instruction.
- Planned checks: source-backed placement test under the Null renderer, focused MM7 headless controls, `check_style`, and later hidden recapture only after the owner permits graphical/offscreen execution.
- Completed: 2026-11-12. Integrated on neutral `master` at `328b19d33`.
- Result: Tatalia and Avlee were proved to lack `Party Start`; the capture now selects the first available party/cardinal start on the target map and fails when none exists. The Null-renderer regression passed across Tatalia, Avlee and Shoals after the current MM6 integration, and `check_style` passed 157/157.
- Deferred visual gate: no graphical recapture ran under the headless-only instruction. Corrected Tatalia/Avlee frames and Shoals fog still require direct image inspection when graphical execution is permitted.
