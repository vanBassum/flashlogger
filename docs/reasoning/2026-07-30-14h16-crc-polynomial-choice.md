---
id: 2026-07-30-14h16
date: 2026-07-30
time: "14:16"
title: Which CRC recipes to use, and where the choosing happens
supersedes:
---

Picked well-known standard CRCs rather than anything homemade, so a flash dump
can be checked with any tool off the internet — useful when a device in the field
looks wrong and you only have the bytes. For the 1-byte case, `0x2F` instead of
the more common `0x07`, because it catches more errors on short data and short is
all this format ever has. The 2-byte one (`0x1021`) was already in the code for
the field header. The 4-byte one (`0xEDB88320`) is the same one zip files use.
The CRC code itself stays dumb — it only crunches bytes — so every choice (which
width, what gets covered) lives in the record layer instead. It covers every
field of the record, keys included, so a corrupted key is caught too. Rejected:
the more common `0x07` (weaker on short data); tweaking the CRC when it happens
to come out as `0xFF` or `0` (not needed — we always recompute and compare, so
those values aren't special, and tweaking would throw away detection power for
nothing); homemade or table-driven versions (can't be cross-checked with other
tools, and tables cost flash).

Decision: standard CRCs — `0x2F`, `0x1021`, `0xEDB88320`; the CRC code stays dumb and the choices live in the record layer.
