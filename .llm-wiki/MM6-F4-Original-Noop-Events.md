---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
related: ["[[Source-MM6-Opcode-Side-Effects-F2]]"]
tags: [mm6, f4, events, original-executable]
---

# MM6 F4 Original No-Op Events

The F3-matched original MM6 executable advances past active opcode values 20, 27 and 28 without an effect.

The original interpreter reads the opcode byte, subtracts one and indexes a 43-entry dispatch table. All three entries point to the same ordinary next-record path used by unsupported commands: that path increments the sequence and record cursor and reads no operand or state owner. The local executable hash was checked against the accepted F3 identity before this read-only trace. The pinned editor's `ModifyItem`, `RndPassword` and `RndAnswer` labels are hypotheses about a different or intended behavior; they do not override this executable's dispatch.

The title-specific parser already preserves their exact bounded payloads, and all 68 active retained EVT resources parse. The MM6 executor now advances for these three opcodes. A foreign-title instruction still rejects. This closes the previously named executor exception for seven active opcode-20 records and one record each at 27 and 28; it does not prove that their containing event paths are reachable or that all other effects in those scripts work.

If a later MM6 executable or patch changes the dispatch entries, redo the identity check and revisit the semantics before loading its events. The current normal MM6 SaveLoad and chest guards remain closed; no campaign play is accepted.

See also: [MM6 F4 Offline Seams](MM6-F4-Offline-Seams.md), [MM6 Opcode Side Effects F2 Source](Source-MM6-Opcode-Side-Effects-F2.md), [MM6 F4 First Map Preflight](MM6-F4-First-Map-Preflight.md).
