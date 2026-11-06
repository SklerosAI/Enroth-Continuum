# MM6 indoor runtime projection (F4)

Status: **offline implementation, not a played or saved campaign**. This packet describes the MM6-only BLV/DLV adapter in `MM6IndoorSnapshot.*` and `Indoor.cpp`. It uses the F3-matched retained owner extracts without opening the original installation again. No proprietary bytes or per-map inventory are tracked here.

## Source and field mapping

The [MM6 IndoorFace reader](https://github.com/angeld29/mm8leveleditor/blob/master/src/org/gamenet/application/mm8leveleditor/data/mm6/indoor/IndoorFace.java), [Room reader](https://github.com/angeld29/mm8leveleditor/blob/master/src/org/gamenet/application/mm8leveleditor/data/mm6/indoor/Room.java), [LightSource reader](https://github.com/angeld29/mm8leveleditor/blob/master/src/org/gamenet/application/mm8leveleditor/data/mm6/indoor/LightSource.java), and [Door reader](https://github.com/angeld29/mm8leveleditor/blob/master/src/org/gamenet/application/mm8leveleditor/data/mm6/indoor/Door.java) provide independent offset leads. The retained decoded BLV/DLV corpus verifies section sizes, exact ends, counts and reference ranges; it does not prove gameplay semantics.

- MM6's 80-byte face corresponds to the MM7 face after its leading 16-byte floating-point plane. The adapter copies the 80 bytes, derives a float plane from the retained fixed-point plane, and checks vertex and face-extra references. Variable face arrays use six `(vertex count + 1)` short arrays, as seen in the retained files.
- MM6 room records have the same 116-byte grouping used by the current renderer. The adapter checks every variable room span, face, decoration and light reference before the renderer reconstructs spans. A zero first-BSP index with no BSP nodes is normalized to the renderer's `-1` sentinel only in the projection; the original bytes remain in the wire snapshot.
- MM6 decorations contain a 28-byte record and 32-byte name. The adapter maps the event variable without MM7's 75-offset rule. Twelve-byte MM6 lights become white renderer lights with the MM6 radius, attributes and brightness. Twenty-byte spawn points gain a zero group field for the existing runtime shape.
- The DLV adapter uses the shared MM6 548-byte actor, 100-byte ground-item and 4,204-byte chest projections. It checks all 200 fixed 80-byte door records and their variable array references before reconstructing door pointers. The 875 visibility bytes use the existing outline bit order.
- Reset count and last reset day are read as two bounded 32-bit words. The loader uses the same provisional first-visit/timed-reset selection as the outdoor MM6 seam and retains visibility on timed reset. The decoded DLV, including the 200 persistent bytes, 56-byte tail and unresolved record fields, stays in `IndoorLocation::mm6DeltaBytes` for the later writer.
- The two known original-matched BLV entries with invalid zlib trailers have a filename-scoped raw-DEFLATE fallback. It still requires the wrapper length, expected decoded length and full BLV structure to validate. Every other map uses strict archive decoding; DLV decoding always stays strict.

## Offline verification

The native unit target builds on arm64. `MM6IndoorSnapshot.*` has four focused tests: synthetic round trip and opaque mutation, minimal projection, malformed sections, and all **52** retained F3-matched BLV/DLV pairs. The retained test checks wrapper decode against the decoded fixture, projected face/room/light/decoration and actor/item/chest/door counts, byte-exact re-encode, opaque-region mutations, a rejected invalid door reference and the two-map fallback boundary. It does not call `IndoorLocation::Load` or start MM6.

## Remaining F4/F5 work

1. The MM6 save writer must patch changed actors, items, chests, doors, visibility and reset fields into the retained wire before writing `.ec6`; retaining raw bytes alone does not persist live changes. The 200 persistent bytes and 56-byte tail still lack a verified MM6 event/clock mapping.
2. Door motion and geometry repair need a live closed/open/mid-motion check. The current adapter binds arrays and uses the projected base face planes, but does not independently repeat the MM7 normal repair after a moving door changes vertices.
3. Shared record projection still has unresolved MM6 actor buffs, spell/resistance correspondence and chest grid semantics. The provisional refill timing and reset-state selection require F5 observation.
4. F5 must inspect rendered rooms, lights, decorations, faces, actors, chests, doors, collisions, travel, re-entry, save/relaunch and MM7 isolation. A build and 52-pair parser pass are not a playability claim.
