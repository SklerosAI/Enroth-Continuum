# MM6 EC6 v3 exact manifest candidate

Status, 2026-11-07: detached v3 encode/decode only. The linked v3, v2
regression and ACTR suites passed **11/11** in integration. No atomic slot
writer/reader, menu route, game-loop publication or first-input admission
uses this version yet.

The [baseline actor audit](MM6_F4_BASELINE_ACTOR_SAVE_AUDIT.md) showed that
spawned-only SPAW cannot preserve original actors' runtime mutations. The
[active-actor decision](MM6_F4_ACTIVE_ACTOR_OWNER_DECISION.md) has now been
applied to this structural manifest: full-list ACTR replaces SPAW. This is
still **not** a durable gameplay save.

The exact required set has the existing nineteen v2 owners plus `ACTR`,
`GNPC` and `RNGS`: `PART CHAR QEST EFFT INVR HEAD TIME UIST VISU MAPS RESC
RSTS HELD BUFF HIRE RCVR CHST SCLK SHOP ACTR GNPC RNGS`. Every section is
required; missing, optional, duplicate and extra tags refuse. A canonical
zero-list ACTR remains present when no actors are active. The v3 decoder
checks ACTR framing and full-list slot zero through its bounded codec, GNPC and RNGS through
their own version-one codecs, and GNPC map identity against HEAD. It then
passes the nineteen old fields through the unchanged strict v2 raw decoder.
The container version is three; `RCVR` payload version three and GNPC/RNGS/
ACTR payload version one are independent version numbers. The v1/v2 readers
do not reinterpret a v3 slot, and the v3 reader rejects old slots.

This is an exact container shape, not complete gameplay admission. The
manifest alone cannot prove ACTR's baseline portion agrees with the active
native DDM, a nonzero no-respawn day, post-world actor/PID agreement, or
pre-event GNPC/RNG continuation. Those belong in the map/session stage and
fresh-child test. `MM6AtomicSaveFile` still has only explicit v1/v2 schemas;
a distinct v3 semantic preflight and atomic API is required before disk use.
The current rollback helper also hardcodes v1 validation and must be audited
before a v3 rollback route. No v2 bootstrap slot should be silently upgraded
or treated as playable v3.
