# MM7 rolling replay, upstream button and diagnostics recording

- Owner: Claude Code lane (taking over Codex's paused edits), reviewed and installed by Claude lead.
- On `master` as `992fb71d2` (Codex's carried edits), `a07d71377`, `9b16d2e4b`, `e0f3fae23`, `6c54fe903`.
- Installed 2026-11-10 21:54 local at `/Applications/Enroth Continuum.app`. The previous app is the rollback copy under the owner's Application Support `Atlas Backups/Enroth Continuum-pre-rolling-replay-20261110-215456.app`. The MM7 helper is byte-identical to the one that passed W3 and W5. The bundle signature verifies.

## Behavior

- Record video: numbered 60-second `screencapture` chunks. The oldest roll off, so disk holds at most two finished chunks plus the one recording. Command-Shift-R keeps a clip of at least the last two minutes, linked rather than copied. An abnormal or unreported MM7 exit keeps a clip. A normal exit discards the buffer.
- Record diagnostics, independent of video and needing no Screen Recording permission: log ranges, save snapshots, timeline, build identity and launch command, exit detail, 30-second CPU and memory samples, and macOS crash reports for the game's PID after an abnormal exit.
- Play upstream OpenEnroth: runs `Contents/Helpers/OpenEnroth.app` (upstream `8010ac364`) with only `--data-path` and `--user-path` on its own `Profiles/MM7-OpenEnroth`, with fallback paths removed from its environment. It is shown disabled when the helper is missing.

## Checks

On `master`: `tools/tests/test_mm7_playtest_capture.py` 18/18 and `launcher/tests/atlas_page.js` pass. In the lane: the Swift launcher checks pass and the build compiles without warnings.

## Not yet verified, owner check

Real `screencapture` chunking, the hotkey in the GUI, an upstream launch with real data, and a real crash report. Screen Recording permission must be granted again, because the ad-hoc code hash changed. Suggested check: grant permission, play about three minutes, press Command-Shift-R, confirm the clip holds three chunks of about 60 seconds. Then launch upstream once and confirm its saves land in `MM7-OpenEnroth`.

Privacy: no video, saves, logs or game data in Git.
