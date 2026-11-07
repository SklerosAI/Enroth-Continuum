# MM6 gameplay RNG save candidate

Status, 2026-11-07: detached `RNGS` version-one section and concrete gameplay
engine snapshot/restore seam. It is **not** in the strict EC6 v2 manifest and
is not called by live MM6 SaveLoad. The linked RNGS/GNPC suites passed **5/5**
in integration; an MM7 headless control passed **1/1** after the shared RNG
change.

## State and wire contract

The engine has two random endpoints: `grng` affects gameplay and `vrng` is for
visual/audio randomness. `EngineRandomComponent` owns a Mersenne Twister and a
sequential engine for each endpoint, selects one type and may expose gameplay
draws through a tracing wrapper. The snapshot reads the selected underlying
gameplay engine. It does not consume a draw or change the selected type.

The Mersenne Twister snapshot uses its standard stream state representation
under the classic locale, bounded to 8,192 bytes. Restore parses into a
temporary engine and assigns only after a complete valid parse, so malformed
input leaves the old stream unchanged. The sequential engine retains its
32-bit counter. A restore through `EngineRandomComponent` refuses a different
selected type; the tracing wrapper still points at the same underlying engine.
`RNGS` v1 encodes type plus exact MT text or the sequential counter, validates
the state on decode, and rejects wrong title, missing/optional/duplicate,
oversized, truncated or trailing section data. The generic container knows
the tag; EC6 v2's exact nineteen-tag decoder continues to reject it. `vrng`
is intentionally outside this gameplay capsule.

Focused synthetic tests cover MT continuation after prior draws, failure-
atomic bad-text restore, sequential continuation, malformed section refusal,
wrong-title refusal and the current v2 extra-section boundary. The changed
source/test translation units passed AppleClang syntax checks and targeted
cpplint; a standalone compiled MT continuation probe passed. Linked focused
checks passed in integration; a fresh-child save/reload of RNG continuation
has not.

## Publication order still open

A correct exact snapshot is necessary but not enough for MM6 reload. The
current map loader regenerates NPC rows before `onMapLoad()`, and that event
path may itself consume gameplay randomness. Restoring a saved state after
`onMapLoad()` would resume later draws but cannot undo earlier effects or the
generated identities that events observed. A future versioned save needs a
source-backed pre-event row/RNG publication order or a tested alternative
entropy policy. The `GNPC` section is the separate identity candidate; the
chest LCG is not `grng`. Neither section opens normal SaveLoad, QuickLoad or
Atlas Play.

See [generated NPC identity](MM6_F4_GENERATED_NPC_IDENTITY_SAVE.md) and
[the full session owner gap](MM6_F4_V2_FULL_SESSION_OWNER_GAP.md).
