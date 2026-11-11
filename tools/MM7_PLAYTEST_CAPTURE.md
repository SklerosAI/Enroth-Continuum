# Local MM7 playtest recording

`mm7_playtest_capture.sh` passively records an owner-played MM7 session. It does not start, stop, signal, or configure the game. Atlas can bundle it and start it after Play; the recorder observes the supplied MM7 PID and finishes when that process exits. The owner can also stop it by writing a stop request through the CLI. Ordinary gameplay remains unchanged.

The recording stays in a private, owner-only session folder. Choose an existing destination under ignored `local-game-data/` or an existing folder outside any Git checkout, including an external drive. The tool refuses a tracked destination, a destination inside the MM7 profile, or a missing/replaced destination volume. It never uploads anything.

## Start and stop

Atlas uses the `--session` form, supplying a new child name under the chosen recording folder:

```sh
/bin/bash tools/mm7_playtest_capture.sh start \
  --profile "/absolute/path/to/MM7-profile" \
  --session "/absolute/private/recordings/mm7-playtest-session-name" \
  --app "/absolute/path/to/Enroth Continuum.app" \
  --game-pid 12345 --video --display 1 --video-chunk-seconds 60 --min-free-mib 2048
```

Standalone use can pass `--output "/absolute/existing/base folder"` instead of `--session`; the tool creates a unique child session and prints `SESSION_DIR=...` immediately. Atlas passes the device ID saved when you chose the folder through `--expected-volume-id`; a standalone caller can omit it. The script checks that ID before and after session creation so an unmounted drive cannot redirect the recording to a writable mountpoint on the internal disk. `start` remains in the foreground so its exit can be observed. Screen video is **off by default**; use `--video` to keep a rolling replay of the selected display. System/game audio is not captured. `--video-chunk-seconds` defaults to 60 (range 10–3600), `--poll-seconds` to 5 (range 1–60), `--save-checkpoint-seconds` to 600 (range 1–3600), and `--min-free-mib` to 2048 (minimum 128). Atlas always uses 60-second video chunks and the 600-second save interval.

From another terminal, add a timestamped marker or stop the recording:

```sh
/bin/bash tools/mm7_playtest_capture.sh note --session "/absolute/private/session" --text "Entered castle; inventory looked wrong"
/bin/bash tools/mm7_playtest_capture.sh clip --session "/absolute/private/session"
/bin/bash tools/mm7_playtest_capture.sh status --session "/absolute/private/session"
/bin/bash tools/mm7_playtest_capture.sh stop --session "/absolute/private/session" --game-exit normal
```

`clip` saves the rolling replay into a new clip folder. In Atlas, press `⌘⇧R` instead. `stop` writes a request file and returns. The recorder finishes its next check, makes a final log/save pass, stops only its own screen-capture child, and writes the final `status.json`. Atlas passes `--game-exit normal` or `--game-exit abnormal` from MM7's exit status. Closing MM7 also ends recording after the PID check. A session already in progress is never reused. Atlas can poll `status.json` and the optional `warnings.txt` for health. The warning file appears only when a problem is detected.

## What it keeps

- `identity.txt`: UTC start time, canonical profile and session paths, source Git revision when available, script hash, Atlas/helper IDs and executable hashes, game PID, destination volume ID, and video choice. A bundled script may have no adjacent Git checkout; its script hash and app hashes still identify the build.
- `timeline.tsv`: UTC start, stop, snapshots, video-chunk events and owner notes. `note` accepts one line up to 500 characters; write sensitive detail here only because this folder is private.
- `logs/`: append-only byte-range copies of changing `enroth-continuum-mm7*.log` files. `snapshots.tsv` records the source, copy, checksum, starting byte and length for each `.log.part`. Reassemble a log in manifest order. The recorder starts a new range at byte zero after common rotation/truncation signals (new inode, shorter file or changed prefix). An active range can end mid-line; the next range continues it. In-place edits outside the checked prefix are not guaranteed to be detected.
- `saves/`: changed `.mm7` slots and `.mm7.bak` backups, plus a fresh immutable checkpoint copy of every stable save every 10 minutes. Each source must have unchanged device/inode, size, modification time and SHA-256 across a one-second interval and after copying. A changing save is deferred to a later poll. `snapshots.tsv` records each copied file's source, destination, time and checksum. The source saves are never modified.
- `video/`: the rolling replay, present only while recording. `screencapture` records 60-second chunks back to back. The recorder keeps the two newest completed chunks and the one being recorded, and deletes older ones as each chunk completes, so the buffer never holds more than about three minutes and always holds the last two once that much has been recorded. With other `--video-chunk-seconds` values it keeps enough completed chunks to cover two minutes. The buffer is deleted when recording ends. Video is not guaranteed until macOS grants Screen Recording permission. If a chunk fails, `warnings.txt` says to check that permission and video stops for the session. It captures the **whole selected display**, including other apps or notifications visible there.
- `clips/`: one folder per clip, `clip-<time>-<n>/`, holding whole chunks in recording order. The roll never deletes them. A clip starts with the completed chunks at the moment of the request and receives the chunk being recorded when that chunk completes, up to a minute later, so it covers at least the two minutes before the request. `screencapture` finalizes a chunk only when it completes, so a partial chunk is never kept. A clip is saved when the owner asks for one, when MM7 exits abnormally (crash, signal or non-zero exit) and when the game PID disappears without a stop request, since its exit status is then unknown. For an exit clip the recorder waits for the last chunk to complete, at most the chunk length plus 15 seconds. A normal exit with no clip pending deletes the buffer. Clips use hard links where the volume supports them and copies elsewhere, such as on exFAT drives.

The recorder checks free storage on the selected destination and stops at the configured floor. Atlas can warn earlier. A disconnected or replaced external volume ends capture without redirecting output to the Mac's internal disk. The rolling buffer is bounded, but saved clips accumulate; review or move them when finished. Screen video does not replace the engine logs or stable saves for diagnosis.

Keep the entire session folder out of Git and cloud sync. It can contain proprietary game frames, saves, logs and private notes. Share only a brief bug summary unless specific local evidence is deliberately reviewed for transfer.
