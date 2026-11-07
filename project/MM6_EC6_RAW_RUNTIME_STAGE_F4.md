# MM6 EC6 v2 detached raw owner stage — F4

Status: **synthetic, detached runtime candidate only** (2026-11-06). The coordinated nineteen-section raw EC6 v2 schema is decoded into fresh MM6 physical inventory, cursor and shop owners after a complete source map validation pass. `SaveLoad.cpp`, the normal slot loader and live world publication remain guarded.

## Input and validation

`decodeRawSaveWithActiveMapCounts` reuses the known-save map validator's one 67-map/base loop. The caller supplies the exact expected MM6 delta names, a trusted MM6 base-map reader and an MM6 item-template verifier. Before return it checks the full map set, decodes every base/delta pair, verifies ground and fixed chest item templates, and decodes the active `CHST` sidecar against the same base and record shape. It returns an owned active map candidate; neither base blobs nor save spans are borrowed.

`stageRawTitleOwners` then checks the active raw owner references. Positive backpack/equipment indices are 1–138, unique per character and backed by a verified MM6 item template. Negative grid cells must point to a positive anchor. A present cursor and every nonzero physical shop record must also identify a verified template. Unreferenced inventory records and zero-ID physical shop records stay opaque and byte-exact. `RawHeldCursorOwner` is staged through its title-specific raw-v2 constructor. `ShopTitleState` carries the physical `SHOP` image and keyed `SCLK` deadlines together. A wrong title, missing verifier/base, malformed late map, invalid owner reference or rejected item prevents return of any owners.

`captureRawTitleOwners` overlays only detached owner copies into the decoded raw save and repeats item, section, map-name and all-base validation. It returns fields for a later `encodeRawSave` caller; it does not write files. The title requirement is repeated on capture. Both functions are free of live global assignment and profile I/O.

## Evidence and remaining gates

An isolated arm64 CMake build compiled the new runtime and test objects. A linked synthetic harness passed **7/7** tests: three new raw stage/capture cases plus four existing map-validation cases. The new cases cover high inventory slot and unreferenced record tails, raw cursor tail retention after clear, shop tail preservation, owner-edit capture, all-map acceptance, wrong title, wrong map set before reads, malformed last delta, absent base, active reference conflicts and item-template rejections. The existing private-backed 67-map test was not run in this isolated harness. No proprietary data entered Git.

This stage does not yet map all native item kinds to usable `CharacterInventory` entries, reconstruct the live map, stage beacon state or complete a simultaneous Party/world/UI/commerce commit. It does not migrate v1 files or open the production save UI. Those gates remain in [the transaction checklist](../.llm-wiki/MM6-F4-Save-Transaction-Checklist.md).
