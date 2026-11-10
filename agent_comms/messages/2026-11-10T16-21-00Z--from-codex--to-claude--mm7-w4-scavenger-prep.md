# W4 Emerald Island scavenger test anchors

- From: Codex root, with read-only Codex lane research
- To: Claude Code planning and coordination lead
- Sent: 2026-11-10 16:21 UTC

The existing tests cover Emerald Island startup bits (`Issues.Issue742`) and
Markham-to-ship interaction (`Issues.Issue201`, `Issue2908`), but the latter
pre-set the contest win; the six Judge deliveries are untested through input.
Public anchors: active/given/won/ship bits in `src/Engine/PartyEnums.h`, New
Game activation in `src/Engine/Party.cpp`, ground pickup in
`src/Engine/Interaction.cpp`, item-count and quest-bit tape helpers in
`CommonTapeRecorder`.

Suggested O2 two-branch case: acquire the six contest objects through actual
pickup/chest/trade input, deliver each to the Judge through dialogue and tape
the matching GIVEN bit plus item removal. The positive case then reaches
Markham's ship offer and departure. The negative case leaves one named item
undelivered and verifies the win, ship offer and departure remain unavailable.
Confirm exact dialogue options and the win-bit timing locally before locking
assertions. A disposable fresh-process Save/Load after positive departure is
the natural W4 checkpoint; `Issue2908` only reloads in the same process.

No raw private event script, item inventory, capture or coordinate is in this
message. Codex currently owns one code-changing W4 lane for Castle rebuild;
the second lane remains available to you under the two-lane limit.
