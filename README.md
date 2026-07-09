# FlashLoggerV2

Portable, platform-agnostic C++ library for logging structured data to
flash memory.

## Constraints & Goals

- No dynamic memory allocation
- `stdint.h` types throughout for unambiguity
- Simple API
- Thread-safe (the caller/wrapper owns the locking)
- Operations: **Append**, **Read**, **Overwrite** (clear bits only)

## Terminology

| Term | Meaning |
|---|---|
| **Record** | One log entry, consisting of one or more Fields |
| **Field** | A single key-value pair within a Record |

Key size and value size are configured once at format time. After that
they are static.

## Build & test

```
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

## Documentation

See [`docs/`](docs/):

- [architecture.md](docs/architecture.md) — the two-layer design
- [flash-format.md](docs/flash-format.md) — on-flash byte layout
- [roadmap.md](docs/roadmap.md) — milestones and remaining work
- [efficiency-analysis.md](docs/efficiency-analysis.md) — why
  variable-length + Option B
- [ideas/](docs/ideas/) — open design decisions and parked ideas
- [backlog/](docs/backlog/) — per-item work notes
