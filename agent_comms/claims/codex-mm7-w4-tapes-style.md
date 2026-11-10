# MM7 W4 advisor and Xenofex tape cleanup

- Owner: Codex root
- Branch/worktree: `codex/mm7-w4-tapes-style` in reused `mm7-load-admission`
- Files owned: `test/Bin/GameTest/GameTests_MM7LightAdvisors.cpp`, `GameTests_MM7LightXenofex.cpp`, and `GameTests_MM7LightSeam.cpp`
- Purpose: W4 review finding F5 and the F6 comments in these files. Tape quest, item, NPC topic and gold transitions around input-driven hand-ins, remove defensive comments, and keep the bounded fixtures explicit in the handoff.
- Checks: focused advisor, Xenofex and seam controls. No runtime source or disposable gate changes.
- Conflict check: Claude owns style and actor-scroll files. The continuous chain mode is reserved for Claude's pending plan.
