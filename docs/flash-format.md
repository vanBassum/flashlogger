# Flash format

On-flash byte layout. Much is still undecided (the record layer, sector
sequencing, reclaim); update this doc as those land. Sections marked
**TBD** are not decided yet — see
[ideas/design-decisions.md](ideas/design-decisions.md).

NOR-flash rules assumed throughout: erased = `0xFF`, writes only clear
bits (1→0), erase resets a whole sector to `0xFF`.

## Field-layer header (decided, implemented)

Written by `format()`, validated by `init()`. Lives at offset 0 of
sector 0.

| Offset | Size | Field |
|--------|------|-------|
| 0 | 4 | Magic `0x464C4F47` ("FLOG"), little-endian |
| 4 | 1 | key_size (uint8) |
| 5 | 1 | value_size (uint8) |
| 6 | 2 | CRC16-CCITT over bytes 0–5, little-endian |

- `init()` reads 8 bytes: magic all-`0xFF` → `FORMAT_MISSING`; wrong
  magic or bad CRC → `FORMAT_CORRUPT`.
- Constraints enforced by `format()`: `key_size` in 1..4,
  `value_size` non-zero.

## Field placement (decided, implemented)

A **Field** is `key_size + value_size` bytes: `[key][value]`, keys and
values little-endian. Fields are fixed size, so the field layer is
index-addressed.

- Fields never span a sector boundary; the tail bytes of a sector too
  small for one more Field are wasted.
- **Sector 0** reserves the first 8 bytes for the header; the rest holds
  `floor((sector_size − 8) / field_size)` Fields.
- **Sectors 1..N** hold `floor(sector_size / field_size)` Fields each.
- `write()` writes key then value, then reads both back and fails with
  `FLASH_WRITE_ERROR` on mismatch (so a plain rewrite fails by design —
  clearing bits is a separate `overwrite()` primitive, TBD).

```
Sector 0:  [ 8B header ][ field 0 ][ field 1 ] ... [ field k ][ waste ]
Sector 1:  [ field k+1 ][ field k+2 ]           ... [ field m ][ waste ]
...
```

## Record layout — TBD

The record layer sits on top of Fields; its on-flash encoding is not
decided. Open points:

- **Record header field** — contents (CRC algorithm & width, field
  count vs byte length, flags), written last for crash safety.
- **Record start marker / reserved keys** — `0xFF` = empty, `0x00` =
  erased (a record-layer concern, not field-layer). How a record's
  start is recognized.
- **Long values** — stored by repeating the same key across
  consecutive Fields.

## Sector headers / sequencing — TBD

Ties into the sector-0 problem: whether sectors carry small per-sector
headers with sequence numbers for mount-recovery ordering, or the format
header is handled another way.
