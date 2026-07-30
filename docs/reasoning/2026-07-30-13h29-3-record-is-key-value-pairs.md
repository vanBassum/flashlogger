---
id: 2026-07-30-13h29-3
date: 2026-07-30
time: "13:29"
title: A record is a list of key/value pairs, written straight through
supersedes:
---

One log entry carries several tagged values at once — temperature *and* humidity
— each pair becoming one Field, rather than one key with a long payload.
Reasoning: a log entry is naturally several distinct measurements, and tagging
each with its own key is what makes selective reads possible later ("give me key
7 from this record"); a single-key blob would force the reader to know the
payload's internal structure, which is exactly the knowledge the key is there to
carry. Long values still work by repeating one key across consecutive Fields, so
nothing is lost. The API grew into the streaming form — `createRecord()`, then
`field(key, value)` per pair, then `close()` — which writes each field straight
to flash with nothing buffered in RAM, matching both the no-allocation rule and
the crash-safe write order (marker first, data, CRC last). Rejected:
`append(key, data, length)` (one key per record — simplest to test, but the
reader then needs out-of-band knowledge of what's inside); a single
`append(pairs, count)` taking an array of pairs (needs the caller to marshal a
struct array, and buffers the whole record's worth of pointers for no gain over
streaming).

Decision: a record is several key/value pairs, streamed one field at a time straight to flash.
