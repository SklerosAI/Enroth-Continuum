# MM6 `.ec6` container foundation — F4

Status: **offline container codec, 2026-11-06; no runtime save integration**. The standalone `engine_mm6_save_container` library writes and validates a title-tagged, versioned byte container. It does not serialize MM6 party, character, event or map state, list save slots, write files, or load into game globals. It is separate from MM7's `SaveGame_MM7` path. No partial MM6 build was launched.

## Version 1 envelope

All integers are unsigned little-endian. The fixed 24-byte header has eight signature bytes (`EC6SAVE` followed by NUL), 16-bit version `1`, zero 16-bit flags, a 16-bit section count, a zero 16-bit reserved field, 32-bit exact total size and CRC-32 of the bytes after the header. A section has a four-character uppercase ASCII/digit/underscore tag, a 16-bit required bit, zero 16-bit reserved field, 32-bit payload length and CRC-32 of its payload, followed by its bytes. CRC-32 uses the standard reflected polynomial. The decoder validates the whole stream before returning a temporary `SaveContainer` value; it has no references to live engine state.

The limits are 64 MiB per file, 32 sections and 32 MiB per section. The codec rejects zero/duplicate sections, malformed tags, unsupported version, wrong title, reserved bits, truncated or trailing data, oversized lengths, checksum mismatches and unknown **required** sections. Unknown optional sections remain opaque for a round trip. The twelve recognized tags correspond to the [F3 logical state families](MM6_F3_SAVE_BOUNDARY.md); their field layouts and mandatory-presence rules are **not yet defined**. The container encoder accepts structurally valid section tags so a later version can preserve optional extensions; the decoder is the compatibility boundary.

## Validation and next lock

The new unit source covers round-trip with an optional unknown section, wrong title, future version, truncation, corruption, trailing bytes, unknown required section, duplicate tag and empty container. A direct clang C++23 standalone harness compiled the real codec and passed round-trip plus six rejection cases. After integration, the Mac CMake unit target built and its three focused tests passed; the separate `MM6.app` target also built. No save slot or game state was written.

Before this becomes a working `.ec6` save, F4 must lock every runtime field's section, type, bound, default and version policy from the private logical manifest; require the complete mandatory set; implement all encoders/decoders and 67 world-delta round trips; validate temporary state before any global mutation; and add same-directory atomic write/recovery plus title-specific slot listing. F5 must then save, quit, relaunch and inspect state in the graphical game. No native `.mm6` import is implied.
