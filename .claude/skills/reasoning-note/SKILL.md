---
name: reasoning-note
description: >
  Capture the WHY behind decisions as an append-only log of one-paragraph
  reasoning notes, stored in docs/reasoning/. Proactively OFFER a note the
  moment a real decision is made in conversation — an architecture choice, a
  trade-off resolved, a rejected alternative, a changed assumption, a corrected
  earlier belief. Also use when the user asks to record reasoning, wants to know
  WHY something was built a certain way, asks what past decisions exist, or says
  an earlier decision turned out wrong. Notes are immutable: never edit or delete
  them; corrections are always new notes.
---

# Reasoning Note

Log the reasoning behind decisions so the project's *understanding* — not just
its code — is preserved. The reasoning is the asset; the decision is just its
current output. A note with a fat decision but thin reasoning is a failure —
capture the WHY.

## Core rules (do not violate)

1. **Notes are immutable.** Never edit or delete an existing note file. If a
   note was wrong, write a NEW note that says what was wrong and why. The old
   note stays exactly as written — it explains why the (possibly wrong) code
   exists.
2. **A note is one short paragraph.** Fast to read, fast to approve.
3. **Always include what was rejected.** The rejected alternatives are the part
   that's normally lost and the most valuable thing here.
4. **Reasoning over decision.** The paragraph (why + alternatives) is the point;
   the `Decision:` line is a single trailing sentence. Never fabricate a
   rationale — if the WHY wasn't stated, ASK for it before writing.
5. **No status fields, no maintained index.** A note is true as of its
   timestamp. Supersession is a new note with a backlink, not an edit.

## When to offer a note (proactive)

Watch for a decision actually landing: "let's go with…", "we'll use X instead of
Y", an assumption adopted, a trade-off resolved, or the user realizing an
earlier decision was wrong. When you sense one, **stop and offer a note** —
draft it, show it, let the user approve or tweak before writing. This is the
gate: it should cost the user one glance. Do NOT log routine implementation
steps or anything with no alternative considered.

## The gate (how a note gets written)

1. Draft the note in the format below and show it in chat.
2. Ask the user to approve or edit — a single quick exchange.
3. Only after approval, write the file. Never write a note silently.
4. After writing, **commit the note and push** to the remote — a note
   isn't safe until it's pushed. Commit it on its own or alongside the
   change it explains; never leave a note uncommitted.

## Where notes live

`docs/reasoning/` in the repo root (create it if missing). One file per note —
this enforces immutability; git only ever ADDS files here, never modifies them.

The note is **date/time stamped**, not sequentially numbered. Get the ACTUAL
current date and time (e.g. run `date '+%Y-%m-%d %H:%M'`) — never guess or reuse
a timestamp. The `id` is that timestamp: `YYYY-MM-DD-HHhMM` (24-hour clock, `h`
as the hour/minute separator, e.g. `2026-07-27-14h32`), which is also the
filename prefix. If two notes land in the same minute, append `-2`, `-3`.
Timestamps sort chronologically, so the folder reads as a timeline.

Filename: `docs/reasoning/YYYY-MM-DD-HHhMM-short-slug.md`

## Note format

```markdown
---
id: 2026-07-27-14h32
date: 2026-07-27
time: "14:32"
title: WebSocket vs polling for live status
supersedes:
---

We went with a single WebSocket multiplexed across all channels rather than
per-feature polling, because TLS is already established and reconnection logic
is simpler to own in one place. Rejected polling (too chatty at scale) and
per-feature sockets (connection sprawl). Untested on flaky mobile networks.

Decision: single multiplexed WebSocket.
```

- `id`, `date`, and `time` all come from the real current clock, not a guess.
- `supersedes:` is blank for a normal note.
- `title` is a short human-readable handle.
- The paragraph must contain the *why* and the *rejected alternatives*.
- The final `Decision:` line states the outcome in one sentence.

## Corrections / supersession

When something proves wrong, write a NEW note. Do not touch the old one. Set
`supersedes:` to the timestamp id(s) it replaces.

```markdown
---
id: 2026-10-02-09h15
date: 2026-10-02
time: "09:15"
title: Phone-only access is insufficient
supersedes: 2026-07-06-11h20
---

The 2026-07-06 note assumed phone-only access was fine. A customer whose phone
battery died couldn't get in, so the assumption was incomplete rather than the
implementation being wrong. Considered "document the edge case and move on" but
access failure is not acceptable. Adding a PIN fallback; code built on the
phone-only entry flow now needs a refactor.

Decision: add PIN fallback; refactor entry flow.
```

`supersedes:` holds the timestamp id of the note(s) being replaced. The older
note remains, so the timeline (old timestamp -> new timestamp) tells the story
of what was learned. A correction is a real event with its own consequences —
call out downstream impact (refactors, invalidated decisions) in the paragraph.

## Retrieval (finding relevant reasoning)

When the user asks why something exists, whether a past decision applies, or
what's unresolved — and at the start of substantial work — pull in the log:

1. **While the whole log fits in context, read all of it.** A thousand
   one-paragraph notes is well under 100k tokens. Reading everything is the only
   way to *guarantee* nothing relevant was missed, so prefer it. Glob
   `docs/reasoning/*.md` and read them.
2. **If the log is too large to read whole**, select the most relevant notes
   plus the most recent, and TELL the user what you did ("Read all N notes" or
   "Searched N, surfaced these M") so a missed note is visible, not silent.
3. **Follow `supersedes` chains.** If a relevant note was superseded, surface
   the superseding note and mention the original for history.
