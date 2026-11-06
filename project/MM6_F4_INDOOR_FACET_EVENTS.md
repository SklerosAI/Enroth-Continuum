# MM6 F4 indoor facet event targets

Status: **MM6-only direct face-index dispatch for event opcodes 11 and 23; rendered and persistent effects remain unverified** (2026-11-06).

The staged executable was checked against the accepted F3 identity before a read-only interpreter trace. Original opcode 11 reads a 32-bit face index and a texture name; its handler acts only indoors and returns without mutation for a negative or out-of-range face index. Original opcode 23 reads a 32-bit face index, a 32-bit attribute mask and an on/off byte; its handler checks the same indoor bounds, then sets or clears those mask bits on that one face. Neither uses a facet cog/group ID. The F3-retained active EVT set has **215** opcode-11 records and **32** opcode-23 records. Exact operands and disassembly remain ignored locally.

The inherited `setTexture` and `setFacesBit` helpers match faces by `cogNumber` and can change several faces or none for one MM6 source index. The MM6 interpreter now selects `pIndoor->faces[index]` after the original-style indoor and index checks. It calls the current face texture resolver for opcode 11 and applies the raw bit mask to one face for opcode 23. MM7 continues through the inherited helpers. A synthetic headless test uses three faces with the same cog number to distinguish direct index from group targeting, and covers set/clear, texture, invalid index and nonindoor no-op. The integrated focused headless test passed **1/1**, full MM6 headless **49/49**, MM7 headless control **1/1**, and general non-MM6 unit gate **437/437**, all monitored headless runs without an on-screen window or focus change.

This corrects runtime target selection only. Original texture animation selection, redraw timing, changed-face persistence in `.ec6`, graphical appearance and campaign effects still need separate checks. The normal MM6 save and chest guards remain closed.

See [event framing](MM6_F4_EVENT_FRAMING.md), [F2 opcode side effects](MM6_OPCODE_SIDE_EFFECTS_F2.md), and [F4 build graph](MM6_F4_BUILD_GRAPH.md).
