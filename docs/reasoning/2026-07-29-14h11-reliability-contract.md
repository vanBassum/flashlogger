---
id: 2026-07-29-14h11-3
date: 2026-07-29
time: "14:11"
title: Reliability contract — structural robustness always, integrity opt-in via CRC
supersedes:
---

The library guarantees it never derails on any flash contents: iteration is
bounded, terminates, and never crashes or reads out of bounds — whatever the
bytes are. It does **not** guarantee data correctness without CRC: a corrupt
record, even a corrupted marker that mis-splits or merges records, is returned
as-is, and noticing is the user's job. CRC is opt-in and upgrades "I won't
derail" to "the data is right." Reasoning: the library must be usable without
CRC (for flexibility and space), so it can't depend on CRC for basic
operation; and full corruption detection without CRC is impossible, so
promising it would be a lie. The exception is structural hazards the library
*itself* creates (e.g. dangling fields at reclaim) — those it must fix
structurally, CRC or not, because they aren't the user's corruption. Rejected:
mandatory CRC; pretending to guarantee integrity without CRC.

Decision: promise structural robustness unconditionally; promise data integrity only when CRC is enabled.
