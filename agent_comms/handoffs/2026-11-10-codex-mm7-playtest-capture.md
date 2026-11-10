# MM7 passive playtest recorder handoff

- Branch: `codex/mm7-playtest-capture`, based on integration master `ca6fc1e90` in an existing clean worktree.
- Code commits: `4c1b99778` (recorder and public usage contract), `a53eae90f` (append-only engine-log deltas).
- Coordination claim was closed with this handoff. No launcher, engine, campaign, wiki, or main-tree file was edited in this lane.

## Contract

`/bin/bash tools/mm7_playtest_capture.sh start --profile ABS_DIR --session ABS_NEW_DIR --app ABS_ATLAS_APP --game-pid PID --video --display 1 --min-free-mib 2048` matches the Atlas integration call. `--output ABS_EXISTING_BASE_DIR` instead creates and prints a unique session child for standalone use. `stop --session ABS_DIR` writes a stop marker; `note --session ABS_DIR --text TEXT` adds a timestamped owner note; `status --session ABS_DIR` prints the atomic status JSON. Video is off unless `--video` is explicit. The script never launches or signals MM7. It signals only its own optional screen-capture child during cleanup.

The session contains private identity, UTC timeline, incremental log byte ranges with rotation/truncation detection, stable changed-save copies and checksums, optional timed screen-video chunks, and a `warnings.txt` file only when a problem occurs. It pins the destination filesystem device and refuses tracked Git locations, so an unavailable external volume does not silently redirect output. See `tools/MM7_PLAYTEST_CAPTURE.md` for usage, storage, privacy and evidence limits.

## Verification

- `/bin/bash` is macOS Bash 3.2.57; `bash -n tools/mm7_playtest_capture.sh` and `git diff --cached --check` passed.
- Synthetic profile and fake app bundle: `--session` under a path containing spaces, stable save and engine-log copies, owner note, stop marker, final status, unchanged source save, absent warning file passed.
- Synthetic low-space and tracked-destination rejection passed.
- Synthetic `--output` unique session and automatic exit when a non-game watched PID ended passed.
- A changing synthetic save was deferred until stable; all final snapshots matched its final bytes.
- Growing synthetic log append, inode rotation and truncation yielded exact nonduplicated `.log.part` ranges.

No game or GUI process was started, and screen capture was not run. Actual macOS Screen Recording permission, video finalization after interruption, external-drive removal, and Atlas-to-recorder lifecycle remain to be validated in a later owner-authorized local session. `screencapture` here advertises timed video and display selection; game/system audio is not claimed. Never commit or upload a recorded session: it can include proprietary frames, saves, logs and owner notes.

Follow-up code commit `301eb232d` narrows Git-contained destinations to ignored `local-game-data/` only. A synthetic temporary Git repository confirmed that an otherwise ignored non-vault path is rejected and an ignored local vault child is accepted. Bash syntax and staged diff checks passed again.
