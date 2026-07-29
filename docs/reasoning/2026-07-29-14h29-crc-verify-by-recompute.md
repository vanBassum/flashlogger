---
id: 2026-07-29-14h29
date: 2026-07-29
time: "14:29"
title: CRC verified by recompute-and-compare (commit / edit / read semantics)
supersedes:
---

Integrity is verified by always **recomputing** the CRC over the current data
and comparing to the stored CRC — the stored value alone is never assumed.
`R == S` → intact & unedited, and this works for *any* stored value including
`0` and `0xFF`, so no genuine CRC output is a "magic" collision (this fixes the
data-loss worry I raised about the extremes). Only a **mismatch** is
interpreted by the stored value: `S = 0xFF` → CRC was never back-filled
(torn/uncommitted write) → discard; `S = 0` → record was cleared on a
deliberate edit (clear-bits, so the library does it app-agnostically on any
overwrite of a written field) → it was valid at first write but changed since →
trust, don't verify current; any other `S` → a real CRC was written and the
data changed *without* going through the edit path → corruption. Commit = CRC
back-filled last (a record isn't durable until the call returns); edit = clear
CRC to `0`. So the CRC bytes carry incomplete / edited / verify, decided by
recompute, not by assuming the stored value — and no extra "was-edited" bit is
needed. Residual, all ~1/2^width and inherent to CRCs, accepted: an uncommitted
record whose partial data happens to recompute to `S` reads as valid; a
genuine-`0` record that later bit-rots reads as "edited" (out of contract —
`S=0` means current integrity isn't promised). Rejected: a separate edit/status
bit (unneeded); excluding a mutable field from the CRC (would force the library
to know which field is app-mutable). This resolves the write-verification open
question.

Decision: verify by recompute-and-compare; on mismatch `0xFF`=torn, `0`=edited(trust), else=corrupt; commit=back-fill-last, edit=clear-to-`0`.
