# MM6 first-map party-flying event

Status: bounded F4 read-only event comparison, 2026-11-06. No first-map timer cast or visible flight behavior has been observed.

The retained first outdoor script registers a timer whose reachable branch compares raw variable ID 222 with numeric operand 0 after a quest-bit condition. When that comparison succeeds, the script proceeds to a spell cast. The [MM6 event editor's variable table](https://github.com/angeld29/mm8leveleditor/blob/master/src/org/gamenet/application/mm8leveleditor/data/mm6/fileFormat/EventFormatMM6.java) labels ID 222 `PARTY_FLYING`, and [MMExtension](https://grayface.github.io/mm/ext/ref/) names it `Flying`.

The F3-hash-matched original MM6 executable (SHA-256 `b804fda03f56c38cfeecd1a620ac97c2eeebe916e3048189520b34d43688e3e6`) was read locally. Its variable Compare dispatcher at `0x43fe40` uses raw ID 222's jump-table entry to reach `0x43ffda`. That branch reads a party flight flag and a positive 64-bit buff time, returns true only when both are set, and never reads the numeric event operand. The current engine's inherited MM7 `VAR_IsFlying` comparison uses the same two conditions, but its numeric ID is different; MM6 must dispatch raw 222 explicitly. These observations establish the narrow Compare result, not the downstream spell's MM6 effect.

`mm6::comparePartyFlying` checks raw ID 222 and evaluates the party's `bFlying` flag with `PARTY_BUFF_FLY.Active()`. The interpreter calls it once for MM6 ordinary and dialogue-topic Compare; MM7 still uses its inherited path. The original rule ignores the numeric operand, including the observed zero. MM6 writes to ID 222 and other unresolved high IDs remain guarded. No party state is changed by this comparison.

Pure tests cover all flag/buff combinations, operand independence and wrong-ID rejection. A separate headless GameTest parses a synthetic MM6 Compare and exercises false/true dispatch with scoped restoration of both party fields. Source and test syntax, native helper execution, style and diff checks passed; linked test and first-map timer observation remain pending integration. No original executable bytes, disassembly or proprietary EVT records are committed.

See also: [first-map month event](MM6_F4_FIRST_MAP_MONTH_EVENT.md), [event variable dispatch](MM6_VARIABLE_DISPATCH_F2.md), [first-map preflight](MM6_F4_FIRST_MAP_PREFLIGHT.md).
