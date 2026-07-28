---
id: 2026-07-28-16h34
date: 2026-07-28
time: "16:34"
title: Keep the field layer simple; defer power-failure robustness to the record layer
supersedes: 2026-07-28-14h49-prove-gap-before-fixing-sector-0.md
---

We reversed the earlier plan to prove the field layer's torn-write gap with a
power-loss rig and test. Writing a test to demonstrate a failure we have no
intention of fixing at this layer isn't TDD — a red test is a promise to make
it green, and here it would just be noise. Instead we keep the field layer
deliberately dumb (fixed-size fields, no per-field housekeeping byte or CRC, no
torn-write detection) and push power-failure / crash-safety up to the record
layer, where the actual use case lives. Rejected: characterization/red tests
for the field-layer gap, and making the field layer "foolproof" with per-field
metadata. The power-loss rig itself isn't discarded — it returns when the
record layer needs to test crash safety.

Decision: field layer stays simple; power-failure robustness is a record-layer concern; drop the field-layer power-fail test.
