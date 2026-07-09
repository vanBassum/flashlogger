# Blocking design decisions (vague, deliberately)

_Placeholder. These are Bas's to make (per CLAUDE.md); nothing here is
decided until locked in._

1. **Sector-0 problem** — header shares sector 0 with fields; circular
   reclaim would erase it.
2. **Record header field contents** — CRC algorithm & width, count vs
   length, flags.
3. **Where the append cursor lives** — field layer or record layer.
4. **Iterator shape** — caller-owned handle; forward-only?; how a
   lapped handle reports invalidity. Leaning (2026-07-06): a caller-owned
   handle stores a hash of the records it spawned; on a later read or
   overwrite the hash reveals whether the ring has since lapped/
   invalidated it.

## Other open questions

- **Record start marker / magic value** — how the start of a record is
  recognized on flash.
- **Thread-safety mechanism** — mutex, critical section, or
  caller-provided lock. (Library core omits threading; this is the
  consumer/wrapper's story.)
- **Sector management** — who erases sectors, and when.

<!-- TODO: one decision per section, with the chosen option and why. -->
