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

## Why things are the way they are

There is deliberately no design documentation — it goes stale, and it can be
regenerated from the code plus the log. What is kept is the reasoning:

- [reasoning/](docs/reasoning/) — append-only log, one note per decision, with
  the alternatives that were rejected. Immutable: a note is never edited, and a
  correction is a new note that supersedes the old one. Timestamps sort, so the
  folder reads as a timeline of how the project got here.
- [flash-format.md](docs/flash-format.md) — the one exception. On-flash byte
  layout is a contract with data already written on devices, so being wrong
  costs data that can't be recovered.
- [backlog/](docs/backlog/) — plain TODO lists so things aren't forgotten.
  Disposable: items are deleted once implemented.
- [LogBook.md](docs/LogBook.md) — Bas's own notes.
