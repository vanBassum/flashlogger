# Flash format

On-flash byte layout. Much is still undecided (the record layer, sector
sequencing, reclaim); update this doc as those land. Sections marked
**TBD** are not decided yet — see
[ideas/design-decisions.md](ideas/design-decisions.md).

NOR-flash rules assumed throughout: erased = `0xFF`, writes only clear
bits (1→0), erase resets a whole sector to `0xFF`.

## Field-layer header (decided, implemented)

Written by `format()`, validated by `init()`. Lives in the reserved top
8 bytes of sector 0.

| Offset | Size | Field |
|--------|------|-------|
| 0 | 4 | Magic `0x464C4F47` ("FLOG"), little-endian |
| 4 | 1 | key_size (uint8) |
| 5 | 1 | value_size (uint8) |
| 6 | 2 | CRC16-CCITT over bytes 0–5, little-endian |

- `init()` reads 8 bytes: magic all-`0xFF` → `FORMAT_MISSING`; wrong
  magic or bad CRC → `FORMAT_CORRUPT`.
- Constraints enforced by `format()`: `key_size` in 1..4, `value_size`
  in 1..255, and the field (`key_size + value_size`) must fit in a
  sector's usable space (`sector_size − 8`).

## Field placement (decided, implemented)

A **Field** is `key_size + value_size` bytes: `[key][value]`, keys and
values little-endian. Fields are fixed size, so the field layer is
index-addressed.

- Fields never span a sector boundary; the tail bytes of a sector too
  small for one more Field are wasted.
- **Every sector reserves the first 8 bytes** for a header, so all
  sectors hold the same number of Fields:
  `floor((sector_size − 8) / field_size)` — the erase-unit size, exposed
  as `fieldsPerUnit()`. This uniformity is why `clear()` needs no special
  case for sector 0.
- Sector 0's reserved bytes hold the real format header. Sectors 1..N
  leave theirs erased (`0xFF`) for now — duplicate/per-sector headers are
  a later, currently-untested feature; the space is reserved so adding
  them needs no layout change.
- `write()` writes key then value, then reads both back and fails with
  `FLASH_WRITE_ERROR` on mismatch (so a plain rewrite fails by design —
  clearing bits is a separate `overwrite()` primitive, TBD).
- `clear(first_field, field_count)` erases whole erase-units (one or
  more); `first_field` and `field_count` must be multiples of
  `fieldsPerUnit()` (else `ARG_INVALID`), and the range must lie within
  the store (else `ARG_OUT_OF_BOUNDS`).

```
Sector 0:  [ 8B header  ][ field 0 ][ field 1 ] ... [ waste ]
Sector 1:  [ 8B 0xFF gap][ field n ][ field n+1] ... [ waste ]
...
```

## Record layout — TBD

The record layer sits on top of Fields; its on-flash encoding is not
decided. Open points:

- **Record header field** — contents (CRC algorithm & width, field
  count vs byte length, flags), written last for crash safety.
- **Record start marker / reserved keys** — a record-layer concern; the
  field layer stores keys opaquely. The values exploit the flash's own
  states and so are **relative to the key width**:

  | Reserved | 1-byte key | 4-byte key | Meaning |
  |---|---|---|---|
  | empty | `0xFF` | `0xFFFFFFFF` | never written (erased state) |
  | tombstone | `0x00` | `0x00000000` | every bit cleared |
  | record start | `0x01` | `0x00000001` | marker / header field |

  Enforced so far: `RecordWriter::field()` rejects all three with
  `ARG_INVALID`, so user data can never look like framing. Because empty
  is all-ones for the width, a plain `0xFF` is a perfectly legal user key
  once keys are 4 bytes wide. Still open: how a record's start is
  recognized during iteration.
- **Long values** — stored by repeating the same key across
  consecutive Fields.

## Sector headers / sequencing — TBD

Every sector already reserves 8 bytes at its top; sector 0 uses them for
the format header, the rest stay `0xFF`. Populating those reserved bytes
with per-sector headers (magic, config, sequence numbers for
mount-recovery ordering) is the deferred feature — no layout change
needed when it lands.
