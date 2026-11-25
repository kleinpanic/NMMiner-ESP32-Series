# Pipelined classic-ESP32 SHA256d port: correctness review

## Status, 2026-11-25 01:00

Fixed on fork `v4/panicminer`, flashed as `v4-review-fixes`:
- F1: `96f2b62`, nonce submitted as exactly 8 hex digits.
- F3: `0bd8907`, a loop with no confirmed candidate in 2^23 nonces is demoted.
- F4, in part: `4c1ecc7`, the `[SHA]` line ends with a count of dropped ring slots.
- F8, HW path: `ce04d99`, the cursor reseeds on a new header only.

Open: F2 (the `plain` fallback's store spacing), the rest of F4, F5, F6, F7, and F8 on
the SW path. On this unit F2 only matters if both fast variants fail and the loop is
demoted to `plain`.

Range: `83edb33..4b93f0b`, repo `firmware/custom/NerdMiner-CYD-Custom` (working tree at 4b93f0b).
I only read the code. I built nothing and changed no file. The assembly timing was out of scope, but I read the asm's calling convention (hit, budget and run order; `nonce - 1`) and checked it against the C side.

## Verdict

No blocker shows up in the C code alone. Every hop keeps the byte order consistent. The SW and HW nonce halves do not overlap. A stale-job candidate can never be submitted against a new header, and every lock path releases all three locks. One finding becomes a blocker on pools that require an 8-character nonce (F1). The rest are should-fix or minor.

## Findings (ranked)

### F1. should-fix, a blocker on strict pools (pool side unconfirmed): nonce submitted without zero padding
- `src/stratum.cpp:223`: `String(nonce, HEX)`. Arduino's `ultoa` does not pad.
- The pipelined path makes this frequent. The native nonce is `bswap32(cursor)`, so its top byte is `cursor & 0xFF`, and that byte cycles through 0x00..0xFF every 256 nonces. So 1/16 of HW shares produce a 7-character (or shorter) nonce string. HW is about 95% of the hashrate. Before this change, native nonces started at 0xDA54E700 and almost never reached a leading zero during a job's life, so the bug never fired.
- Failure: a share that passes `verifyShareSw()` locally goes to the pool as e.g. `"7abcdef"`. A pool that checks the length rejects it; I recall ckpool's `parse_submit` doing this, but did not confirm it. A pool that hex-decodes the string by bytes builds a different header and answers "low difficulty". Either way about 6% of HW shares are lost.
- Fix: `char n[9]; snprintf(n, sizeof n, "%08lx", nonce);` and pass `n`.

### F2. should-fix (likely, not run on the unit): the last-resort `plain` loop breaks the asm's own store-spacing rule
- `src/mining.cpp:1312` `pl_mine_plain` uses `nerd_sha_ll_fill_text_block_sha256*` (`:1161`, `:1184`, `:1222`). These are plain C stores to non-volatile `SHA_TEXT`, now compiled at `-O3 -funroll-loops`.
- Evidence: I compiled the same fill bodies standalone with the project toolchain at the same flags. The `TEXT` stores come out 2 cycles apart (store, `l32r`, store), and two of them are back to back (`s32i.n` at 0x5d, then 0x5f). The asm header's rule 1 says that at 2-cycle spacing a store is lost whenever core 0 touches DPORT, with 30-60% wrong hashes. Commit 0812284 moved Wi-Fi, lwIP, TLS, the Monitor's SPI flush and the SW miner all onto core 0.
- Failure: `plain` is the floor. It is the only loop chips before rev 3 ever get, and it is where `pl_health` demotes to. It is never self-tested, and `pl_health` cannot demote past it. SW verify keeps wrong hashes from being submitted, but shares are lost with nothing in the logs. The bench did not cover `plain`, since it is not an asm variant.
- Fix (smallest): give `pl_mine_plain` volatile `TEXT` stores with 2 `nop`s between them, or make it an asm `PL_MINE` instance with wide delays. Run `pl_selftest` on it as well and log the result. Separately, run the bench with `plain` under the 9 stress modes.

### F3. should-fix: the runtime fallback only sees false positives, so a loop that stops producing candidates is never demoted
- `src/mining.cpp:1428-1443` `pl_health` runs only when the asm returns a hit (`:1508`).
- A wrong hash is random, so only 1 in 65536 wrong hashes shows up as an "unconfirmed" candidate. The other 65535 just hide a real candidate. The detection rate scales with the wrong-hash rate r: about 256·r expected misses per 256-hit window. That works for r ≳ 0.1% (demotion within about 10 min), but a failure where the sampled word is never zero (a constant non-zero read) yields no hits at all. The hashrate on the UI stays full while shares drop to zero. I have no repro for that mode.
- Fix: also count nonces per loop, and demote when `hits < nonces/65536 / 4` over at least 2^22 nonces (expected 64 hits; seeing ≤16 has probability ≈ 1e-14).

### F4. minor: the `[SHA] checked/mismatched` log does not measure what it says on the fast loops
- `src/mining.cpp:1507` publishes `sw_hash` (or zeros), and `:1770` prints the counts.
- On the fast loops the Monitor recomputes the same SW hash it was handed, so every confirmed hit is a check of software against itself. `mismatched` is exactly the unconfirmed count, a false-positive count as in F3, and says nothing about digests the engine got wrong. `lock %s` always prints `all` (`minerShaLockAllOn()` returns true).
- Ring overruns (`prefilter_consume` `:157`) are skipped without being counted. At about 15 hits/s, a Monitor stall of more than about 1 s (an NVS `saveStat` flash write, a long LVGL frame) drops hits that nothing counts.
- Fix: print the loop name, candidates per 2^16 nonces against the expected 1.0, and dropped ring slots. Drop the `lock` field on this build.

### F5. minor (latent): a core-1 task the HW miner starves forever
- `.pio/libdeps/ESP32-2432S028R/OpenFontRender/src/OpenFontRender.cpp:728`: `RenderTask` is pinned to core 1 at priority 1, and its caller spins `while (g_RenderTaskStatus != IDLE) vTaskDelay(1);`.
- Under `HW_MINER_OWNS_CORE1` that task never runs, because the miner is priority 19 and never blocks while the queue is fed. The Monitor would hang on its first `render.drawString` call, which also stops `prefilter_consume` and `saveStat`.
- This build uses `PANICMINER_UI`, so the panicMiner driver is selected (`display.cpp:35`) and OpenFontRender is unused. But `HW_MINER_OWNS_CORE1` depends only on `PIPELINED_ASM_MINING` and ESP32 (`mining.h`). Dropping `-DPANICMINER_UI` brings back `esp23_2432s028r.cpp`, which then deadlocks.
- Fix: `#error` when `HW_MINER_OWNS_CORE1` is defined without `PANICMINER_UI`, or call `render.setUseRenderTask(false)`.
- I found no other task pinned to core 1. Wi-Fi and lwIP run on CPU0, Arduino UDP on core 0, serial event on any core, and `ipc1` (priority 24) still preempts the miner, so flash ops do not stall.

### F6. minor: the 0xFF stop sentinel collides with job ids ≡ 255 mod 256
- `src/mining.cpp:389` and `:1482-1484`. `MiningJobStop` writes `s_working_current_job_id = 0xFF`.
- If the current job is #255 (and every 256th job after), the id does not change. Take a chunk popped just before the stop whose `run = 1` lands after the stop's `run = 0`. The set-then-check passes (0xFF == 0xFF), so it mines a full 0.27 s chunk on a dead connection. Its shares carry id 255 and are dropped by `job_pool == res->id`, so no wrong submit, only wasted work. The SW path has the same alias.
- Fix: add a separate `volatile bool s_mining_stopped`, or check `job_pool` generation in 32 bits.

### F7. minor: late candidates after a reconnect are counted as false shares
- `src/mining.cpp:1520` pushes shares mid-chunk. `MiningJobStop` clears the result list, then restarts `job_pool` at 0.
- A candidate the asm returned just before `run = 0` is pushed after the clear, carrying the old id K. If K equals the new `job_pool` (for example K = 0 because the drop came during the first job), stratum `:763` verifies it against the new header. That fails, so `falseShares++` and the log prints "[VERIFY] dropped false share", making a correct loop look like it is producing wrong hashes. Nothing is submitted.
- Fix: put a connection epoch in `JobRequest`/`JobResult`, or make ids monotonic across `MiningJobStop` (don't reset `job_pool`).

### F8. minor (pool behaviour unconfirmed; also present on the SW path): an identical re-notify reseeds to the same nonces
- `src/mining.cpp:1460-1468`. The reseed key is `id OR header`, and every notify increments `job_pool`. A pool that resends the same job (same header, extranonce2 fixed) makes the cursor restart at the constant `bswap(0xDA570700)|0x80000000`. Every candidate already found is then submitted again as a duplicate. The SW path does the same thing (`nonce_pool = 0xDA54E700`).
- Fix: key the HW reseed on the header alone (bytes 0..75). It already contains everything that makes nonces distinct.

## Areas checked and found correct

1. **Byte order across all hops.** `hdr[i] = bswap(LE word i)` gives the engine BE words. The asm writes `TEXT[3] = cursor`. The candidate is `ctx->nonce - 1`: the asm increments `a4` after the block-2 fill and checks the hit before budget and run, so the last nonce of a chunk is not lost. `cand_native = bswap(cursor-1)` is stored LE at +76. `verifyShareSw` stores `res->nonce` LE at +76, and the SW path uses the same convention. `pl_mine_plain` double-swaps correctly. The SW prefilter `a7&0xFFFF == 0x32E7` equals `H7 & 0xFFFF == 0`, the same test as the asm's `l16ui [28]`, so a correct loop never yields "unconfirmed".
2. **Nonce split.** `swNonce` stays within 8063 < 8192 = `SW_NONCE_SPAN`, and both `nonce_pool +=` sites were updated. The HW cursor has bit 31 set, which means native byte 76 ≥ 0x80. It starts at 0x800757DA and needs about 2^31 nonces to wrap, against the 10-minute job cap. The cursor carries across chunks and demotions, so there are no gaps or overlaps.
3. **Job change.** The order is queue cleared under mutex, then id, then `run = 0`. The worker sets `run = 1` and then checks the id. Both are volatile (memw), so every interleaving ends with `run == 0`. Stale shares are filtered by `job_pool == res->id`, then re-verified against `mMiner`. The header memcmp (4b93f0b) covers id reuse after a reconnect.
4. **Locks.** `pl_mine_chunk` has a single lock/unlock pair with no `return` in between. Hit, budget, `run = 0`, and a stale chunk where the loop never runs all reach `sha_unlock_job(true)`. `pl_select_loop` locks and unlocks once. The asm is only called under the locks. Try-locks plus back-off mean no deadlock.
5. **Self-test.** The 6 expected candidates, the `budget == 0` check and a fresh `c` are sound. No false-drop path found (see F3 for the opposite failure).
6. **Watchdogs.** `WDT_MINER_TIMEOUT = 900 s`. The interrupt WDT on CPU1 is fed by the tick, and the asm leaves interrupts enabled. The IDLE1 check is off in the sdkconfig.

## Not confirmed
- F1: which pools reject short nonces.
- F2: codegen inside the real inlined `pl_mine_plain`, since I compiled a standalone copy only.
- Whether any IDF component calls the blocking `esp_sha()` / `esp_sha_lock_engine`. Such a caller could wait a long time against the miner's back-to-back re-locks, but it could not corrupt a hash.
