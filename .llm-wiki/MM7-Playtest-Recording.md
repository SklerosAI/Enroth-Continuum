---
type: synthesis
up: "[[Campaign-Status]]"
related: ["[[MM7-Disposable-Save-Reload]]", "[[Agent-Handoff]]"]
tags: [mm7, audit, mac, recording]
---

# MM7 Playtest Recording

The Atlas can start a passive, private recorder alongside an owner-played MM7 session. Source and synthetic checks now guarantee that the newest two-minute replay survives every exit; live macOS screen-capture permission and visible video quality remain owner-playtest gates.

## Persistent rolling replay — 2026-11-15

The first owner crash report arrived without video. Recording was disabled for that launch, and the only older recorder session had failed its screen capture with a macOS Screen Recording permission warning. The engine log and macOS crash report were still enough to identify the separate transition/combat defect recorded in [MM7 Gameplay Harness](MM7-Gameplay-Harness.md).

The recorder no longer deletes its rolling buffer after a normal exit. It keeps two completed 60-second chunks in `video/`, waits for the active chunk to complete at every exit and then retains the newest 120 seconds. Older chunks are replaced as recording continues, and the existing 20-session cap bounds completed sessions. Abnormal or unreported exits still copy the same window into a protected clip; the retained `video/` window is available even when no clip hotkey was pressed.

Nine focused synthetic cases pass across normal, abnormal, unreported, low-space, requested-clip, interrupted-copy, failed-permission and video-disabled paths. The Atlas page checks pass. The signed Release helper and recorder were installed from commit `95442333e`; the MM6 helper hash stayed unchanged, the prior Atlas is retained in the private rollback folder and both video and diagnostics are enabled for the next MM7 launch. macOS may still require the owner to grant Screen Recording permission to the newly signed Atlas before a real `.mov` can be inspected.

## Verified source, staging and headless install — 2026-11-10

The Atlas has independent persistent video and diagnostics toggles plus a folder picker for local or external storage. When enabled, Play launches the bundled Bash recorder after the MM7 process starts; the recorder watches that PID and ends after game exit. It records incremental engine-log byte ranges, stable changed-save snapshots, checksums, timestamps and optional owner notes in a unique session folder. Atlas keeps two completed 60-second screen-video chunks as a rolling buffer and retains them at exit. Pressing `Command-Shift-R` while tracking is active also writes the current window to a uniquely named protected `clips/` folder. The selected drive is pinned, Git-contained output is limited to ignored `local-game-data/`, and capture stops before the destination falls below 2 GiB free. Atlas warns below 4 GiB free and when the macOS memory-free reading is below 10%. An in-session recorder failure leaves MM7 running; a prelaunch destination failure blocks Play until corrected or recording is turned off.

The Bash 3.2 script passed synthetic checks for paths with spaces, notes/stop, changing-save deferral, log append/rotation/truncation, tracked-destination refusal, low-space stop and watched-PID exit. The rolling-buffer revision passed a synthetic two-chunk preservation request: it copied both current chunks into one unique clip folder while the source rolling files remained intact. The Atlas source typechecked and compiled with the macOS Carbon hotkey registration. No MM7 or macOS screen-capture process was run for this feature under the owner's current headless-only authorization.

A later independent static review found and closed quick-relaunch, early-ready,
warning-suppression, final-video-status and drive-remount races. Atlas now
waits for the recorder's ready status, keeps Play closed while the previous
recording finalizes, checks the selected volume ID across the Bash handoff and
reports uncertain final video. A synthetic wrong-volume attempt was rejected;
the matching-volume session started and stopped. The hardened stage was
rebuilt and its signatures, notices, page and packaged script rechecked.

After the owner authorized reinstalling Atlas, the signed recording candidate
was installed headlessly at `/Applications/Enroth Continuum.app`. The prior
bundle was moved to a local rollback directory under Application Support;
profiles and saves were not changed. The installed page and Bash script hashes
match tracked source, and the installed bundle passes deep signature
verification. No launch or actual capture has been observed after this swap.

The first live gate is to enable recording on a disposable profile, choose an owner-approved destination, grant macOS Screen Recording permission if requested, play beyond two chunk rotations, press `Command-Shift-R`, quit, and inspect the bounded rolling buffer, saved clip, incremental logs, final status and unchanged original saves. External-drive removal and low-memory warnings need later owner-attended checks. Display video can include other windows; system/game audio is not captured. Raw video, saves, logs and notes stay outside Git and cloud services.

The built-in Ctrl+Shift+R trace recorder remains a short defect-reproduction tool: it reloads a save, enters deterministic mode and redirects saves to memory while active, so it is unsuitable for the long durability playthrough.

Source: [Atlas launcher](../launcher/AtlasApp.swift), [recording script](../tools/mm7_playtest_capture.sh), [script usage](../tools/MM7_PLAYTEST_CAPTURE.md), [audit checklist](../project/MM7_AUDIT_CHECKLIST.md), reviewed 2026-11-10.

See also: [Campaign Status](Campaign-Status.md), [MM7 Disposable Save and Reload](MM7-Disposable-Save-Reload.md), [Agent Handoff](Agent-Handoff.md).
