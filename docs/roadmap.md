# Roadmap to usable

"Usable" = a consumer (e.g. the Strux manager) can mount a flash area,
append Records crash-safely, iterate them after a power cycle, and the
ring reclaims space forever. Gap analysis 2026-07-06; decisions marked
**decide** are Bas's (per CLAUDE.md), everything else is work.

See [architecture.md](architecture.md) for the layer structure.

## Design decisions

Tracked in [ideas/design-decisions.md](ideas/design-decisions.md).

- **Resolved — the sector-0 problem:** every sector reserves header-sized
  space (per-sector reserved headers); sector 0 holds the real header, the
  rest stay 0xFF. Structural, implemented — see
  [flash-format.md](flash-format.md).
- **Resolved — where the append cursor lives:** the record layer; the field
  layer is index-addressed and holds no cursor.
- **decide — record header field contents:** CRC algorithm & width,
  field count or byte length, flags.
- **decide — iterator shape:** caller-owned handle (stated preference);
  forward-only?; how a lapped/invalidated handle reports itself (ties into
  hash-validity).

## Milestones

| # | Milestone | Scope | Backlog item |
|---|---|---|---|
| M1 | Field layer | small | ✅ done |
| M2 | Record layer | **the bulk** | [record-layer](backlog/record-layer.md) |
| M3 | Circular behavior | evening-ish | [circular-reclaim](backlog/circular-reclaim.md) |
| M4 | ESP integration | mechanical | [esp-partition-adapter](backlog/esp-partition-adapter.md) |
| M5 | Consumer side | tracked in Strux | — |

### M1 — field layer ✅ done

`format`/`init`, indexed `read`/`write` (with read-back verify), `clear` of
whole erase-units, `keySize`/`valueSize`/`fieldsPerUnit`, symmetric per-sector
layout. 32 tests. The originally-planned `overwrite()` and field-empty
detection were folded into the record layer: write integrity / overwrite is
parked in [write-verification](backlog/write-verification.md);
empty-detection emerges with mount/recovery.

### M2 — record layer (the bulk)

- Append: data fields first, header field written last (= crash
  safety), CRC over the record; long values via repeated keys.
- Mount/recovery: find append point after reboot; torn record (data
  without header) is skipped garbage, not corruption.
- Record iterator (skips torn/erased).
- Hash-validity handle for read/overwrite of possibly-lapped records.

### M3 — circular behavior

- Reclaim policy (erase oldest / erase-ahead), sector sequencing for
  recovery ordering, execution of decision 1. A record spanning the
  reclaim boundary must die cleanly (LogBook's "dangling fields").

### M4 — ESP integration (mechanical)

- `EspPartitionFlash` adapter (esp_partition read/write/erase), IDF
  component packaging. **decide:** IFlash timeouts — pass through or
  drop (currently decorative).
- One on-target smoke test.

### M5 — consumer side (tracked in Strux)

- Strux partition change (shrink www, add `log`; one factory reflash)
  and the thin manager owning the store + mutex — the thread-safety
  layer the library deliberately omits.

## Cheap wins, anytime

- CI: GitHub Action running `cmake && ctest`
  ([ci-workflow](backlog/ci-workflow.md)).

Rough proportions: M1/M4/M5 ≈ an evening each; **M2 is the project** —
comparable to everything built so far, and where strict TDD earns its
keep (torn-write recovery driven from RamFlash tests instead of
hardware debugging).
