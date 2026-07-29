# Design decisions

_Bas's to make (per CLAUDE.md); nothing is decided until locked in._

## Decided

- **Sector-0 problem → per-sector reserved headers.** Every sector reserves
  header-sized space; sector 0 holds the real format header, the others stay
  `0xFF` (no duplicate header content yet). Structural, implemented — see
  [flash-format.md](../flash-format.md) and the reasoning notes
  [header-in-every-sector](../reasoning/2026-07-28-16h34-header-in-every-sector.md)
  / [structural-not-powerfail](../reasoning/2026-07-28-16h37-per-sector-headers-are-structural-not-powerfail.md).
- **Append cursor → record layer.** The field layer is index-addressed and
  holds no cursor.

## Open

1. **Record header field contents** — CRC algorithm & width, field count or
   byte length, flags.
2. **Iterator shape** — caller-owned handle; forward-only?; how a lapped handle
   reports invalidity. Leaning (2026-07-06): the handle stores a hash of the
   records it spawned; a later read or overwrite uses it to tell whether the
   ring has since lapped/invalidated it.

## Other open questions

- **Record start marker / magic value** — how a record's start is recognized.
- **Thread-safety mechanism** — mutex, critical section, or caller-provided
  lock (consumer layer; the library core omits threading).
- **Sector management** — reclaim policy and when erasing happens. The field
  layer exposes `clear`; the *policy* is a record-layer / reclaim concern.
- **Write integrity** — field-layer read-back verify vs record-layer CRC; see
  [write-verification](../backlog/write-verification.md).

<!-- TODO: as each open item is decided, move it to "Decided" with the why. -->
