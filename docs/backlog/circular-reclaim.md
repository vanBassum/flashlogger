# Circular reclaim

_Placeholder (M3)._

- Reclaim policy (erase oldest / erase-ahead).
- Sector sequencing for recovery ordering.
- Records spanning the reclaim boundary must die cleanly ("dangling fields").

<!-- TODO: sector-0 is solved (per-sector reserved headers already in place);
     what remains is reclaim policy + sector sequence numbers. -->
