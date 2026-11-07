# MM6 raw held cursor section (detached F4 candidate)

The current required `HELD` v1 stores logical `Item` fields and drops the new `Item::mm6RawRecord` carrier. `MM6HeldItemSaveRuntime.cpp` captures scalars; its copy-stage reconstructs them after `Item::Reset()`. Thus `Party::setHoldingItem()` and `takeHoldingItem()` can transport the 28-byte carrier by value, but the current `.ec6` route cannot retain it. `KnownSaveFields` still requires v1 and the provisional `ITEM` section; normal MM6 SaveLoad remains guarded.

`MM6RawHeldItemSaveFields` is a detached v2 proposal. The required `HELD` payload is exactly 32 bytes:

| Offset | Bytes | Meaning |
| --- | ---: | --- |
| 0 | 1 | Version `2` |
| 1 | 1 | Presence `0` or `1` |
| 2–3 | 2 | Reserved zero |
| 4–31 | 28 | Exact original MM6 cursor `ItemRecord28` bytes |

A present record requires a structural source item ID in 1–580. An absent record requires the first 25 raw bytes to be zero, including item ID zero; the final three bytes remain opaque and round-trip exactly. The original cursor clear trace copies the whole record into the cursor on set and zeroes only bytes 0–24 on clear, leaving 25–27. A shorter empty marker would discard those tail bytes. The existing shared `Item::Reset()` does not itself establish this native byte policy, so a future raw cursor owner must retain the absent record separately from the visible runtime `Item`.

The codec validates MM6 title, required status, unique section, exact length, version, presence, reserved bytes and the structural raw ID/cleared prefix. The whole `.ec6` container and section checksums are decoded before assignment to a caller-owned candidate. It does not infer item category, enchantment semantics, safe live projection, cursor image, click offset or pending action state. In particular, the v2 packet alone does not authorize a potion, special item or other currently unsupported raw record for live use.

## Migration and live transaction gate

The existing v1 and v2 codecs reject each other's version. The final manifest must replace `ITEM` v1 with a proven raw inventory owner and `HELD` v1 with this raw cursor owner in one explicit schema revision. A present v1 cursor cannot be losslessly converted to v2 because v1 omitted source bytes. A future migration may accept an empty v1 only if the project deliberately defines the missing trailing bytes as zero and records that as a conversion, not a recovered native value. Otherwise reject old partial candidates before any live mutation.

For a live route, capture must read the exact raw cursor owner alongside `pPickedItem` and validate their agreement under a stable idle-action boundary. Staging must check loaded MM6 item templates and any supported raw-to-runtime projection on copies, then join Party, inventory, world and all other mandatory owners. The existing `HeldCursorRestorePlan` intentionally resets the transient click offset after successful load; this policy can remain. Mouse icon publication belongs after the complete all-owner commit, outside codec and staging. No live save/load route is added by this candidate.

## Verification

Three synthetic linked GoogleTest cases cover present and absent-tail exact round trips, version/presence/shape/required/source-ID/prefix/CRC failures before destination mutation, foreign-title rejection, and explicit v1/v2 incompatibility. The detached test binary passed 3/3 without a full engine build or private game data. Live owner, native save and visual checks remain outstanding.
