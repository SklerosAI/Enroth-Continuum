---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
related: ["[[MM6-F4-Indoor-Facet-Events]]", "[[MM6-F4-Save-Transaction-Checklist]]"]
tags: [mm6, f4, events, indoor, saves]
---

# MM6 F4 Indoor Face Overrides

An optional, versioned IFOV section provides a detached MM6-only fork extension for indoor face changes that the current DLV writer does not save.

IFOV is keyed by canonical `.blv` map name and bounded face index. It stores only attributes and bitmap identity changed from the deterministic pre-event base, plus a base-face fingerprint. That base includes fixed load-time repairs such as `repairClickableFaces`; otherwise they would become spurious overrides. The original BLV and DLV bytes stay untouched. The codec is outside the eighteen required known-save sections and is not wired to live SaveLoad. The underlying original MM6 save rule remains unknown.

The defined fork order is BLV/DLV reconstruction, map `OnMapReload`, then saved IFOV fields. Saved changed fields win; untouched fields retain script effects. Detached capture, full-container staging and application reject malformed or duplicate entries, a different base, and late failure before destination mutation. Three focused linked arm64 unit tests passed. Live texture identity capture, animated faces, retained-map `.ec6` replay and graphical acceptance remain open.

The v2 post-world audit found no safe final-face shortcut: `onMapLoad()` runs
`OnMapReload` before the hook, and the strict nineteen-section candidate carries
neither observed IFOV nor a verified pre-event face base. Final faces alone
cannot separate script edits from unsaved player edits. See the [raw v2
admission packet](../project/MM6_F4_RAW_V2_BEAC_IFOV_ADMISSION.md).

A later detached IFOV writer and separate fresh-child reader passed against
retained private indoor BLV/DLV data. A scoped headless world load also captured
the real indoor face baseline immediately before `OnMapReload` (**1/1**). This
supersedes the earlier lack of a live pre-event base. The checks still do not
write or publish a live indoor `.ec6` slot; indoor gameplay replay,
texture/animation behavior and graphical acceptance remain open.

A strict detached EC6 v6 indoor shell stages raw v2 with GNPC/RNGS/EVRT,
native actor/sprite/decoration anchors and optional nonempty IFOV. Its first
shape accepted 25 sections without changed faces or 26 with IFOV and refused
gameplay admission. The initial focused container, v6 stage and atomic suite passed
**25/25**. A fresh disposable v2 seed and separate real-data indoor v6 stage
then passed **1/1** each. The stage used the actual pre-event face baseline,
constructed a candidate only in memory, checked its native anchors and kept
seed bytes unchanged. The earlier test-thread stack crash was resolved by
staging on the established game thread. A later disposable three-process
sequence passed v2 seed, atomic indoor v6 writer and independent fresh-child
reader at **1/1** each. The reader rebuilt the pre-event face base, re-staged
the exact candidate and kept gameplay admission false. Live owner publication,
first input and graphical acceptance remain open.

The current v6 shape requires ordered `IDRT` runtime decoration ownership,
making its manifest 26 sections without IFOV or 27 with IFOV. The descriptor,
flags and event ordinal are bound to source BLV, EVRT and native IDCR slots;
descriptor zero is accepted only as the source-backed table sentinel. A
detached replay changes a controlled invisible bit while retaining all
immutable slots. Focused linked checks passed **6/6**, and a new disposable
three-process seed/writer/reader chain passed **1/1 each**. It still refuses
gameplay. A later MM6-only one-shot pre-event hook applies staged IDRT mutable
fields to the live indoor decoration list, and the fresh child requires exact
IDRT recapture after `OnMapReload`. The disposable v2 seed, v6 writer and
reader passed **1/1 each**, with focused linked v6 checks **7/7**. The
test-only reader stages the candidate's chest owner under the existing scoped
map-load gate; production chest loading and SaveLoad stay guarded.
Actor/sprite owner publication and first input remain open.

The current v6 manifest also requires `ISRT` full-list indoor sprite runtime
state, so it has 27 required sections without IFOV or 28 with IFOV. Exact
native `ISCR` bytes anchor the source prefix; only descriptor-zero free slots
may be reused, while appended sprites use canonical synthetic images and
exact runtime overlays. A v6-only item scalar overlay retains fields omitted
by inherited snapshots without changing v5 outdoor SPRT or raw MM6 item
provenance. Focused linked v5 sprite/v6 indoor checks passed **12/12**, and
the disposable seed/writer/reader chain passed **1/1 each**. An MM6-only
one-shot pre-event scope now publishes the full saved sprite list; the fresh
child recaptures exact ISRT after `OnMapReload`, before input. Focused v6
checks passed **10/10**, and the three-process chain passed **1/1 each**
again. Actor runtime ownership and gameplay admission remain gated.

V6 now also requires `IART` exact indoor actor runtime over the `IACR`
native prefix, including runtime actor names; its manifest has 28 sections
without IFOV or 29 with IFOV. The v5 outdoor ARTR wire is unchanged.
Focused v5 actor/v6 indoor checks passed **14/14**, and a disposable seed,
first-map baseline and indoor writer passed **1/1 each**. The initial
pre-event fixture could not prove post-world equality because normal map
events generate NPC links after publication. A separate genuine post-world
writer now captures GNPC, RNG, IART, ISRT and IDRT after those events. A
test-only fresh-child scope publishes the saved NPC pool and actor list,
restores RNG and bypasses only the corresponding generated-NPC loop; exact
GNPC row bytes, RNG and IART/ISRT/IDRT recapture pass after `OnMapReload`.
Both disposable three-process chains pass **1/1 per child**, and focused
v6/GNPC/v3 units pass **18/18**. The candidate still reuses seed Party/title
owners, so complete indoor session validation, gameplay admission, first
input and production SaveLoad remain open.

A separate strict live-session v6 mode now captures the represented raw
Party/title/NPC/inventory/cursor/shop/chest/metadata/map owners at one real
post-world callback. Four separate headless children (seed, first-map v4,
indoor writer, fresh-child reader) pass **1/1 each**. The reader publishes and
recaptures all 19 raw sections and GNPC/RNG/EVRT/IART/ISRT/IDRT/IFOV after
`onMapLoad`. Native chest and door residuals remain strict, allowing only
door face-delta words deliberately reconstructed from the loaded face base.
At that checkpoint this was one indoor map with a null renderer; first input,
post-input save, production SaveLoad, other maps and graphical acceptance remained open.

The later [v6 indoor input cycle](MM6-F4-V6-Indoor-Input-Cycle.md)
supersedes that limit for a **test-only disposable fixture**: a genuine
live-session candidate captures Party/title and all represented raw owners,
then survives real-loop turn/walk, an exact post-input write and independent
fresh-child replay. Five headless children passed 1/1 each. The stage's
production gameplay-admission flag remains false; normal SaveLoad and a
graphical indoor/campaign check are still open.

See also: [MM6 Indoor Facet Events](MM6-F4-Indoor-Facet-Events.md), [MM6 V6 Indoor Input Cycle](MM6-F4-V6-Indoor-Input-Cycle.md), [MM6 F4 Offline Seams](MM6-F4-Offline-Seams.md), and [implementation packet](../project/MM6_F4_INDOOR_FACE_OVERRIDE_SECTION.md).
