---
phase: 03-debrand-and-egress
verified: "2026-11-26T22:50:24.669091+00:00"
status: passed
score: 4/4 requirements verified
---

# De-brand + Egress: evidence reconciliation

Requirements: PRIV-01, PRIV-02, PRIV-03, MINE-03.

Branded surfaces removed; portal saved/reopened primary/fallback, forced fallback authorized and advanced work, original primary restored. Header audit attributed all observed IPv4 endpoints; two IPv6 frames were counted without destination decoding.

Evidence: [docs/panicminer-validation-2026-11-25.md](../../../../docs/panicminer-validation-2026-11-25.md), exact image
`0e32d00af0c9cb8d996f1a284cc9a04768707402d336ae5ec8f9e64c4c27b835`,
firmware source `07c3931`, public CI 45897395376 and preserved private captures.
This report reconciles completed hardware work; it does not invent an executed
plan or relabel the prior image's hour. No requirement gap remains within the
documented observed-board and protocol scope.
