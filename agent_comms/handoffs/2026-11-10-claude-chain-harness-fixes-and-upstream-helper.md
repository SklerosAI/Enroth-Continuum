# Chain harness fixes and upstream OpenEnroth helper

- Owner: Claude Code lanes, reviewed by Claude lead. Recipient: the Claude integration lane, then Codex on its return.

## Chain harness fixes

- Branch `claude/mm7-chain-harness-fixes` on top of `codex/mm7-chain-harness` (`a30db1654`): `4aa8dda15`, `fc4ff8305`, `380d79763`, `b915d9d4d`. Queued for integration with Codex's commits, in order.
- F1: the runner writes `saves/save{N}.mm7.passed` holding the slot's SHA-256 after a clean chapter, and resume trusts only matching markers. An unmarked slot is removed and the chapter reruns. An unmarked C0 resets the profile to its marker. `--through N` exits 0 only with markers through N. The runner fails with "C<N> test not implemented" before touching a profile.
- F2: a chapter refuses its own slot, backup or marker.
- F4: the claim and handoff state the keep-all policy.
- F5: C0 turns Lady Margaret's tour off through her dialogue instead of `NoMargaret`. Markham's deed sets the same bit later in the route either way.
- Checks: C0 through the runner, the missing-predecessor, own-slot, later-slot and stray-file negatives, resume after a deleted marker and a tampered slot, a bad data path creating no profile, `--remove-profile`, the five legacy writer and reader pairs 10/10, unit tests 1087 passed and 45 skipped, the headless suite 535/535 through gtest-parallel. `check_tidy` not run (no clang-tidy). The branch's build has style checks off, so cpplint ran directly and was clean on changed files.

## Upstream OpenEnroth helper

- Upstream master `8010ac364` built as-is in the `openenroth-upstream` checkout, RelWithDebInfo arm64 as upstream's macOS release job does. The bundle is staged outside Git under the owner's Application Support `Staging/OpenEnroth-upstream/OpenEnroth.app`, 18 MB, ad-hoc signed, bundle ID `OpenEnroth`, version `nightly`.
- CMake picked XQuartz's libGL on this Mac. Pinning the system OpenGL framework at configure time fixed it without source changes, and the refresh tool rejects non-system links.
- A headless retrace of four upstream test traces with the owner's MM7 data, read-only, and a temp user path matched byte for byte. No window opened, and nothing was written to game data or profiles.
- Refresh: `tools/openenroth_upstream_refresh.sh` (`7d0103bef`).
- Launch facts for The Atlas: `--data-path` and `--user-path`, both must exist, no game selection flag. The Atlas side is in the recorder lane (`claims/claude-mm7-rolling-replay.md`).

Privacy: no game data, saves or logs in Git.
