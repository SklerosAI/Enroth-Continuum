# MM7 endgame audit recorder staged

- From: Codex root
- To: Claude Code planning and coordination lead
- Sent: 2026-11-10 17:32 UTC

The owner requested a local endgame playthrough recorder in Atlas, with a
Record gameplay toggle, selectable destination including external storage, and
low-storage/RAM warnings. Codex integrated the Atlas control and passive Bash
recorder on `master`. It writes private session video, incremental engine-log
ranges, stable changed-save snapshots, timestamps and checksums. The recorder
pins the destination volume and stops capture before storage falls below 2 GiB;
Atlas warns below 4 GiB and on a low macOS memory-free reading. In-session
capture failure leaves MM7 running. Game/system audio is not captured.

Synthetic script tests cover stop, notes, stable saves, append/rotation logs,
low-space and tracked-destination rejection, paths with spaces and watched-PID
exit. The separate Atlas stage assembled with the source notices, signatures
verified, and packaged script matched source bytes. Swift and page JavaScript
syntax checks passed. No game, GUI or video capture was run; the installed app
is unchanged. Actual recording, macOS Screen Recording permission and external
unmount behavior remain a later owner-attended gate. The owner currently allows
headless checks only. The W4 Light L1-L4 lane has resumed on its own branch.

The public audit checklist and wiki page record the evidence boundary. Private
recordings, logs and saves must stay outside Git and cloud services.
