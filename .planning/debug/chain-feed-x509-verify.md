---
status: resolved
trigger: "ESP32 board's mempool.space feed fails certificate verification: [CHAIN] https://mempool.space/api/v1/prices -> -1 X509 - Certificate verification failed, every request, every retry"
created: 2026-11-25T20:30:00
updated: "2026-11-26T22:50:24.669091+00:00"
---

## Current Focus

Resolved: certificate verification was failing when RSA big-number workspace
allocation exhausted fragmented heap. The hardware-SHA corruption hypothesis
was not established. The current firmware retains certificate validation,
uses bounded/coalescing TLS allocation and prioritizes/reuses the live session.

## Symptoms

expected: [CHAIN] 5/5 ok every 2 min
actual: all 5 GETs return -1, lastError "X509 - Certificate verification failed, e.g. CRL, CA or signature check failed"; heap 69108 free
errors: MBEDTLS_ERR_X509_CERT_VERIFY_FAILED (-0x2700)
reproduction: board on build ac9d35e + uncommitted get_body diagnostic; PMFEED on serial
started: seen 2026-11-25 17:48; worked 15:09-15:32 on build ef5e330

## Eliminated

## Evidence

- timestamp: 2026-11-25T17:52
  checked: openssl s_client TLS1.2 ECDHE-ECDSA and ECDHE-RSA, and TLS1.3 default
  found: server has only an RSA cert; same 3-cert chain for every hello (leaf <- Sectigo OV R36 <- Sectigo R46 cross-signed by USERTrust RSA); ECDSA-only hello gets no cipher
  implication: the ESP32 cannot be handed a different (ECC) chain by cipher choice
- timestamp: 2026-11-25T17:52
  checked: openssl verify -no-CApath -no-CAstore -CAfile root.pem (pinned, sha256 E7:93:C9:B0...CB:D2) -untrusted R36+R46 -no_check_time -purpose sslserver -verify_hostname mempool.space served1.pem
  found: "served1.pem: OK"
  implication: the pinned root alone builds the path in openssl
- timestamp: 2026-11-25T20:25
  checked: scratchpad/online.log, build wifi-tx13-11b (ef5e330), 15:09-15:32
  found: 12 consecutive "[CHAIN] 5/5 ok" lines while "[MINER] HW SHA loop: fast (pipelined)" ran
  implication: the pipelined miner and this TLS code coexisted without error 2 h before the failure; ef5e330..ac9d35e changes only UI/display files

## Resolution

root_cause: RSA verification allocation failure (-17040), established by native instrumentation; the higher-level error hid fragmented-heap exhaustion.
fix: Reduced competing display reservation, reserved TLS records and coalescing scratch allocation, retained authenticated connection and feed scheduling; certificate validation remains enabled.
verification: 25 complete in-window feed cycles on 0e32d00a, prior independent hour, and fresh wallet data after owner-confirmed charger/SD cold boot. See docs/panicminer-validation-2026-11-25.md, Chain feed diagnosis and fix and later SD/TLS follow-up.
files_changed: [src/chainFeed.cpp, src/mining.cpp, src/drivers/displays/panicMinerDisplayDriver.cpp]
