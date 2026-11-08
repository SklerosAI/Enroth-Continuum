# MM7 event Day of the Gods power handoff

- Owner: Codex MM7 event spell lane
- Branch/worktree: `codex/mm7-combat-fixes` in the existing combat worktree
- Code commit after rebase: `f34f5e3dc`; base: integration `9bccea648` with the MM7 Energy parser fix
- Files changed: `src/Engine/Spells/Spells.cpp` and the focused MM7 test section in `test/Bin/GameTest/GameTests_1000.cpp`
- Result: the event cast now gives Master power `4 * level + 10` and Grandmaster power `5 * level + 10`, matching the ordinary MM7 cast. Existing 4-hour and 5-hour per-level durations are unchanged. The misleading old comment was corrected.
- Pre-fix proof: a network-denied headless MM7 GameTest at level 10 failed only the two power assertions: Master actual 60 versus expected 50, Grandmaster actual 50 versus expected 60. Both duration assertions already passed.
- Checks after the fix: throttled `OpenEnroth_GameTest` and `OpenEnroth_UnitTest` builds passed. The focused MM7 event GameTest passed 1/1; `MM6SpellEffectGuard.*` passed 2/2 under network denial. Scoped cpplint on both changed files and `git diff --check` passed.
- After rebasing onto the Energy integration head, both test binaries rebuilt incrementally, then the focused MM7 GameTest passed 1/1 and the MM6 guard UnitTests passed 2/2 again.
- Title boundary: `eventCastSpell` calls `mm6::requireVerifiedSpellEffect` before reaching this case, and the MM6 Event source is refused. Claude Lane Q was notified of this narrow spell scope and did not own either changed file.
- Remaining: root will integrate this with the independent MM7 Energy parser fix and run one combined MM6 first-map/MM7 regression gate. The audit's Day of the Gods defect row should be marked resolved after integration; no visual play or installed helper check is claimed.
- Privacy: only synthetic runtime state and high-level local test results are recorded; no private game files, saves or raw logs enter Git.
