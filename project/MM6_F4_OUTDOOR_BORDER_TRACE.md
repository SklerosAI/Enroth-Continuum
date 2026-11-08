# MM6 outdoor border travel — F4 original-route trace

Status: **bounded original route proof and detached calculator; live adapter remains gated** (2026-11-07). A read-only trace used the staged original executable reconciled to its accepted F3 hash and the retained 67-map manifest. Exact executable addresses, map names and assertions remain in ignored owner-local research. No original installation was opened.

Update, 2026-11-07: a later executable read corrected the zero-food claim below. The trip's time advance always rests and heals, then rests again with food, so a starving party arrives healed but Weak. The confirmation transaction now exists detached in [`MM6OutdoorBorderConfirmation.h`](../src/Engine/MM6/MM6OutdoorBorderConfirmation.h), with `restAndHealDetachedParty` and the four proven border hirelings. `getTravelDestination` still returns no route for MM6.

## Proved route shape

The original outdoor crossing path checks both party axes against the map edge. Its helper derives a neighboring outdoor filename from a five-column, three-row coordinate encoded in the current filename. It moves the column for an east or west crossing and the row for a north or south crossing, rejects candidates outside the grid, and can change **both** at a corner. All 15 outdoor entries in the accepted retained map manifest occupy this grid. This is a geometric filename route, not the inherited MM7 destination table.

Before showing travel, the caller also tests the low byte of Party flags and clamps the party back inside the edge if the helper fails or that state disallows travel. A later accepted-PE trace resolved its three checked bits as water damage, airborne and standing on water, with current `PartyFlag` values and original movement writes supporting that mapping. Burning and unrelated bits are not in this offer mask. The original confirmation branch separately refuses nonzero flying state, rechecks the neighboring identity, computes a baseline five-day trip reduced by several hireling checks with a one-day floor, advances game time, and wraps each out-of-range position by one full map width. It does not repeat the offer mask. The arrival thus preserves the party's edge offset in the observed branch. It does not select an MM7 named map start point.

The current MM6 `getTravelDestination()` still returns no route, correctly preventing MM7 table access. This trace does not authorize replacing that guard: full confirmation dispatch, later state changes, corner cases and a nonzero native arrival sample need proof. In addition, current cross-map `startMapTransition()` autosaves before scheduling the move while MM6 `saveGame()` is guarded, so a route adapter alone would not make travel work.

A later confirmation trace resolved the ordinary time/food/rest branch and coordinate wrap. The original skips full rest/heal at zero food, unlike the inherited MM7 foot handler; it refreshes Weak timestamps even when already present, independently wraps crossed axes and recalculates target ground height. The distinct original cancel and rejection paths clamp both axes, close travel UI and leave time, food and map unchanged. The [autosave transaction packet](MM6_F4_BORDER_AUTOSAVE_TRANSACTION.md) records the safe order before enabling live travel.

## Detached implementation

[`MM6OutdoorBorderTravel.h`](../src/Engine/MM6/MM6OutdoorBorderTravel.h) now calculates a pure geometric candidate for a validated lowercase 5×3 map identity. It changes each crossed grid axis independently, wraps coordinates by one map width, computes the four observed hireling reductions with a one-day floor, and rejects invalid coordinates or an arrival still outside bounds after one wrap. It reads and changes no Party or global state. The assessment always returns `mayOffer() == false`; cardinal trips are refused for unverified party eligibility and special edges, while corners also lack a native acceptance sample. The caller must still verify archive/map identity. Four focused synthetic route checks and three existing transition-intercept checks passed **7/7** on the integration build. This is no live route, travel UI, confirmation, autosave or playable campaign.

[`MM6OutdoorBorderEligibility.h`](../src/Engine/MM6/MM6OutdoorBorderEligibility.h) now models the three-bit offer mask and separate zero-flying confirmation check as pure MM6-only predicates. It does not connect either to live travel. Four focused eligibility tests, four route tests and three transition-intercept controls passed **11/11** on the integration build. The original neighbor helper and immediate caller showed no per-map exceptional edge in this traced path; broader confirmation and arrival behavior remain open.

## Next bounded steps

1. Check corner crossings and arrival positions against owner-local native state, and confirm the detached calculator's edge and travel-day behavior against the original executable.
2. Trace the rest of confirmation dispatch and later party-state changes before offering travel.
3. Wire the result into MM6 outdoor movement only after the route and cross-map save policy are complete; leave MM7 and MM8 paths untouched.

No fog or underwater predicate was found in the traced neighboring-name helper or immediate edge caller. This narrow trace does not establish MM6 weather or underwater rules; those remain a separate outdoor task. The earlier ignored focused checker passed executable-hash, helper/caller/confirmation shape, 15-map grid and current MM6 guard assertions; the later detached C++ checks are described above.
