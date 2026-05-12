# FlashLoggerV2 — Claude Instructions

## Workflow rules

- **User owns all design decisions.** Never invent API shapes, names, data layouts, or behaviour. When anything design-related is unclear, stop and ask.
- **Strict TDD.** A failing test must exist before any implementation change. Never add a method, member, or constructor parameter without a failing test that requires it.
- **Minimal effort.** Only add what the current failing test demands. No speculative members, no future-proofing.
- **LogBook.md is read-only.** The user writes to it, Claude only reads.

## Build & test

```
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Always run tests after every implementation change. Commit only when all tests pass.

## Constraints

- No dynamic memory allocation
- `stdint.h` types throughout (`uint8_t`, `uint32_t`, etc.)
- C++17
- No threading primitives in the field layer

## Architecture

Two-layer design (field layer first, record layer later):

```
[ Record layer ]   <- not yet started
[ Field layer  ]   <- FieldStore — current work
[ IFlash HAL   ]   <- interface + RamFlash test double
```

- **IFlash** (`src/iflash.h`) — abstract flash interface: `read`, `write`, `erase`, `getSectorSize`, `getSize`
- **RamFlash** (`test/ram_flash.h`) — RAM-backed test double; asserts on 0→1 bit transitions (flash rule)
- **FieldStore** (`src/field_store.h`) — field layer; `init()` reads header from flash, `format(key_size, value_size)` writes it
- **FlashLogError** (`src/flashlog_error.h`) — shared error enum for all layers

## Header format (written by `format()`, validated by `init()`)

| Offset | Size | Field |
|--------|------|-------|
| 0 | 4 | Magic `0x464C4F47` ("FLOG") |
| 4 | 1 | key\_size (uint8\_t) |
| 5 | 1 | value\_size (uint8\_t) |
| 6 | 2 | CRC16-CCITT over bytes 0–5 |

## Key decisions

| Topic | Decision |
|-------|----------|
| Record layout | Variable-length |
| Overhead scheme | Option B — one header field per record |
| Field layer threading | None — caller's responsibility |
| Flash HAL timeout type | `uint32_t` by default, overridable via `FLASHLOGGER_TIMEOUT_T` define |
| Reserved keys | `0xFF` = empty, `0x00` = erased — record layer concern, not field layer |
