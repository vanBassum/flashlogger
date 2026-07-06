# Roadmap to usable

"Usable" = a consumer (e.g. the Strux manager) can mount a flash area,
append Records crash-safely, iterate them after a power cycle, and the
ring reclaims space forever. Gap analysis 2026-07-06; decisions marked
**decide** are Bas's (per CLAUDE.md), everything else is work.

## Blocking design decisions

1. **decide — the sector-0 problem:** the format header occupies bytes
   0–7 of sector 0, which also holds fields. Circular reclaim of
   sector 0 would erase the header. Options: dedicated header sector /
   small per-sector headers / rewrite header on reclaim. Per-sector
   headers would also carry the sequence numbers mount recovery needs.
2. **decide — record header field contents:** CRC algorithm & width,
   field count or byte length, flags.
3. **decide — where the append cursor lives:** field layer or record
   layer.
4. **decide — iterator shape:** caller-owned handle (stated
   preference); forward-only?; how a lapped/invalidated handle reports
   itself (ties into hash-validity).

## Milestone 1 — finish the field layer (small)

- `overwrite()` with clear-bits-only (AND) semantics — `write()`'s
  exact read-back verify makes rewrites fail by design; the flags use
  case needs a deliberate primitive.
- Field-empty detection (all 0xFF) — the primitive every scan needs.

## Milestone 2 — record layer (the bulk)

- Append: data fields first, header field written last (= crash
  safety), CRC over the record; long values via repeated keys.
- Mount/recovery: find append point after reboot; torn record (data
  without header) is skipped garbage, not corruption.
- Record iterator (skips torn/erased).
- Hash-validity handle for read/overwrite of possibly-lapped records.

## Milestone 3 — circular behavior

- Reclaim policy (erase oldest / erase-ahead), sector sequencing for
  recovery ordering, execution of decision 1. A record spanning the
  reclaim boundary must die cleanly (LogBook's "dangling fields").

## Milestone 4 — ESP integration (mechanical)

- `EspPartitionFlash` adapter (esp_partition read/write/erase), IDF
  component packaging. **decide:** IFlash timeouts — pass through or
  drop (currently decorative).
- One on-target smoke test.

## Milestone 5 — consumer side (tracked in Strux)

- Strux partition change (shrink www, add `log`; one factory reflash)
  and the thin manager owning the store + mutex — the thread-safety
  layer the library deliberately omits.

## Cheap wins, anytime

- CI: GitHub Action running `cmake && ctest`.
- Commit or explicitly gitignore `LogBook.md` (currently untracked —
  exists only on one machine).

Rough proportions: M1/M4/M5 ≈ an evening each; **M2 is the project** —
comparable to everything built so far, and where strict TDD earns its
keep (torn-write recovery driven from RamFlash tests instead of
hardware debugging).
