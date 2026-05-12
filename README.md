# FlashLoggerV2

Portable, platform-agnostic C library for logging structured data to flash memory.

---

## Constraints & Goals

- No dynamic memory allocation
- Use `stdint.h` types throughout for unambiguity
- Simple API
- Thread-safe
- Operations: **Append**, **Read**, **Overwrite** (clear bits only)

---

## Terminology

| Term | Meaning |
|---|---|
| **Record** | One log entry, consisting of one or more Fields |
| **Field** | A single key-value pair within a Record |

Key size and value size are configured once at format time. After that they are static.

---

## Open Design Questions

### Record layout: static vs variable-length

**Option A — Static records** (fixed number of Fields per Record)
- Simple; index-based access works
- Wastes space when a Record has fewer Fields than the maximum

**Option B — Variable-length records** (start marker + Fields until end marker)
- More space-efficient; no gaps
- Requires iterators instead of index access
- Cannot determine up front how many Records fit in a sector

---
