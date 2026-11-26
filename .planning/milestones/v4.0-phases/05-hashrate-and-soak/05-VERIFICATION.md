---
phase: 05-hashrate-and-soak
verified: "2026-11-26T22:50:24.669091+00:00"
status: passed
score: 2/2 requirements verified
---

# Hashrate + Soak: evidence reconciliation

Requirements: MINE-01, MINE-02.

Prior rc.1 mining-core image passed its own 60.24-minute 967.925 kH/s soak with accepted work. Current rc.2/stable image passed its distinct 1,201.807-second, 220-sample run at 968.720 kH/s; all 17 strict checks passed. Image identities are preserved.

Evidence: [docs/panicminer-validation-2026-11-25.md](../../../../docs/panicminer-validation-2026-11-25.md), exact image
`0e32d00af0c9cb8d996f1a284cc9a04768707402d336ae5ec8f9e64c4c27b835`,
firmware source `07c3931`, public CI 45897395376 and preserved private captures.
This report reconciles completed hardware work; it does not invent an executed
plan or relabel the prior image's hour. No requirement gap remains within the
documented observed-board and protocol scope.
