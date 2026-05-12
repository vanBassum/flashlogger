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

## Flash Efficiency Comparison

Assumptions:
- **Useful bytes** = avg fields × field size (key + value)
- **Option A** overhead = 1 byte per field
- **Option B** overhead = 1 header field per record (same size as a regular field)
- **Option C** overhead = 4 bytes per record (fixed; x=4 assumed: CRC16 + status + reserved)
- **Static max** = 7 fields per record

### 5-byte fields (1B key + 4B value)

| Layout | Avg fields | Opt A (bytes) | Opt A % | Opt B (bytes) | Opt B % | Opt C (bytes) | Opt C % |
|---|---|---|---|---|---|---|---|
| Variable | 3 | 18 | 83% | 20 | 75% | 19 | 79% |
| Variable | 5 | 30 | 83% | 30 | 83% | 29 | 86% |
| Variable | 7 | 42 | 83% | 40 | 88% | 39 | 90% |
| Static (max=7) | 3 | 42 | 36% | 40 | 38% | 39 | 38% |
| Static (max=7) | 5 | 42 | 60% | 40 | 63% | 39 | 64% |
| Static (max=7) | 7 | 42 | 83% | 40 | 88% | 39 | 90% |

### 9-byte fields (1B key + 8B value)

| Layout | Avg fields | Opt A (bytes) | Opt A % | Opt B (bytes) | Opt B % | Opt C (bytes) | Opt C % |
|---|---|---|---|---|---|---|---|
| Variable | 3 | 30 | 90% | 36 | 75% | 31 | 87% |
| Variable | 5 | 50 | 90% | 54 | 83% | 49 | 92% |
| Variable | 7 | 70 | 90% | 72 | 88% | 67 | 94% |
| Static (max=7) | 3 | 70 | 39% | 72 | 38% | 67 | 40% |
| Static (max=7) | 5 | 70 | 64% | 72 | 63% | 67 | 67% |
| Static (max=7) | 7 | 70 | 90% | 72 | 88% | 67 | 94% |

Key observations:
- Static records drop to ~38% efficiency when avg fill is 3/7 — the layout choice matters far more than the overhead option
- Variable records are 75–94% efficient regardless of avg fields
- Option A is constant efficiency per field size (independent of record size)
- Option B gets more efficient as records get larger (header cost amortized)
- Option C beats Option B for small records; loses at large records when value size is small

---

## Decisions

| Decision | Choice | Reason |
|---|---|---|
| Record layout | **Variable-length** | No wasted space; static never outperforms variable |
| Overhead scheme | **Option B** — one header field per record | CRC covers whole record; header-last write gives crash safety naturally |

---

## Open Design Questions

- What does the header field contain exactly? (CRC algorithm, size, other flags?)
- What is the record start marker / magic value?
- How does the iterator API look in C?
- Thread-safety mechanism: mutex, critical section, or caller-provided lock?
- How are sectors managed — who erases, and when?

---
