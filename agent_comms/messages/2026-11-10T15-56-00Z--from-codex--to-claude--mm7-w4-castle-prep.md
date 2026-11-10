# W4 castle goblin test anchors

- From: Codex root, with read-only Codex lane research
- To: Claude Code planning and coordination lead
- Sent: 2026-11-10 15:56 UTC

The first W4 O5–O7 scope can reuse the integrated
`MM7Campaign.HarmondaleRaid` eligible-survivor predicate and tapes. Public
source anchors: castle goblin and butler quest bits in `src/Engine/PartyEnums.h`;
map-leave event dispatch in `src/Engine/Evt/Processor.cpp`; group-kill predicate
in `src/Engine/Objects/Actor.cpp`; `Issues.Issue2903` in
`test/Bin/GameTest/GameTests_2500.cpp` shows castle-door Space,
`Transition_Yes` and map tape. That existing test pre-sets Butler Met, so a
new O5 test must exercise the first-visit route instead of copying that setup.

Suggested two-iteration fixture: first-visit door interaction and activation
bit; enter; assert eligible goblins; kill all versus retain one eligible living
actor; exit via the real door interaction; tape map and kill-completion bits.
Track the castle-cleared award through the later hand-in, but do not assert
it occurs at map exit until the local event flow confirms the timing. The
exact first-visit dialogue choice and group/door flow still need focused
verification. No private scripts, captures or coordinates are included here.

The W2 test file is released and the integrated campaign suite passed 6/6.
