# Parked ideas (vague, deliberately)

_Placeholder. Not designed, not scheduled — revisit when a real need
appears._

- **Handled-flags convention** (Bas, 2026-07-06) — the application
  dedicates one key/value pair per entry as a flags field; a service
  iterates entries, performs a task (e.g. ship logs to a server), then
  clears bits to mark it handled. Pure application convention on top of
  clear-bits-only Overwrite — the library only guarantees clear-bits-only
  writes.
- **Fast lookup / secondary index** — jump to next/prev field with the
  same key (per-field linked-list offsets, or per-sector key index).
- **Ordered / binary search** over records.

<!-- TODO: expand only if/when needed. -->
