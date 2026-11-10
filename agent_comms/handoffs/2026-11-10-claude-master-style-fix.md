# Master style fix handoff

- Owner: Claude Code lane subagent, reviewed by Claude lead. Recipient: Codex root for integration.
- Branch: `claude/master-style-fix`, commit `349e55d62` Make the full style check pass on master again, based on `master` `245a37fe6`. Not pushed or merged.
- Scope: 57 files under `src/`, mostly MM6 code. Full `check_style` on `master` reported 75 errors in about 55 files, not only the three named in the earlier message.

## Change

Missing standard headers added, a using-directive in `src/Engine/Mods/PrivateMods_ut.cpp` replaced by two using-declarations, include order fixed in three files, wrapped namespace-scope declarations joined onto one line, access-label indent in one header, an `} else switch` in `UIPartyCreation.cpp` braced (whitespace-only otherwise), braces around a two-line `if` body in a unit test, `EXPECT_FALSE(a == b)` to `EXPECT_NE`, `std::errc{}` to `std::errc()`. No behavior change.

## Checks

- Full `check_style` passes. cpplint over all 154 style commands reports 0 errors.
- `OpenEnroth_UnitTest` and `OpenEnroth_GameTest` build. Unit tests 1087 passed, 45 skipped, 0 failed.
- `check_tidy` and the Lua style check were not run. clang-tidy and LuaLS are not installed on this machine.

## Lead review

Read the whole non-include part of the diff with whitespace ignored. Every hunk is mechanical. The one added brace pair encloses exactly the original `for` body.

## Integration note

`master` has moved since `245a37fe6`. Rebase or cherry-pick, then rerun full `check_style`, because files added since may need the same includes. Please integrate this before the next shared-code merge.

Privacy: no game data, saves or logs in Git.
