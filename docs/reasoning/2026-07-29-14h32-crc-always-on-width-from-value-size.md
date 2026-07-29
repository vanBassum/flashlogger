---
id: 2026-07-29-14h32
date: 2026-07-29
time: "14:32"
title: Reliability contract v2 — structural robustness always; CRC always on, width from value size
supersedes: 2026-07-29-14h11-3
---

Supersedes the opt-in framing of the earlier contract. **Unchanged:** the
library never derails on any flash contents — iteration is bounded, terminates,
and never crashes or reads out of bounds. **Changed:** CRC is not user-optional,
it is **always on**. Its width is derived from `valueSize` — 1 → CRC8, 2–3 →
CRC16, ≥4 → CRC32, i.e. the largest CRC that fits the value. Since `valueSize`
is already in the store header, nothing extra is stored and `format()` takes no
CRC argument. Reasoning: an opt-in CRC added a behaviour axis and a
"works-without-CRC" mode with no real demand; always-on upgrades the guarantee
from "won't crash, may hand back garbage" to "won't crash, *detects*
corruption" (except the `S=0` edited/genuine-0 case, still trusted per the
recompute scheme). Deriving width from `valueSize` keeps `format()` simple and
needs no stored width byte. Rejected: opt-in CRC (extra mode, no need);
user-chosen width via `format(keySize, valueSize, crcSize)` + a stored width
byte (flexibility with no current use case — deferred; would only be wanted to
leave header room for flags, itself undecided).

Decision: CRC always on; width = largest that fits `valueSize` (derived, not stored, not user-chosen for now); structural robustness unchanged.
