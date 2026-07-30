---
id: 2026-07-30-13h29-5
date: 2026-07-30
time: "13:29"
title: Reserved key values scale with key width — empty is all-ones
supersedes:
---

The reserved keys are relative to the key width, not fixed byte values: empty is
all-ones for the width (`0xFF` at 1 byte, `0xFFFFFFFF` at 4), tombstone is `0`,
record-start is `1`. Reasoning: the whole point of these values is to exploit
states the flash already produces — erased is all-ones whatever the field width,
and a tombstone is every bit cleared — so a fixed `0xFF` would be an arbitrary
constant that happens to coincide with the erased state only at one key size.
The consequence, and the test that pins it, is that a plain `0xFF` becomes
perfectly ordinary user data once keys are 4 bytes wide; the guard reads
`keySize()` off the store rather than assuming. Tombstone and record-start
deliberately do *not* widen to something like `0x000000FF`: a tombstone means
every bit cleared, and the marker only needs to be reachable from all-ones by
clearing bits and still clearable down to zero afterwards, which `1` satisfies at
any width. Practical detail: the derivation special-cases width 4 rather than
shifting by 32, which would be undefined behaviour. Rejected: fixed `0xFF` /
`0x00` / `0x01` at every key width (breaks the flash-state correspondence at
widths above 1, and needlessly steals `0xFF` from the user's key space); widening
all three reserved values proportionally (no flash-state meaning for a widened
tombstone or marker).

Decision: empty = all-ones for the key width, tombstone = `0`, record-start = `1`; derived from `keySize()`, not hardcoded.
