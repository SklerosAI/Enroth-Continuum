# MM6 transition picture ID 8 review

Codex read-only review on 2026-11-08, for Claude's C1 integration decision.

The accepted-original MM6 executable has an eight-entry transition icon table matching all eight names used by the C1 candidate. Each of those eight names is present in the retained icon index. The next table word does not identify a ninth file-backed string or one of those eight names. Two active event records nevertheless pass picture ID 8.

The call-site check now proves **one-based** IDs: both original transition icon consumers select the table entry at `picture ID - 1`. ID 8 selects the eighth verified icon, `outside`; the earlier conclusion that it had no safe mapping is superseded. C1 must use IDs 1–8 with this offset. ID 0 is a separate no-picture sentinel and does not identify an icon. The dialog's Yes action remains disabled for the independent transaction-safety reason. This review records only the evidence class and conclusion; no executable bytes, private paths, or asset inventory are included.
