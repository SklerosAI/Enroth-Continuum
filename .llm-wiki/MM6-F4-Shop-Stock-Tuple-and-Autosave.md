---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Shop-Refresh-PE-Trace]]", "[[MM6-F4-World-Save-Runtime]]"]
tags: [mm6, f4, save, shop, stock, travel]
---

# MM6 F4 Shop Stock Tuple and Autosave

The [source packet](../project/MM6_F4_SHOP_STOCK_TUPLE_AND_AUTOSAVE.md) records an original stock-read trace and the current cross-map autosave dependency. Exact PE and retained-house details are in ignored local evidence.

The original shop menu selects two distinct stock banks. Each bank uses the current house key, a 12-position physical stride and a zero item ID to suppress a displayed slot. A selected slot uses the same address formula. The original guild path has a corresponding 12-position stride and zero-ID read. The later [purchase-write trace](MM6-F4-Shop-Purchase-Write-Gate.md) proves all three successful bank paths copy 28 bytes and clear source bytes 0–24, while ordinary/special labels, full item field meaning and adjacent family-boundary service lifetime remain unproved. It also records the one-row layout conflict with a pinned research editor. A detached physical `SHOP` codec now preserves all 1,440 raw records, but no live MM6 stock owner or required known-save section exists; [SCLK](MM6-F4-Shop-Guild-Clock-Save.md) covers refresh deadlines only.

In current source, `startMapTransition()` autosaves on a different destination map before it sets the pending transition. `autoSave()` reaches the MM6 `saveGame()` throw before the write-error catch. Thus this helper cannot complete an MM6 cross-map move while live save remains guarded. This is a source route finding, not a launched gameplay observation. A complete `.ec6` transaction or a separately proved transition policy is needed; the save guard stays closed.

The ignored focused static checker passed the accepted executable hash, branch and zero-slot checks, boundary math, Party save span and autosave call order. No C++ build was needed.

See also: [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), [Shop Refresh PE Trace](MM6-F4-Shop-Refresh-PE-Trace.md), [World Save Runtime](MM6-F4-World-Save-Runtime.md).
