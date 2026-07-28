---
id: 2026-07-28-14h49
date: 2026-07-28
time: "14:49"
title: Prove the power-failure gap with a test rig before designing the sector-0 fix
supersedes:
---

Rather than jump straight to implementing a sector-0/header-safety scheme, we
decided to first extend RamFlash with power-loss injection and write tests that
demonstrate what the current field layer does under a torn write — proving it
isn't foolproof before committing to a fix. The reasoning: a red test that
documents the actual gap is stronger justification (and a stricter spec) than
an abstract argument, and it keeps us honest about which layer owes which
guarantee. Rejected: designing/building the fix (approach B) directly, which
risks solving a problem we haven't concretely characterized. Also sequenced
power-failure tests on existing operations (format/write) ahead of any
sector-erasing work.

Decision: build the power-loss test rig and characterize current behavior first; defer the fix.
