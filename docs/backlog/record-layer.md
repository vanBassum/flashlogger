# Record layer

_Placeholder. This is the bulk of the project (M2)._

- Append (data fields first, header field last, CRC over the record).
- Mount / recovery after reboot (skip torn records).
- Record iterator.
- Hash-validity handle for read / overwrite of possibly-lapped records.

<!-- TODO: depends on the design decisions in ideas/design-decisions.md. -->
