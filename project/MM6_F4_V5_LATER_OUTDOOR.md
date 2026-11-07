# MM6 v5 later-outdoor admission

Status, 2026-11-07: the source and direct world-load gates passed for the
shipped outdoor maps; no later-map `.ec6` candidate or fresh-child replay has
passed. Production MM6 SaveLoad and Atlas Play remain closed.

The v5 outdoor preflight, runtime actor/sprite/decoration replay scopes and
map identity check now admit the exact 15 shipped outdoor grid names. They
retain title, slot, source-map and native-anchor checks; v1–v4 rules are not
relaxed. The read-only headless source audit decoded all 15 outdoor base/delta
pairs, and focused v5 later-map replay checks passed 3/3. A direct headless
`oute2.odm` world preparation from an unsaved creation Party reached the
requested outdoor map with 20 chests (1/1). This establishes a map-load route,
not a gameplay transition or save/reload cycle.

The first direct load exposed an MM6-only map-table mistake: the encounter
loader used display labels in place of authored monster stems. The corrected
column selection passed a synthetic table check and the existing seven-process
first-map v5 cycle. The full-map encounter audit then found one authored stem
without corresponding ranked monster entries. Source-backed reachability
checks showed that encounter family has neither a positive random spawn
chance nor a reference from the map's actor spawn indices. The all-67 map
headless gate therefore validates every *reachable* encounter family against
the loaded monster/descriptors (1/1); production lookup remains strict and
does not invent a special name alias.

A separate owner-only, test-only `candidate-v5-oute2.ec6` slot is reserved;
its atomic isolation check passed with the rest of the atomic suite (18/18).
It does not write a candidate yet. The next gate needs an oute2-specific
native DDM/actor/EVRT source anchor, exact live-owner capture, atomic write,
fresh-child pre-event replay, first input and second save/reload. The existing
first-map capture API remains first-map only. A direct map load and bounded
source audits cannot establish cross-map campaign play.

See [v5 runtime ownership](MM6_F4_EC6_V5_SPRITE_RUNTIME.md),
[local status](LOCAL_STATUS.md), and [the F4 path](plan.md).
