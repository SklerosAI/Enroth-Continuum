# Claude — master style fix, then actor-target scroll cancel

- Owner: Claude Code lane subagent.
- Branch/worktree: two branches in the `mm7-save-durability` checkout, in order. `claude/master-style-fix` from current `master`. Then `claude/mm7-actor-scroll-cancel` stacked on `claude/mm7-scroll-cancel` (`401d8e2a4`), rebased onto `master` once that integrates.
- Files owned: style fix: `src/Engine/Mods/PrivateMods.cpp`, `src/Engine/Mods/PrivateMods_ut.cpp`, `src/Engine/Objects/Chest.cpp`, and any other file the full `check_style` reports, include and using-directive changes only. Actor scrolls: the scroll branch of `Character::useItem` in `src/Engine/Objects/Character.cpp`, the `UIMSG_SpellScrollUse` and targeting-cancel paths in `src/Application/Game.cpp`, `src/Engine/Spells/CastSpellInfo.{h,cpp}`, and `test/Bin/GameTest/GameTests_MM7Scrolls.cpp`.
- Purpose: stage 1 defect queue items 4 and 3.
- Started: 2026-11-10.
- Conflict check: Codex's Tolberti seam lane owns its Light test files and `GameTestMain.cpp` filters. No overlap.
- Planned checks: full `check_style` for the style fix. For actor scrolls, a confirming throwaway test, a cancel and complete loop, revert-and-fail, existing scroll tests, unit tests and the full headless MM7 binary.
- Handoff target: Codex root for integration after Claude's review.
