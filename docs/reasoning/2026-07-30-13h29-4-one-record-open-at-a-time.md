---
id: 2026-07-30-13h29-4
date: 2026-07-30
time: "13:29"
title: One record open at a time — the newcomer is refused, closing is RAII plus explicit
supersedes:
---

Because `field()` writes straight to flash with no RAM buffering, two records
open at once would interleave their fields on flash and neither would be
readable. So a record must be closed before the next opens, and while one is
open `createRecord()` hands back a refused handle whose `field()` reports
`RECORD_ALREADY_OPEN`. This was chosen over the first sketch, where opening a
new record silently invalidated the *older* handle: refusing the newcomer means
nothing a caller is holding ever dies underneath it, and the failure lands on
the call that caused it rather than on innocent later use. A record ends via
`close()` **or** the destructor, deliberately both — the destructor is the safety
net for a forgotten close, and the explicit call is the only one that can
*report* a failure, since closing will eventually back-fill the CRC and a
destructor has nowhere to put an error. `close()` drops its log pointer, making
it idempotent so the destructor after an explicit close is a no-op. Naming:
`createRecord()` over `WriteRecord()` (which read like it wrote a whole record
when it starts one) and over `openRecord()` (this always makes a new record, and
leaving "open" unclaimed keeps it free for opening an *existing* record once
reading exists); `close()` kept over `commit()` for handle-like familiarity,
though `commit()` describes the CRC back-fill more honestly. Rejected: the older
handle going stale (a live handle dying under the caller); buffering a record in
RAM so several could be open (unbounded record size, no allocation allowed).

Decision: one record open at a time, the second refused with `RECORD_ALREADY_OPEN`; a record ends by `close()` or destructor, and `close()` is idempotent.
