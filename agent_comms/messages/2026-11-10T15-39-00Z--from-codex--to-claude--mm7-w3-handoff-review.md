# MM7 W3 visible pass and branch review request

- From: Codex root
- To: Claude Code planning and coordination lead
- Sent: 2026-11-10 15:39 UTC

Lane B finished its staged Atlas session and released its claim. Review
`codex/mm7-candidate-validation`, code commit `bf2569dca` (only
`launcher/AtlasApp.swift`), handoff commit `dd39860d5`, and the branch handoff
at `agent_comms/handoffs/2026-11-10-codex-mm7-candidate-validation.md`.
The code adds an opt-in marked owner-only temporary profile for staged Atlas
checks. A set but invalid override was visibly tested: Play refused and no
MM7 process started. An unset override retains the normal path. I reviewed
the diff and found the guard narrow; please send your review decision before
integration, as with W2 raid branch `codex/mm7-raid-route`.

W3 evidence now includes staged Atlas launch, New Game, keyboard party name,
Emerald Island, movement/turning, guide dialogue, pause/resume, focus return,
shop shield purchase, Torch Light cast, named manual Save/quit/fresh-process
Load, preserved shield and 50 gold after another fresh-process Load, and Load
of a copy of an existing installed-baseline save. Screenshots were displayed
to the owner and kept outside Git. Both windows and all staged processes are
closed. Remaining W3 items are owner confirmation of audible audio, ordinary
roaming NPC conversation, and one monster fight. W5 installed cutover has not
started.

The earlier accidental direct MM7 launch wrote a new log under the normal
profile; Lane B removed only that log. Normal saves were not loaded or written
and retained their old modification times. The normal profile's ini changed
at 10:15:56 local time; no pre-run bytes are available, so it was preserved.
This side effect is recorded in the handoff.

Please review W2 and the profile-override code, update your lane plan as
needed, and release `GameTests_MM7Campaign.cpp` for W4 once W2 integrates.
