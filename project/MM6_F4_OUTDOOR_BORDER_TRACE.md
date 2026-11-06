# MM6 outdoor border travel — F4 original-route trace

Status: **bounded original route proof; runtime adapter remains gated** (2026-11-06). A read-only trace used the staged original executable reconciled to its accepted F3 hash and the retained 67-map manifest. Exact executable addresses, map names and assertions remain in ignored owner-local research. No MM6 process or original installation was opened.

## Proved route shape

The original outdoor crossing path checks both party axes against the map edge. Its helper derives a neighboring outdoor filename from a five-column, three-row coordinate encoded in the current filename. It moves the column for an east or west crossing and the row for a north or south crossing, rejects candidates outside the grid, and can change **both** at a corner. All 15 outdoor entries in the accepted retained map manifest occupy this grid. This is a geometric filename route, not the inherited MM7 destination table.

Before showing travel, the caller also tests a byte of party state and clamps the party back inside the edge if the helper fails or that state disallows travel. The bit meanings and all exceptional edge permissions are **not yet typed**. The original confirmation branch rechecks the neighboring identity, computes a baseline five-day trip reduced by several hireling checks with a one-day floor, advances game time, and wraps each out-of-range position by one full map width. The arrival thus preserves the party's edge offset in the observed branch. It does not select an MM7 named map start point.

The current MM6 `getTravelDestination()` still returns no route, correctly preventing MM7 table access. This trace does not authorize replacing that guard: the party-state eligibility bits, confirmation state, corner cases and a nonzero native arrival sample need proof. A detached candidate calculator without those checks could present a trip the original refuses. In addition, current cross-map `startMapTransition()` autosaves before scheduling the move while MM6 `saveGame()` is guarded, so a route adapter alone would not make travel work.

## Next bounded steps

1. Trace the original party-state bit writers/readers and the confirmation branch to identify eligibility, including airborne, water or other edge conditions by evidence rather than MM7 labels.
2. Check corner crossings and arrival positions against owner-local native state. Preserve coordinate offsets and the original travel-day reductions in a detached MM6 route result with late-invalid/no-mutation tests.
3. Wire the result into MM6 outdoor movement only after the route and cross-map save policy are complete; leave MM7 and MM8 paths untouched.

No fog or underwater predicate was found in the traced neighboring-name helper or immediate edge caller. This narrow trace does not establish MM6 weather or underwater rules; those remain a separate outdoor task. The ignored focused checker passed executable-hash, helper/caller/confirmation shape, 15-map grid and current MM6 guard assertions. No C++ code changed and no native build was needed.
