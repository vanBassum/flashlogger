---
id: 2026-07-29-14h11-4
date: 2026-07-29
time: "14:11"
title: Crash-safe append — marker first, CRC back-filled last; cursor found structurally
supersedes:
---

Direction (not locked). A record is written marker-header-first (`key=0x01`,
`value=0xFF` placeholder), then its data fields, then the header value is
back-filled with the CRC. Reasoning: writing over `0xFF` is always legal on NOR
(only clears bits), so back-filling the CRC (`0xFF`→value) — even re-writing the
whole header field a second time — is always allowed, and FieldStore supports
it as-is (each write independently read-back-verifies against its own intended
value). Marker-first makes a torn record self-identifying (it has a start), so
recovery skips it and resumes cleanly; if the marker were written last, torn
data would have no owner and mis-attach to the previous record. The CRC, when
enabled, doubles as the "record complete" signal. Crucially, finding the append
point after reboot is **structural** — scan to the frontier between written
fields and `0xFF` — and needs no CRC, so cursor-safety (always) and
data-integrity (optional CRC) are separable concerns. CRC is configurable
on/off, width sized to the value (1→8, 2–3→16, 4+→32); coverage is still open
(the flags-vs-CRC conflict, see write-verification backlog). Rejected:
header-at-end / header-fully-written-last (torn recovery can't tell log-end
from a missing header, and torn data blocks the append point); leaning on CRC
for crash-safety (breaks when CRC is off).

Decision: leaning marker-first + CRC-back-fill-last with structural cursor recovery — not locked; CRC coverage TBD.
