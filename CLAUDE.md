# FlashLoggerV2 — Claude Instructions

## Workflow rules

- **User owns all design decisions.** Never invent API shapes, names, data layouts, or behaviour. When anything design-related is unclear, stop and ask.
- **Strict TDD.** A failing test must exist before any implementation change. Never add a method, member, or constructor parameter without a failing test that requires it.
- **Minimal effort.** Only add what the current failing test demands. No speculative members, no future-proofing.
- **LogBook.md is read-only.** The user writes to it, Claude only reads.
- **Log, don't document.** Do NOT write or maintain design documentation. It goes stale and can be regenerated from the code plus the log. Capture the *why* as reasoning notes instead — see below.

## Logging, not documentation

The design docs (`architecture.md`, `roadmap.md`, `efficiency-analysis.md`,
`ideas/*`) were **deleted on purpose** on 2026-07-30. Do not recreate them, and
do not answer a change by "updating the docs".

| Path | What it is | Rules |
|---|---|---|
| `docs/reasoning/` | the log — one note per decision, with rejected alternatives | append-only and **immutable**; never edit or delete a note, a correction is a new note with `supersedes:`. Use the `reasoning-note` skill; timestamp from the real clock |
| `docs/flash-format.md` | on-flash byte layout | the one kept reference — it is a contract with data already on devices. Update when the encoding changes |
| `docs/backlog/*.md` | plain TODO lists so things aren't forgotten | disposable; delete items once implemented. TODOs only — no prose, no design write-ups |
| `docs/LogBook.md` | Bas's own notes | read, never write |

Offer a reasoning note when a decision actually lands — a trade-off resolved, an
alternative rejected, an assumption changed. Not for routine implementation
steps. The reasoning is the asset; the decision is one trailing line.

## Build & test

```
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Always run tests after every implementation change.

**Always commit and push.** Don't leave work uncommitted. For code,
commit once all tests pass; for reasoning notes and backlog edits, commit and
push directly. Every commit is pushed to the remote in the same step.

## Constraints

- No dynamic memory allocation
- `stdint.h` types throughout (`uint8_t`, `uint32_t`, etc.)
- C++17
- No threading primitives in the field layer

## Where the code lives

```
[ Record layer ]   <- RecordLog + RecordWriter, in progress
[ Field layer  ]   <- FieldStore, done
[ IFlash HAL   ]   <- interface + RamFlash test double
```

- **IFlash** (`src/iflash.h`) — abstract flash: `read`, `write`, `erase`, `getSectorSize`, `getSize`
- **RamFlash** (`test/ram_flash.h`) — RAM test double; asserts on 0→1 bit transitions (flash rule)
- **FieldStore** (`src/field_store.h`) — field layer, index-addressed fixed-size fields
- **RecordLog** (`src/record_log.h`) — record layer; owns a `FieldStore`, so callers never see the field layer
- **FlashLogError** (`src/flashlog_error.h`) — shared error enum for all layers

Byte layout is in `docs/flash-format.md`. Everything else — why any of it is
this way, and what was rejected — is in `docs/reasoning/`. Read the log rather
than trusting a summary here; this file deliberately keeps none.
