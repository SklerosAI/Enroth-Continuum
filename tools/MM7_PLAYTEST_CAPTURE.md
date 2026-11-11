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

Standalone use can pass `--output "/absolute/existing/base folder"` instead of `--session`; the tool creates a unique child session and prints `SESSION_DIR=...` immediately. Atlas passes the device ID saved when you chose the folder through `--expected-volume-id`; a standalone caller can omit it. The script checks that ID before and after session creation so an unmounted drive cannot redirect the recording to a writable mountpoint on the internal disk. `start` remains in the foreground so its exit can be observed. Screen video is **off by default**; use `--video` to maintain two alternating rolling `.mov` chunks on the selected display. System/game audio is not captured. `--video-chunk-seconds` defaults to 60 (range 10–3600), `--poll-seconds` to 5 (range 1–60), `--save-checkpoint-seconds` to 600 (range 1–3600), and `--min-free-mib` to 2048 (minimum 128). Atlas always uses 60-second video chunks and the 600-second save interval.

From another terminal, add a timestamped marker or stop the recording:

```sh
/bin/bash tools/mm7_playtest_capture.sh note --session "/absolute/private/session" --text "Entered castle; inventory looked wrong"
/bin/bash tools/mm7_playtest_capture.sh clip --session "/absolute/private/session"
/bin/bash tools/mm7_playtest_capture.sh status --session "/absolute/private/session"
/bin/bash tools/mm7_playtest_capture.sh stop --session "/absolute/private/session"
```

`clip` requests a unique copy of the latest completed rolling video chunks. In Atlas, press `⌘⇧R` instead; the hotkey is active only while Atlas tracking is active and does not conflict with MM7's `Ctrl+Shift+R` deterministic trace recorder. `stop` writes a request file and returns; the recorder finishes its next check, makes a final log/save pass, stops only its own screen-capture child, and writes the final `status.json`. Closing MM7 also ends recording after the PID check. A session already in progress is never reused. Atlas can poll `status.json` and the optional `warnings.txt` for health. The warning file appears only when a problem is detected.

## What it keeps

- `identity.txt`: UTC start time, canonical profile and session paths, source Git revision when available, script hash, Atlas/helper IDs and executable hashes, game PID, destination volume ID, and video choice. A bundled script may have no adjacent Git checkout; its script hash and app hashes still identify the build.
- `timeline.tsv`: UTC start, stop, snapshots, video-chunk events and owner notes. `note` accepts one line up to 500 characters; write sensitive detail here only because this folder is private.
- `logs/`: append-only byte-range copies of changing `enroth-continuum-mm7*.log` files. `snapshots.tsv` records the source, copy, checksum, starting byte and length for each `.log.part`. Reassemble a log in manifest order. The recorder starts a new range at byte zero after common rotation/truncation signals (new inode, shorter file or changed prefix). An active range can end mid-line; the next range continues it. In-place edits outside the checked prefix are not guaranteed to be detected.
- `saves/`: changed `.mm7` slots and `.mm7.bak` backups, plus a fresh immutable checkpoint copy of every stable save every 10 minutes. Each source must have unchanged device/inode, size, modification time and SHA-256 across a one-second interval and after copying. A changing save is deferred to a later poll. `snapshots.tsv` records each copied file's source, destination, time and checksum. The source saves are never modified.
- `video/`: at most two optional rolling screen-recording chunks. Atlas uses two 60-second chunks, so the live buffer is capped at about two minutes; the current incomplete chunk is discarded when recording stops. Video is not guaranteed until macOS grants Screen Recording permission. It captures the **whole selected display**, including other apps or notifications visible there.
- `clips/`: unique, intentionally retained copies of the latest one or two completed rolling chunks created by `clip` or Atlas's `⌘⇧R` hotkey. These are the only video files that accumulate across a session.

The recorder checks free storage on the selected destination and stops at the configured floor. Atlas can warn earlier. A disconnected or replaced external volume ends capture without redirecting output to the Mac's internal disk. The rolling buffer is bounded, but intentionally saved clips can accumulate; review or move those clips when finished. Screen video does not replace the engine logs or stable saves for diagnosis.

Keep the entire session folder out of Git and cloud sync. It can contain proprietary game frames, saves, logs and private notes. Share only a brief bug summary unless specific local evidence is deliberately reviewed for transfer.
