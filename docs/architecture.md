# Architecture

Two-layer design over a hardware abstraction. Each layer depends only on
the one below it. Build order is bottom-up: field layer first, record
layer on top.

```
┌─────────────────────────────────────────────┐
│  Record layer          (in progress)         │  Records = 1+ Fields.
│  append / iterate / overwrite, CRC,          │  Crash-safe append,
│  mount-recovery, circular reclaim            │  power-cycle recovery.
├─────────────────────────────────────────────┤
│  Field layer            (FieldStore)         │  Fixed-size Fields,
│  format / init / read / write / clear        │  index-addressed,
│                                              │  header + validation.
├─────────────────────────────────────────────┤
│  IFlash HAL             (interface)          │  read / write / erase
│    RamFlash   — test double (NOR rules)      │  + geometry + timeouts.
│    EspPartitionFlash — target (M4, TBD)      │
└─────────────────────────────────────────────┘
```

## IFlash — hardware abstraction

Abstract flash interface: `read` / `write` / `erase`, `getSectorSize` /
`getSize`, with an overridable timeout type. Models real NOR flash:
erased = `0xFF`, writes can only clear bits (1→0), erase resets a whole
sector to `0xFF`. `RamFlash` (test) asserts on illegal 0→1 transitions;
`EspPartitionFlash` (target adapter) is future work.

## Field layer — `FieldStore`

The dumb, simple layer. Owns raw flash access, the format header, and
fixed-size Field placement; hides sector math from callers.

- A **Field** = one key + one value, both fixed size, set at `format()`.
- Fields are fixed size, so the layer is **index-addressed** (no
  iterators here).
- Fields never span a sector boundary.
- Every sector reserves header-sized space (symmetric layout), so erase-units
  are uniform: `clear(first_field, field_count)` erases whole units and
  `fieldsPerUnit()` reports the unit size.
- `format(key_size, value_size)` writes the header; `init()` reads and
  validates it (magic + CRC). See [flash-format.md](flash-format.md).
- No threading — the caller's responsibility.
- No knowledge of Records, CRC-over-records, or what a key means.

## Record layer — on top of Fields (in progress)

Being built test-first; only what a test demanded exists so far —
`RecordLog(IFlash&)`, `format(key_size, value_size)` and `init()`, which
forward to the field layer. `RecordLog` **owns** its `FieldStore` (by value,
no allocation): the field layer is an internal detail, so a consumer
constructs and formats a `RecordLog` and never names a Field. Everything
below is the target, not the current state.

The hard part. A **Record** is one log entry made of one or more Fields;
long values are stored by repeating the same key across consecutive
Fields. This layer adds append, iteration, overwrite, per-record CRC,
crash-safe recovery, and circular reclaim.

Internals are still open — see
[ideas/design-decisions.md](ideas/design-decisions.md) (record header
contents, iterator shape).

## Boundaries (why the split)

- Fields stay trivial and independently testable; the record layer can
  be driven entirely from `RamFlash` tests, including torn-write
  recovery, without hardware.
- Reserved keys (`0xFF` empty, `0x00` erased) are a **record-layer**
  concern, not a field-layer one.
- Thread safety lives above the library entirely (in the consumer's
  manager), not in either layer.
