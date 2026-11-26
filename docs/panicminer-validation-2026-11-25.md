# PanicMiner validation — 2026-11-25

The CYD runs the matching Mine, Hash, Chain, Clock, Boot and Setup design in
the fork's `ESP32-2432S028R` environment, with the verified panel inversion.
The full 4 MB device image is backed up before every firmware write.

## November 26: SD, runtime reconnect and wallet follow-up

### Qualified USB/SD release candidate

Firmware source `49afce5` passed the complete ASan/UBSan suite and the board
build (59.759 seconds). Application: **1,781,040 bytes**, SHA-256
`23236906a43745274836183fa75348ceca927dee2350919bea08c25ce96de986`.
Installation followed a fresh full 4 MB backup, checksum/full-flash comparison
and independent verification of all four written images. Matching factory
inputs and ELF are archived locally. The first boot authorized the pool in
16.539 seconds. Smoke and 24 private-path checks passed. Three complete CSV
downloads returned 381–382 records in 7.307, 7.700 and 6.962 seconds. A complete
half-closed export and its immediate follow-up passed, as did cancellation and
three overlapping busy/status checks. Blocked-peer pressure passed ten full
exports and 66 status requests, with 17,732 bytes minimum sampled free heap,
zero native faults and zero radio drops. The CSVs contained 383–385 records;
each declared body arrived in full. Fixed source-port JSON/SD requests passed
initially and after a 65-second delay (both reused ports); the EOF-reader test
closed after 2.079 seconds. Packet headers show the Content-Length clients
closing first and fresh SYN/SYN-ACK handshakes on both reused ports.

All five pages passed programmatic navigation/stack checks; automatic pool
recovery after a controlled Wi-Fi drop took **36.216 seconds**. The fresh strict
Mine run then passed **all 17 checks**, independently graded against retained
native/serial snapshots: **1,203.988 seconds**, **222 samples**, **967.899 kH/s**
completed-work mean. The 10-second range was 964.003–970.669 kH/s and the
60-second range 967.208–968.372 kH/s, all within 944.3–1043.7 kH/s. Twenty SD
records were added, all 26 in-window feeds completed 5/5, maximum wallet age
was 46 seconds and minimum sampled free heap was 23,556 bytes. The post-window
CSV returned **409 nine-column records** in **8.464 seconds** with its full
declared body. No API error, native fault, restart or SHA mismatch occurred.

The same boot passed the complete hour and independent continuous qualification:
**3,614.418 seconds**, **236 samples**, **967.925 kH/s**, **one newly accepted
pool share** and **60 new SD records**. All **78 in-window feed cycles** completed
5/5. Minimum sampled free heap was 22,600 bytes and maximum wallet age 48 seconds.
There were zero API gaps, reboots, native faults, radio drops, SHA mismatches or
uncommanded page changes. All 23 continuous-qualification checks passed. The
observer subsequently reached **164.51 minutes on the same boot** without a
native fault; this longer observation is separate from the sampled one-hour test.

After preserving the hour's native evidence, a separate saved-settings EN reset
with SD mounted passed: the pool reauthorized, verified wallet data returned,
and persisted lifetime counters showed **3 accepted shares** with verified best
difficulty **3.077973282**. Wallet amounts/address remain private. This tests
saved restart behavior on laptop USB, not removal/reinsertion of charger power.
The fresh 60-second rate window fills after restart; it is not an endurance
measurement until warmed up. No firmware changed after source `49afce5`.

The owner subsequently authorized publishing the single repository and a release.
Version `v4.0.0-rc.1` records these passing USB/SD results and explicitly leaves
charger cold boot with SD, complete network egress capture, portal/fallback-pool
persistence and observed audio/LED hardware acceptance open. Prior failures
below remain part of the record; the full milestone is not marked complete.

The first public CI run built both firmware targets and passed sanitizers and
restore safety, then exposed a test that required ignored recovered stock
screensavers. Public decoder tests now construct full-size RLE, literal-color
and overlapping LZSS cases independently. The separate stock check still
requires all 17 images when present or explicitly requested; public checkouts
report its absence as a skip. The updated local suite passes 108 tests and 29
subtests; public CI runs 107 with one explicit private-fixture skip. Firmware
source and the hardware-tested application are unchanged by this test repair.
The [public CI run](https://github.com/kleinpanic/NMMiner-ESP32-Series/actions/runs/45873084629)
passed both board builds, sanitizers, restore safety, Python tests, both UI
themes and artifact uploads. The [release](https://github.com/kleinpanic/NMMiner-ESP32-Series/releases/tag/v4.0.0-rc.1)
was published on November 26 after history/private-value/binary scans passed.
All six uploaded assets were downloaded again; their bytes and both external
and bundled checksums matched the local release inputs.

| Same-unit measurement | Hashrate | Evidence boundary |
| --- | --- | --- |
| Historical stock API readback | About 1.04 MH/s | Earlier stock configuration; not this SD-enabled run |
| Agreed acceptance baseline | 994 kH/s | Required range: 944.3–1043.7 kH/s |
| Current strict Mine run | 967.899 kH/s | Completed-work mean over 1,203.988 seconds |
| Current USB/SD hour | 967.925 kH/s | Completed-work mean over 3,614.418 seconds |
After a full declared response has been queued, it now lets a Content-Length
client close first, with a two-second deadline for clients waiting for server
EOF. A request's read EOF never cancels an unfinished body. SD headers hand off
to the file sender; only the end of that body starts the close deadline. Fresh
native gates include reusing the same source ports for JSON and SD requests
after 65 seconds, plus a client that waits for the bounded server close.
Completed SD closes are polled before accepting another request, so an already
finished, half-closed export can release ownership before the next busy check.
The reusable native checker has four real host TCP fixture tests; the complete
Python suite passes **105 tests and 29 subtests**. Native acceptance is separate.

### Previous response-close attempt

Source `47e1232` / application
`a1278a71f139d4dfbba16321d8c2d417b1ad1299c43dc7b8db7c1bf2bfa730cd`
(1,780,960 bytes) was installed after a fresh verified 4 MB backup and all four
image comparisons. Sanitizers and board build passed (62.250 seconds), as did
live-data smoke and private paths. Its first ordinary export timed out; packet
headers show a missing 512-byte sequence range and retransmission after the
client's ten-second read timeout. Mining and feeds continued without a native
fault or reboot. Three ordinary retries returned all 377 records in 7.725,
10.310 and 7.164 seconds; a complete half-closed export also passed. The next
immediate export received 503 because completed-transfer cleanup ran after
request dispatch. This attempt failed prequalification; no timed run began.
A mistakenly overlapping fixed-port/ordinary-download probe also received the
expected busy rejection; that invalid harness attempt is preserved separately
and does not grade port reuse. Original failures and retry evidence are retained.

### Previous saved-network retry qualification

Installed application: **1,780,560 bytes**, SHA-256
`1f59e35593885c57921a509b47c4e5076f29cfa34f1e064eacdf8e07c477a20c`,
firmware source `2fa0ea4`. Fresh full 4 MB backup checksum/full-flash comparison
and all four written images verified; matching factory inputs and ELF are
archived locally. The complete ASan/UBSan suite and board build passed (58.138
seconds). The first native boot joined candidate 1, authorized the pool and
completed three 5/5 live-data cycles. Wallet data was valid, SD was mounted,
minimum unused feed stack was 3,676 bytes and no SDK TLS allocation fallback
occurred. All 24 private-path probes passed. Three ordinary exports, a complete
half-closed request and cancellation/busy/status coexistence passed. Blocked-peer
pressure then passed **ten complete exports and 66 status requests**, including
six while the bulk client stopped reading and a TCP-reset cancellation. Each
CSV contained 335–336 nine-column records, with declared body lengths fully
received. Minimum sampled free heap was **17,804 bytes**; no native fault,
reboot or radio drop occurred during pressure. Five pages and automatic pool
reauthorization after a controlled Wi-Fi drop passed in **36.263 seconds**.

The fresh strict Mine run passed all **17 checks**, with an independent native
grade: **1,202.707 seconds**, **221 samples**, **968.067 kH/s** completed-work
mean (2.61% below the 994 baseline). The 10-second readings ranged from
964.690–971.404 kH/s; 60-second readings from 967.519–968.664 kH/s. All were
within 944.3–1043.7 kH/s. Twenty SD records were added; all 26 in-window feed
cycles completed 5/5. Maximum wallet age was 46 seconds and minimum sampled
free heap was 19,240 bytes. No API error, native fault, restart or SHA mismatch
was recorded. The post-window download returned 359 consistent nine-column
records in 7.113 seconds. Original timed/native snapshots are preserved locally.
The same boot's longer soak **failed composite qualification**: one API connect
timeout occurred around 25.8 minutes. Mining, SD and 5/5 feeds continued without
a native fault or radio drop. TCP observation showed the client reusing a source
port about 62 seconds after a successful request, then remaining in SYN-SENT
while packets arrived. A controlled negative reproduction confirmed the cause:
the initial fixed-port request passed, reuse after 65 seconds timed out while
the board acknowledged the old connection, and reuse after about 140 seconds
passed. Packet headers confirmed the board closed first; the SDK sets TCP MSL
to 60 seconds. This is separate from the repaired bulk-memory exhaustion.
Original soak samples/error, packet headers and failed reproduction are retained
locally. No clean-hour or new-share PASS is claimed for this rejected attempt.

A failed saved-network join previously entered WiFiManager's 180-second setup
portal, then restarted the board. Normal mining tasks and runtime Wi-Fi recovery
could not start while that portal blocked. The repair preserves explicit BOOT
Setup and missing-configuration provisioning, but allows a configured offline
boot to finish and retry in the background. Retries clear temporary BSSID/channel
pinning and restore all supported protocols and the configured TX power. A
post-diagnostic scan now releases its results. Host tests cover the startup
policy, an initial failed join and the existing retry timing. This successful
boot did not exercise the failed-initial-join branch; that distinction remains
part of native acceptance.

### Previous TCP low-water startup attempt

Firmware source `5049bca` passed the complete ASan/UBSan suite and the
`ESP32-2432S028R` board build. Application SHA-256
`a0e20291a4d45b05537405ddf9659d22d5938673bce418f47a271fb4f2c157a9`
(1,783,520 bytes) was installed after a fresh verified 4 MB backup and verification
of all four images. Its initial association failed despite a strong scanned
signal. The blocking setup fallback caused the feed-smoke deadline to expire;
at 286.872 capture seconds it restarted deliberately after the portal timeout,
then joined on the next boot. That restart is counted as a native qualification
fault; it was not a recorded SDK exception. This attempt failed startup
prequalification, so the TCP/SD changes and timed endurance were not qualified.
The RF cause remains unisolated; no router settings were changed.
The SD sender now checks zero-timeout TCP write readiness, respecting the
SDK's send low-water mark rather than filling its entire send queue. It keeps
**20,480 bytes free / 6,144 bytes contiguous** before each bulk write, with
512-byte stdio and retained transfer buffers. Socket errors are read with
`SO_ERROR` without peeking/consuming the request's FIN. The 15-second idle and
three-minute total transfer deadlines remain. Real host tests cover blocked
write readiness, a TCP reset, repeated half-close checks, unread data, idle
connections, stale errno and invalid descriptors. Native acceptance remains open.

### Previous bounded-file qualification

Installed application: **1,783,440 bytes**, SHA-256
`d67f51076944500f0c33490ad0542587c5868b594b29c44783a4d609d14fc633`,
firmware source `d5a39e4`. Fresh full 4 MB backup checksum/full-flash comparison
and all four written images verified. Exact application, ELF and all four
factory inputs are archived locally. The complete ASan/UBSan suite and the
`ESP32-2432S028R` board build passed. Three complete native feed cycles,
24 private-path denials and three complete CSV downloads passed. The downloads
returned 14,832 bytes / 324 rows in 10.097, 5.752 and 5.845 seconds. A newly
added half-close probe then failed after only 512 bytes. A separate blocked-peer
pressure diagnostic failed on its fifth status connection, with **15,064 free
bytes / 2,036 largest**; 4,096 SD body bytes had been queued. The 4,000-byte
contiguous guard was still insufficient. This image failed prequalification;
no timed run started. Original evidence for both failures is preserved.
SD file stdio is now explicitly limited to **512 bytes** before any file
operation; the installed SDK otherwise configures a separate 4,096-byte buffer.
Bulk sends wait below **20,480 free bytes / 4,000 contiguous bytes**. A transfer
also expires after **15 seconds without progress**, within its existing
three-minute total deadline. This prevents an unreachable or low-memory peer
from retaining a file for the entire total timeout. Nonblocking one-byte socket
peeks detect actual socket errors without relying on stale errno; read EOF is
allowed so a valid half-closed request can still receive its response.
Real host socket tests cover idle/data/half-close/invalid descriptors, and
transfer-policy cases cover idle expiry even while memory would prevent a send.
Native complete-body, cancellation, blocked-client and endurance checks remain
required. No hardware acceptance is inferred from the passing host tests.

### Previous control-memory reserve qualification

Installed application: **1,783,152 bytes**, SHA-256
`a25a5ade0ae43679aa38de5356d2311d92bb6ec2b42ee931ea4bda5467044adb`,
firmware source `cda9176`. Fresh full 4 MB backup checksum/full-flash comparison
and all four written images verified. Exact application, ELF and all four
factory inputs are archived locally. The complete ASan/UBSan host suite and
`ESP32-2432S028R` board build passed. Feed/privacy smoke passed, but the first
ordinary CSV download timed out before any body bytes. The retained file left
only **18,804 free bytes / 6,644 largest**, below the 24 KB cutoff. It remained
held until the three-minute total timeout, then released with 0/14,558 bytes
sent. No timed run started. This image failed prequalification; its original
logs and outcome are preserved.
The SD sender now waits below **24,576 bytes free** or **4,000 bytes contiguous**,
leaving headroom for concurrent request buffers, JSON replies and TCP packets.
The installed SDK uses a 1,436-byte TCP maximum segment and may retain a full
segment for a 512-byte write. The policy still permits the observed 4,084-byte
fragmentation shape once aggregate memory recovers. Regression coverage uses
the actual blocked-peer memory trace below. Native pressure and endurance
acceptance are required; host tests alone do not prove this budget sufficient.

### Previous deferred-API qualification

Installed application: **1,783,152 bytes**, SHA-256
`744567d982e4c33f019647f961bb801e6521c6f2bdfeaf4623b33da67c7ccb3c`,
firmware source `3596762`. Fresh full 4 MB backup checksum/full-flash comparison
and all four written images verified. Exact application, ELF and all four
factory inputs are archived locally. Three complete feed cycles, 24 private-path
denials, three complete CSV downloads, cancellation recovery and three concurrent
busy/status scenarios passed. The blocked-client pressure stage then failed.
A repeat measured **13,064 bytes free with only a 628-byte largest block** while
an SD client stopped reading; a status response sent only 512 of its 591 bytes.
After cancellation, memory recovered to 28,832 free / 6,644 largest. This confirms
contiguous-memory exhaustion during that stall. The 16 KB / 2 KB bulk reserve
was insufficient. Mining continued with one boot and no native fault; no timed
run started. Original reports and diagnostic logs are preserved.

Both the SD body and the HTTP headers/JSON replies now avoid the SDK's repeated
synchronous writes. The server write hooks retain one response (at most 8 KB),
send at most 512 bytes per loop without blocking, and preserve exact bytes
across short sends and temporary backpressure. Bulk writes pause while a reply
is pending, so control/status output gets the next send opportunity. A reply
has a ten-second total deadline and releases its buffer/socket reference after
completion; an SD header retains the separate file-transfer socket reference.
Host cases cover exact headers/body under repeated backpressure, bounds,
allocation failure, invalid writes and reset. Full ASan/UBSan tests and board
build pass; this image nevertheless failed native blocked-client pressure.

### Previous API socket-state qualification

Installed application: **1,781,808 bytes**, SHA-256
`8e482e7d1ba02ec162945fed847e5c8880a857f13d3268afb6b7650d1fbd8a7a`,
firmware source `da95928`. Fresh full 4 MB backup checksum/full-flash comparison
and all four written images verified. Exact factory inputs and ELF are archived
locally. Feed/privacy/download/concurrency preflight passed, but the fourth
status request with a blocked SD client timed out. Native timing recorded an
**18,826 ms request pass**, without a slow SD send pass. A separate speaker-only
probe also produced a truncated HTTP body during a blocked download, excluding
SD capacity reads as a necessary trigger. The image failed prequalification;
no timed run started. Original evidence is preserved, with one boot/no native
fault. The next candidate defers API headers and JSON replies as well as bulk
bytes; the SDK's synchronous response writer was still in use here.

The API initializes errno before the installed Arduino-ESP32 SDK's client pass.
That SDK probes `connected()` with a zero-length receive; success need not clear
a preceding client's socket error. The host boundary regression reproduces a
healthy peer rejected by stale reset/broken-pipe state and checks that genuine
receive errors still reject the peer. This is not a native SDK test or proof
that stale errno caused every observed timeout. Added native timing logs
identify request or SD passes lasting at least 500 ms, plus short reads and
permanent send errors, without recording paths or payloads. Full ASan/UBSan
tests and the board build pass.

### Previous SD fragmentation-floor qualification

Installed application: **1,781,488 bytes**, SHA-256
`eb2b2c88bd07da16278e9e6e5cf86170cf1bd324aa5c24842eeb7812f8056ca2`,
firmware source `c2b86f3`. A fresh full 4 MB backup passed checksum/full-flash
comparison; all four written images verified. Exact application, ELF and all
four factory-image inputs are archived locally. Feed smoke, private-path
denials, three complete downloads, cancellation recovery and three concurrent
busy/status export scenarios passed. The blocked-client pressure stage then
timed out on a status request. A diagnostic repeat served its first status
request in 475 ms, but its second request stalled while mining and feeds
continued. One boot and no native fault were observed. The image failed
prequalification; no timed run started. Original evidence is preserved.

This candidate used a **2 KB** contiguous heap floor with a **16 KB** aggregate
reserve and one nonblocking send of at most 512 bytes per loop. The regression
uses the prior native 21,224-byte free / 4,084-byte largest-block reading:
the former 4,096-byte cutoff waits at that fragmentation plateau, while the
repaired policy can progress. Exact bounds, the smaller low-memory wait state
and the unchanged total deadline are covered. Full ASan/UBSan tests and the
board build passed. Those host results did not establish native API reliability.

### Previous fixed-job and nonblocking-export qualification

Installed application: **1,781,488 bytes**, SHA-256
`e25408f421249e7fba83e63b911f0bd0649d0ac409b9c14a014eb088926f73de`,
firmware source `aa23288`. A fresh full 4 MB backup passed checksum/full-flash
comparison; all four written images verified. The exact application, ELF and
four factory-image inputs are archived locally. Three complete native feed
cycles, 24 private-path denials and three complete CSV downloads passed after
saved-settings startup. A later concurrent-download body timed out before its
first 512 bytes. A subsequent memory reading showed 21,224 bytes free with a
4,084-byte largest block, below the 4,096-byte transfer cutoff. Mining and
complete feeds continued with one boot and no native fault. This candidate
failed prequalification; no timed endurance run started. Its original evidence
is preserved.

### Previous SD private-path qualification

Installed application: **1,782,816 bytes**, SHA-256
`f9c8fe99b59c91f396a5e54d0880c13e55aa068ff700bd04b260e3dfee22fba8`,
firmware source `6cf06a7`. A fresh full 4 MB backup passed checksum/full-flash
comparison before installation; all four written images verified. The archived
application, ELF and four exact factory-image slices are preserved locally.

The shared API guard previously compared private filenames case-sensitively,
although the SD filesystem accepts case variants. The repaired production helper
blocks case variants of configuration/secrets paths and their descendants,
leading/trailing-space or trailing-dot aliases, and tilde short-name aliases.
All three SD list/download/delete handlers use it before touching the card.
Public filenames preserve case and internal spaces.

The full C++ sanitizer suite and board build pass. SD paths now have **68**
acceptance/idempotence/rejection scenarios. A separate C++11 negative control
using the old API guard permits all four synthetic aliases; the repaired helper
blocks all four. No actual private file was fetched or deleted for this check.
Native preflight passed: three complete feed cycles; 24 denied private-path
variants; three complete downloads (11,522–11,567 bytes / 252–253 rows);
cancellation recovery and three concurrent busy/status export scenarios;
five pages and automatic reconnect in 41.955 seconds (one boot/no fault).

The measured window ran **1,204.836 seconds / 227 samples / 968.153 kH/s**.
Both sampled rate bands, wallet freshness, SD progress and native SHA checks
passed. The original strict verdict failed complete-feed checks because it
included a **1/5** cycle during the intentional preflight Wi-Fi drop, before the
window. All 26 feed completions inside the actual timed window were 5/5. A
checker repair scopes feed evidence to sample timestamps and adds positive and
negative fixtures; it does not rewrite this original failed report.

The subsequent CSV export timed out and the board then aborted in
`std::make_shared<JobRequest>` / `JobPush`, decoded with the exact archived ELF.
The export had SDK EAGAIN retry messages immediately beforehand. This candidate
therefore **fails composite acceptance**, independently of the checker defect.
Its full-hour observer was explicitly interrupted; the reboot and original
reports remain preserved.

The next source uses fixed four-entry value queues and worker-owned job copies,
removing mining-request allocation and list-node churn. SD exports retain a
512-byte pending block across partial/zero-progress nonblocking sends, coalesce
small packets, and reserve 16 KB free / 4 KB contiguous heap. The full sanitizer
suite and board build pass, including blocked-allocation negative control and
exact-byte backpressure tests. Fresh backup/install and native pressure checks
are required before accepting this repair.

### Previous socket-lifetime qualification

Previous application: **1,781,888 bytes**, SHA-256
`3d180043903e9302e1a090b635a9f3328fd72017472be5b19064e82ee1e5c582`,
firmware source `412e2c9`. A fresh full 4 MB backup passed checksum/full-flash
comparison before installation; all four written images verified. The archived
application digest and all four factory-image slices match those verified inputs.

Three complete Chain/wallet cycles passed with pool authorization, mounted SD,
two reserved TLS records and zero scratch fallbacks. Three complete SD downloads
then passed (7,844–7,889 bytes, 173–174 rows). Cancellation after 512 bytes and
three overlapping-request scenarios also passed: all 7,889 bytes / 174 rows
arrived, a second download returned 503, and status requests worked during the
active export. No native fault or radio loss occurred. Five pages and automatic reconnect
passed (54.651 seconds, one boot/no fault). The fresh strict 20-minute and
full one-hour USB/SD tests started at **10:44:55 UTC**. The strict test passed
**1,202.960 seconds / 227 samples / 967.639 kH/s** by independent work counts
(2.652% below 994). All 17 checks passed; minimum sampled 10/60-window rates
were 960.771 / 966.065 kH/s. Its post-window CSV also passed: 197 rows, nine columns,
3.642 seconds. No fault or radio drop occurred during its first twenty minutes. At about **47.5 minutes**, the API observer
recorded one `TimeoutError`; the native feed also recorded **3/5** completed
requests in **57.994 seconds**, including transport/read/EOF errors. The next
cycle recovered all five feeds. Mining stayed in band, the wallet remained
current, and no native fault or radio disconnect was observed. This attempt
cannot pass the zero-API-error and complete-feed gates. The complete attempt
finished at **3,614.990 seconds / 236 samples / 967.828 kH/s**, with 60 new SD
records, current wallet data throughout (maximum age 94 seconds), minimum free
heap 19,868 bytes, no native fault, no radio drop and one observed boot. No new
pool-accepted share appeared; saved lifetime shares were not counted as new.
The report is **FAIL** on exactly those three gates, with its original evidence
preserved. A fresh native capture then deliberately reset the unchanged image;
saved Wi-Fi joined automatically after the first candidate failed authentication.
That repeat was interrupted for the independently verified SD API privacy
repair before its timed window completed; it is not a PASS. No speculative
firmware change is inferred from the transient failure, and no overall hardware
PASS is claimed.

### Paced-export qualification

Earlier paced-export application: **1,781,808 bytes**, SHA-256
`a6712fcef1ef3f7989478f25b6e97e110b35afbf080e551612bf02a9cb5207e0`,
firmware source `b0282a3`. A fresh full 4 MB backup passed checksum and full-flash
comparison before writing; all four written images verified. Three complete
feed cycles passed, with current wallet data, pool authorization, mounted SD,
stable TLS reservations and zero allocator fallbacks. The feed stack minimum
was 3,732 bytes.

Three consecutive native SD downloads each returned all **6,562 bytes / 145
rows**, in 2.581–2.789 seconds. There was no radio drop or native fault, including
the post-download watch. Five pages were exercised and Mine restored; controlled
Wi-Fi disconnection recovered pool authorization in **41.416 seconds**, with
one boot and no native fault. Its strict run started at **10:10:21 UTC** and
passed **1,201.593 seconds / 228 samples / 967.780 kH/s** by independent work
counters. All 17 checks passed; minimum sampled rates were 964.259 and 967.140
kH/s. The post-window CSV also passed: 168 rows, nine columns, 3.247 seconds.

A subsequent overlapping-request test cut off the active export at 512/7,609
bytes without a crash or radio drop. Repetition with simultaneous status queries
also produced truncated responses. One trace had sufficient memory (largest
block 6,900 bytes); another reached a temporary 1,908-byte largest block. The
SDK connection check uses zero-length recv plus errno, which can retain another
request's socket error. Source `412e2c9` uses the retained descriptor and actual
write progress instead, waits through temporary low memory within the existing
deadline, and rejects a busy download before opening another file. Native
concurrency checks preceded timing for the current image, with the passing
results recorded above. Host policy/sanitizer tests pass. This earlier candidate's hour soak was
explicitly interrupted for that repair; it is not accepted. Charger cold boot with SD remains unavailable
while the owner is asleep; publication stays held.

### Packet-memory follow-up

The earlier packet-budget application was **1,781,200 bytes**, SHA-256
`4fa5892fafcfc09ebea653e24fdc4385d1f631150fbcb7427b7eeeba9adb20ea`,
source commit `a4be00d`. A fresh full 4 MB backup passed checksum/full-flash
comparison and all four written images verified. The native initialization log
confirms dynamic TX 4/RX 8. Three complete feed cycles passed. Its download
preflight then stopped after 1,536 of 6,338 bytes as the largest free block
fell to 660 bytes. The memory guard prevented another restart or radio drop,
but an incomplete download is still a failed check. No timed run or soak was
started for this candidate.

The next build sends at most one 512-byte block per Arduino main-loop pass,
retaining the file/socket between passes and keeping a 180-second total deadline.
This lets network buffers drain between writes and returns control to buttons,
audio and logging. It builds and passes the sanitizer regression suite; fresh
backup/installation and repeated native downloads are in progress. This is not
yet an accepted fix.

The first bounded-transfer image (`a21a35e3ed3d6359cf1830a9dae3c5df0fb6428717036154e5e739849a441645`,
1,780,896 bytes) passed full backup/image verification and three complete feed
cycles, but failed download prequalification. It aborted in `JobPush` when
`std::make_shared<JobRequest>` could not allocate. The observer caught the abort
and restart; this image is rejected. A later heap trace during another download
measured the largest free block falling from about 15 KB to 1,396 bytes, despite
15–17 KB of total free heap. These are fragmentation/pressure measurements, not
proof that every earlier RF failure had the same cause.

Inspection of the pinned Arduino `WiFiGeneric.cpp` found that it overrides the
SDK defaults with 32 dynamic TX and RX packet buffers. Source commit `a4be00d`
caps dynamic TX at four and RX at eight through the SDK initialization structure;
it preserves radio/security fields and existing lower/static limits. The SD
transfer also stops if free heap falls below 8 KB or the largest block below
2 KB. The SDK remains Espressif binary code; the initialization policy is in
`wifiMemory.cpp` and `wifiBufferBudget.h`. The full sanitizer suite, configuration
scenarios and board build pass. Native repeated-download and endurance checks
are still required before accepting the change.

The host sampler now checks Content-Length as well as CSV structure. A truncated
response that ends at a complete row cannot pass qualification. Complete,
lengthless, truncated, invalid-length, oversized and HTTP-error responses have
regression coverage. The full root-venv suite passes 99 tests and 27 subtests.

### SD download follow-up

After the scheduling measurement below, the automatic post-window SD CSV
download timed out. Two manual repeats returned HTTP 200 and the first 1,360
bytes, then stalled. Each was followed by `BEACON_TIMEOUT`; automatic pool
recovery still worked without restarting. The associated one-hour soak was
interrupted. The composite qualification is failed, despite passing performance
checks inside its original 20-minute window. Regrading the later extended capture
also fails complete-feed checks after the download-triggered radio losses.

The pinned SDK's `WiFiClient::write(Stream&)` reads through the byte-at-a-time
base interface, has no disconnect check in its file loop and does not stop when
a storage read makes zero progress. Source commit `34892a9` replaces that path
for both SD download routes with direct reads of at most 512 bytes, a yield
between blocks, a deadline and abort on failed/short progress. It does not claim
that every Wi-Fi failure was caused by this path. Ten transfer scenarios and the
full sanitizer host suite pass; the board build passes. Repeated native downloads
must pass before a fresh timed run and full soak are accepted.

### Scheduling qualification (before SD download repair)

That scheduling application was **1,781,168 bytes**, SHA-256
`f9b46f96df8bab0520ebc7e73dbeeef935ac564d54c893a16b8274e35fb8ee6b`;
firmware source is commit `e7ff617`. A fresh full 4 MB backup passed checksum
and flash comparison, and all four written images independently verified.
The CYD Stratum task now runs at priority 6, above Monitor at 5. Network waits
still yield. No assembly kernel or rate threshold changed.

Three complete live-data cycles passed with SD mounted, the saved pool worker
authorized and wallet data valid. Active TLS scratch stayed at 11,392 bytes,
peak 22,220, zero SDK fallbacks, with two record-buffer reservations. The feed
task's initial minimum free stack was 3,704 bytes. All five pages were exercised;
Mine was restored before timing. Controlled non-erasing Wi-Fi disconnection
recovered pool authorization in **41.153 seconds**, with one boot and no fault.
Across the first eight prepared jobs, the maximum preparation was **3,823 µs**.

The strict measurement started at **09:15:48 UTC on November 26** and recorded
**1,200.984 seconds, 225 samples and 968.076 kH/s** by independent work counters.
All 17 checks passed at the timed-window end, including both rolling-rate bands,
SD progress, current wallet, one boot, 30 complete feed cycles and clean SHA checks.
The minimum sampled rates were 965.212 kH/s (10-window) and 967.005 (60-window).
The subsequent CSV timeout failed composite qualification and interrupted the
soak, as described above. This is not an overall hardware-acceptance PASS.
Standalone charger cold boot with SD still needs the owner, who is asleep;
publication remains held.

### Earlier continuous-work qualification

That application was **1,781,168 bytes**, SHA-256
`f8da51e39bffbe18419fe24de1a888346af77eae0144da4f26b06f02ac95d792`.
A fresh full 4 MB backup passed checksum and flash comparison before installation;
all four written images independently verified. Firmware source commit `194eada`
includes bounded actual-work reporting and the bounded hex-decoding repair.
The complete sanitizer host suite passes, including 24 work-batch/queue scenarios;
96 Python tests and 22 subtests pass. These are host results, not endurance acceptance.

Timing diagnostics measured a 52 ms hardware-job gap without a new job and a
129 ms preparation gap. They also verified complete live-data cycles and recovery
after a controlled Wi-Fi disconnect in 43.063 seconds. Those diagnostic traces
were deliberately interrupted for installation and do not establish a timed pass.
Source inspection found that the worker discarded its current request when the
network task had not refilled the queue, despite retaining a continuous nonce
cursor for a still-valid header. It can now continue that header through fresh
nonces. A stopped or replaced full generation invalidates retained work, including
IDs with equal low bytes. Pool cancellations still stop the assembly loop.
No completed work is estimated and the rate threshold is unchanged.

That candidate passed three complete feed/wallet cycles, page checks and automatic
pool recovery in 42.720 seconds after a controlled disconnect. Its strict run
failed after 63.264 seconds at 944.1116 kH/s against the unchanged 944.3 minimum.
The native timing counters recorded a new-job preparation lasting 272,937 µs
at the dip; hardware SHA acquisition remained at most 25 µs. Pool, SD and wallet
stayed healthy. The failed run and interrupted soak are preserved.

The CYD Stratum task ran below Monitor on the same core while new-job
preparation deliberately stopped hardware hashing. This motivated the scheduling
correction above; this failed candidate does not establish endurance acceptance.

Factory packaging separately now reads PlatformIO's upload manifest, including
the SDK boot-app image. Missing, empty, overlapping or out-of-range inputs fail
instead of producing an incomplete factory image. Five tests cover those paths;
this packaging change does not alter the already installed application.

### Earlier connection-reuse qualification

Hardware acceptance remains open. The parser-memory image avoided the earlier
heap abort but failed the strict rolling-rate gate during a repeated TLS
handshake. The equal-priority experiment then produced repeated peer connection
resets (`MBEDTLS_ERR_NET_CONN_RESET`), with no completed live-data cycle; that
experiment is rejected too. Host HTTPS access still succeeded.

The next candidate restores ChainFeed priority 2 and retains the authenticated
connection across 45-second snapshots. Failed requests still close the connection
and receive at most one transport retry. Certificate and hostname checks remain
enabled. This avoids deliberately repeating certificate verification on every
update. Monitor/SW stack reductions and the parser memory repairs are retained.
Native connection reuse, stack margins, runtime reconnect and a fresh strict
20-minute run must pass before accepting this candidate. The one-hour soak and
standalone charger boot with SD remain separate gates.

The installed connection-reuse application is **1,779,856 bytes**, SHA-256
`4f318f75542122b963d6303676ac025550609cc995854066f6da00887b1e87f4`.
Its fresh 4 MB backup passed checksum and flash comparison; all four written
images verified. Saved-settings USB boot mounted SD, restored counters and
authorized the pool. Three complete feed/wallet cycles passed: the first took
10,023 ms, reused cycles 1,329 and 1,583 ms. Record reservations stayed at two
uses; active TLS scratch stayed at 11,392 bytes, peak 22,220, zero SDK fallbacks.
Retaining active TLS allocations is expected while the connection remains open.
An obsolete smoke assertion required multiple handshakes; it was corrected to
require exactly two record allocations and stable live scratch use. Its failed
assertion log is preserved and does not indicate a firmware fault.

All five pages were exercised and Mine restored before timing. A controlled
non-erasing Wi-Fi disconnect reauthorized the pool in **41.811 seconds**, without
a restart. A new complete feed/wallet cycle followed in 10,714 ms. Observed
minimum free stacks were Monitor 2,500, Stratum 2,684, HW miner 1,920, SW miner
1,628, ChainFeed 2,224 and loopTask 3,516 bytes. The strict attempt then failed
after 76.995 seconds at a sampled 10-window rate of 931.66 kH/s. The 60-window
rate was 962.12 kH/s; pool, SD and wallet remained valid, all feed cycles were
complete and no native fault occurred. That attempt and its associated soak
are preserved as failed/interrupted. They do not establish endurance acceptance.

The reproducible native capture, API sampler and independent grader are in
`scripts/`; their commands are in `CONTRIBUTING.md`. Fourteen positive/negative
fixtures cover missed abort/restart, stale capture, bad rolling rates, counter
resets, pool/SD/wallet loss and exclusion of wallet values from sample reports.
The sanitizer host suite and nine HTTP retry/connection-lifetime scenarios pass.

### Completed-work accounting correction

Source inspection found that the work counter credited entire 256K-nonce HW
jobs only after their results passed through Stratum's 50 ms network loop.
Monitor samples could therefore omit both the current hardware batch and
already-completed queued work. This produces delayed steps in the displayed
rolling rate; the source did not actually count progress inside the current job.

The correction retains the 256K job queue and unchanged assembly/SHA checks,
but executes at most 32K nonces per assembly call and immediately records only
the actual completed nonces. Cancellation and early candidates credit only work
done. These work counts are no longer queued for a second Stratum accounting
pass; the unused per-job result allocation is removed. The shared counter
normalizes million-hash carry on every update. No rate is estimated or padded.
Sixteen sanitizer scenarios check exact totals, maximum uint32 work size,
bounded publication, early candidates, cancellation and invalid progress.
The bounded-work application is 1,779,488 bytes, SHA-256
`e0bf4c691ba7d073a94b2ab267e59b98966e8d0b16e6d59650ff88893412c71f`.
Its fresh full 4 MB backup and all four installed images verified. Native
prequalification passed: three complete feed/wallet cycles, stable active TLS
scratch, all five pages, SD writes and pool recovery in 42.250 seconds after a
controlled disconnect. The fast SHA loop passed all six known candidates;
checked runtime candidates had zero mismatches or drops.

The strict attempt started at 08:36:54 UTC and failed after 10.401 seconds:
10-window rate 943.44 kH/s, 60-window rate 964.05 kH/s. Pool, SD and wallet
remained valid, and native capture reported no fault. The associated soak was
interrupted. The minimum remains 944.3 kH/s; the near miss is not rounded into
a pass. Accounting delay was a real source defect, but its repair alone did
not establish the required performance stability.

A timing build records aggregate job preparation, hardware-job gaps and SHA
engine waits, with maxima, once per ten seconds. This measures the remaining
pause without changing the SHA kernel or lowering the acceptance threshold.
The full Python suite now passes 91 tests plus 15 subtests, including the
historical dashboard proxy/HTTP tests; all dependencies are pinned in root
requirements and the dashboard launcher uses root `.venv`. Dashboard statement
coverage is 95%; this is not a claim of whole-project or hardware coverage.

### Follow-up charger failure and USB diagnosis

The owner reported Mine at zero with no pool after the charger move. Router
queries returned no active miner association and its status API was unreachable;
the new 20-minute observer never reached its warmup gate. The owner reconnected
USB and reported a restart before any further firmware write. The captured
boot rejected the stronger candidate with `AUTH_FAIL`, joined the other
candidate, authorized the pool and mined near 961 kH/s. A pool-confirmed accepted
share and increasing SD CSV rows are now observed. No payout is inferred.

With the card mounted, all five Chain requests repeatedly failed with mbedTLS
memory-allocation errors; free heap was about 44–49 KB. The build uses FAT
per-file caches with a 4,096-byte maximum sector, and SD's default five file
slots reserve unnecessary RAM. The repair limits SD to two simultaneous file
slots. The first combined build retained a 44 KB LVGL pool and reduced the
display transfer buffer from 20 to 10 rows, preserving certificate validation.
The Stats page has five spaced
metric rows and confirmed wallet BTC/USD and pending transaction values.
Wallet queries strip the worker suffix and never print the address URL on
failure. Their public schema was checked against the saved address privately.

Runtime Wi-Fi recovery previously called Arduino's force-disconnect/reconnect
every five seconds while automatic event retries were enabled. Both paths
retained the startup BSSID/channel restriction. The repair gives the Stratum
task ownership, waits 30 seconds between attempts and rebuilds an unpinned
all-channel configuration. This fixes a recovery defect; the original radio
drop is not isolated. Espressif documents the need to coordinate application
retries and scans in its [Wi-Fi guide](https://github.com/espressif/esp-idf/blob/v4.4/docs/en/api-guides/wifi.rst).

The production host suite, SD slot-limit/no-format check, reconnect timing and
wrap checks, wallet parsing/arithmetic checks and both theme renders pass.
The first combined build was 1,797,600 bytes, SHA-256
`b1d2c7cfd3ace8f4ddc203099e88a0c4b05d5ba20d040d5f8c5d2879be2c4de5`.
A fresh verified 4 MB backup preceded its installation; all four installed
images independently verified. Native boot mounted SD, restored statistics,
authorized the pool and resumed mining. A controlled non-erasing Wi-Fi disconnect
at observer second 183.061 triggered an unpinned join at 213.294 and pool
reauthorization at 223.805; later readings were about 968 kH/s. No application
restart was required. This verifies runtime recovery, not a new charger boot.

That build still failed RSA certificate allocation with SD mounted. The failure
is retained; increasing free heap alone did not establish TLS acceptance.
The follow-up reduces LVGL to 40 KB, the transfer buffer to four rows and the
ChainFeed stack to 8 KB, freeing another 12,032 bytes. ChainFeed reports its
minimum unused stack after a fetch so the smaller stack can be checked natively.
The follow-up build is 1,797,728 bytes, SHA-256
`82b42c1fe714923b946b9da3ea73d212ca6ed7cdb3f470e3e017752ce7ac3640`.
A fresh full 4 MB backup was checksum-checked and compared with flash before
installation; all four images independently verified. Native saved-settings boot
mounted the card, restored statistics, authorized the pool and loaded **5/5
Chain endpoints** in 6,289 ms. The wallet request succeeded on the same verified
TLS path. The feed task reported **1,736 bytes minimum unused stack**; no stack
overflow or SHA mismatch was observed. The API confirmed mounted capacity
31,246,958,592 bytes. Downloaded CSV contained 22 nine-column records with the
expected measured-statistics header. A complete 320 × 240 native Stats frame
shows balance, USD value, rates, accepted/rejected work, difficulty, CPU, RAM,
temperature, signal, SD writes and SHA checks without overlapping rows.
The configured address and its confirmed/pending amounts match an independent
explorer query; no address or wallet amount is published.

The serial framebuffer dump blocks the display/API task and is deliberately
kept outside the timed window; a simultaneous CSV request timed out, and its
retry after the dump succeeded. The display was returned to Mine before a new
20-minute API/work-counter observer began its warmup. No new 20-minute
performance PASS or charger cold-boot acceptance with SD is claimed yet.

### Final stack-margin correction

A second successful feed cycle lowered the 8 KB task's minimum unused stack
from 1,736 to **300 bytes**. The timed observer was stopped and preserved as
interrupted; this is not an endurance pass. The correction increases ChainFeed
to 10 KB and reduces display transfer storage from four rows to two. Its total
RAM requirement is only 768 bytes above the TLS-working candidate, while adding
2,048 bytes of stack margin. No certificate check is bypassed.

The correction builds as 1,797,728 bytes, application SHA-256
`52b88e70b6508b4115f79b4978bdce87acc7392ffe526b3d5b6578124979876c`.
Its fresh full 4 MB backup passed the checksum and flash comparison; all four
installed images verified. A repeated-fetch smoke check succeeded twice, then failed all six requests
with `SSL - Memory allocation failed`: 76–77 KB was free but the largest
block was 32,756 bytes. Stack margin remained 2,340 bytes. This candidate is
not accepted; the failure is separate from the earlier RSA scratch allocation.

The next repair reserves both SDK-sized TLS record buffers before SD/Wi-Fi
allocation churn. The supported mbedTLS allocator hook reuses those two slots
only for ChainFeed record allocations; other tasks and smaller crypto
allocations retain Espressif's original allocator. Released slots are zeroed on
reuse. No certificate flags, cryptography or trust configuration are changed.
The host suite includes repeated reuse, exhaustion, zeroing, arithmetic overflow,
foreign-pointer routing and unequal-slot ordering, including a C++11 sanitizer
build matching the firmware language level. The repair builds as 1,798,496 bytes, application SHA-256
`0c6c662324ddd30c37a6797be89de9a64cd56fa1461fe403c3837e9105442780`.
The first 460,800-baud backup read stopped at 62% on a corrupt USB packet;
the helper aborted before any firmware write. The fresh 230,400-baud backup
then passed its checksum and full flash comparison, followed by all four
installed-image verifications. The installed build then passed three consecutive
native feed cycles: all five Chain endpoints and wallet data loaded each time
in 9,690, 9,251 and 9,205 ms. Minimum unused task stack remained **2,280 bytes**;
reserved-record use advanced from two to four to six, establishing actual
buffer reuse. SD mounted, saved statistics restored and the pool authorized;
no panic, brownout, heap assertion, TLS allocation error or SHA mismatch/drop
was observed during this smoke check. The subsequent timed attempt was stopped
after another certificate-scratch allocation failure at native observer second
437.212. The record reservation was reused successfully, but RSA verification
had only a 2,676-byte largest free block despite 20,092 free bytes. That cycle
loaded 4/5 Chain endpoints; mining, pool authorization, wallet data and SD
writes continued. This interrupted attempt is not an endurance pass.

### Separate certificate-scratch pool

The next correction reserves a separate 24 KB scratch pool before SD/network
startup, alongside the two record buffers. It uses a separate instance of the
TLSF allocator already bundled in pinned LVGL 8.3.11. Only ChainFeed's mbedTLS
allocations use it; other tasks retain the SDK allocator. Free blocks coalesce
between certificate operations, allocations are zeroed, and SDK fallback
remains available if the scratch pool is exhausted. Diagnostics report used/
peak scratch bytes and fallback counts. This changes allocation policy only;
trust, certificate flags and cryptographic checks remain unchanged.

The host suite passes 1,000 allocation/free/coalescing cycles against that actual
allocator under C++11 sanitizers, plus ownership, bounds and overflow checks.
The candidate builds as 1,798,992 bytes, application SHA-256
`10536ea4ef678259813309ecb14062f96018feaae613f5d12a8b88e196344a00`.
Its fresh full 4 MB backup passed the checksum and full flash comparison; all
four installed images verified at 230,400 baud. The saved-settings native boot
mounted SD, passed the HW SHA self-test and authorized the configured worker.
The first feed/wallet cycle passed 5/5 in 7,400 ms. Scratch peak was 22,220
bytes, final used bytes were zero and SDK fallback count was zero. Three
consecutive native cycles then passed with all feeds and wallet valid, SD
mounted, authorized pool and no faults. Minimum unused stack was 2,316 bytes;
scratch returned to zero after every cycle, peaked at 22,220 bytes and never
fell back to the SDK. The subsequent attempt failed after runtime Wi-Fi recovery: repeated socket writes
ran out of memory and `std::make_shared<JobRequest>` aborted at native observer
second 214.5, causing a software restart. The scratch pool itself returned to
zero without fallback, but its reservation left too little general heap. This
candidate is not accepted. The owner is
unavailable for a physical charger move and explicitly requests continued USB
testing while asleep; charger cold boot with SD remains open.

### Pool parser memory correction

The pool parser now borrows incoming messages and the 4 KB JSON document instead
of copying them. Share submission borrows the existing worker/job strings.
The CYD Stratum stack returns 3,260 bytes of the old reservation to the general
heap; PMMEM reports minimum free stack for each application task. Host sanitizer
checks include a near-limit whitespace-padded pool job, preserve the source
message and confirm that whitespace handling still works without trimming a copy.
The build passes as 1,779,760 bytes, application SHA-256
`696e07ebd0fc1d9a505a67744f917c14692d381bd088e6a9ec2135e2e471c9cc`.
Its fresh full 4 MB backup passed checksum and flash comparison; all four
installed images verified. Saved-settings boot restored statistics, mounted SD,
passed the HW SHA self-test and authorized the pool. Three consecutive feed/
wallet cycles passed with minimum ChainFeed stack 2,308 bytes, scratch peak
22,220 bytes, final used zero and no SDK fallback. Native PMMEM minima were
Monitor 5,908, Stratum 2,708, HW miner 1,924, SW miner 3,552 and loopTask 3,516
bytes. A non-erasing controlled disconnect recovered pool authorization in
44.047 seconds; another 72 seconds of observation showed no allocation fault or
restart. The fresh strict attempt then caught a 932.41 kH/s 10-second reading at native
second 327.637, during a feed cycle ending at 328.072. Its 60-second rate was
961.87 kH/s and no native fault/restart occurred, but the strict performance
gate failed. Both this timed attempt and the associated soak were stopped and
preserved. No performance PASS is claimed for this image. Acceptance monitoring now detects allocation aborts, reboots,
failed String allocations and nonfinite rates, which the earlier observer missed.
A separate evidence validator checks a complete 20 minutes, both rate windows,
independent work, sampling continuity, live serial coverage, pool/SD/wallet
progress and SHA/feed integrity. Its negative fixtures reject the previously
missed abort/restart, stale monitoring, counter reset, partial feeds and brief
out-of-band readings.

### Feed scheduling and measured stack reservations

An experimental correction gave ChainFeed priority 1, equal to the software miner,
instead of letting certificate work preempt it at priority 2. Monitor and SW
miner reservations shrink only from measured native minima: 9,500 → 6,144 and
5,000 → 3,072 bytes respectively, preserving over 2 KB and 1.5 KB of observed
unused stack. This returns another 5,284 bytes to general networking memory.
Job preparation also borrows its existing job strings. Its installed application
was 1,779,840 bytes, SHA-256
`e5ab82cfc02176b92c0f532246b1b409c23948496fa99b71057e466896bd795a`.
The full backup and all four written images verified. Saved-settings boot
mounted SD and authorized the pool, but both attempts for every HTTPS endpoint
reset during the handshake. Its feed smoke check timed out. This is a failed
experiment, not a performance or live-data pass; priority 1 is superseded by
the connection-reuse candidate described above.

### Recovery-script follow-up

The full-flash restore helper uses the pinned esptool 4.8.1 `write_flash`
command, takes a fresh full current-device backup, verifies that backup and
compares the restored image afterward. The historical stock-image flash helper
had retained `write-flash` and omitted the backup; it now uses the same verified
backup-before-write sequence and compares all five stock images, including the
filesystem. Both helpers use the root virtual environment and the stable USB
device link. Sixteen stubbed safety cases pass across success, short/read/verify
backup failures, invalid/missing inputs, write failures and final verification
failures. These tests perform no hardware write and are included in root CI.

### First shared-SPI installation

The owner requested reinserting the purchased card and updating the firmware.
The charger observer was preserved as interrupted, with 54 samples through
787 seconds. It did not complete the 20-minute gate. Around the requested card
change, HTTP and ICMP stopped responding and JNAP stopped listing an active
miner. This does not isolate card insertion, a power interruption or a firmware
fault; no USB serial connection was available to inspect the event.

Source inspection found a concrete peripheral ownership conflict: TFT_eSPI and
SDStore each constructed a separate `SPIClass(VSPI)` on different pins, while
touch already owned HSPI. The second VSPI owner could reinitialize the display's
hardware and had a separate transaction lock. The repair uses TFT_eSPI's public
`getSPIinstance()` for the card, fans VSPI clock/data out to GPIO18/23 without
detaching display pins GPIO14/13, and sets the common input to the SD's GPIO19.
Separate GPIO15/5 chip selects isolate display/card transactions. SD unmount
keeps the display controller running. The display deselects the card before its
first traffic; touch and its calibration remain on the existing HSPI path.

The checked Arduino SPI implementation returns early from `begin()` on an
already started instance and serializes transactions on that instance's lock.
The GPIO routing mechanism is also documented in Espressif's
[SPI reference](https://github.com/espressif/esp-idf/blob/v4.4/docs/en/api-reference/peripherals/spi_master.rst).
This repair addresses a source-level conflict; it does not establish the cause
of previous Wi-Fi failures.

The installed application builds for `ESP32-2432S028R`: 1,794,416 bytes, SHA-256
`161f02c2831ac32a2265dc5e49706757968e33fa5eca513f6fc72cd6dfbaf3e9`.
Startup now probes the shared SD bus before Wi-Fi, so the card can be checked
even when the saved network is out of range. The previous uninstalled build
`2a58b816…` was superseded by this startup change.
Production SDStore host checks cover shared ownership, retained display routes,
missing-card/retry paths, non-disruptive unmount, capacity, CSV records and
short-write accounting. Existing 132 regression cases and speaker tests pass.
The negative control compiles the previous production SDStore with the same
host boundaries and fails the shared-bus assertion; the repaired source passes.
The owner reconnected laptop USB with the card inserted and explicitly reported
that this location is out of Wi-Fi range. The flash helper read a fresh 4 MB
backup, checked its SHA-256, verified it against flash, then wrote and separately
verified the bootloader, partitions, boot-app selector and application. Saved
configuration partitions were preserved. An ordinary EN boot reported
`[SD] SD card mounted successfully` about 3.8 seconds after reset, before the
Wi-Fi join. No SD failure or directory warning, panic, brownout or heap assertion
was observed in the offline USB check. Touch initialization also completed;
this does not establish visual/touch coexistence after card use.

The USB observer was released for the owner's move downstairs. A fresh observer
waits for USB removal, router association, mounted SD and pool authorization,
then warms up the 10/60-second rates before starting a new 20-minute window.
It preserves API gaps and out-of-band observations, checks independent work
progress and SD rows, and downloads CSV after the timed window. Mounted
capacity, real downloaded CSV records, post-mount display/touch behavior and
the new 20-minute result remain pending that charger boot near the router.
An offline zero rate is not evidence of a failure to mine while connected.

## November 26: JNAP recovery and saved-settings restart

Authenticated JNAP confirmed that the stored 2.4 GHz SSID and Wi-Fi password
match the router, MAC filtering is disabled, and the stronger scanned AP is the
actual router. The router is an EA8300. Its local UI source supplied the API
names and request schema; radio settings come from `GetRadioInfo3`, not the
unsupported `GetRadioSettings` actions.

The owner moved the equipment as close as practical and unplugged/replugged the
miner. Airtime fairness was disabled using `SetAirtimeFairnessSettings`; readback
confirmed disabled. Applying it briefly interrupted router Wi-Fi. NetworkManager
subsequently reported full laptop connectivity. WPA2 remained configured. The
working configuration retains the disabled fairness setting, rather than making
another radio change during acceptance testing.

At the earlier recovery, the installed application was
`82dca8181f64c04be39835fc3db2ea94e5076265988061d77bd2affe51644548`.
A fresh EN boot joined the stronger AP with the first normal b/g/n profile,
authorized the existing wallet worker and loaded **5/5 Chain endpoints**.
Signal was -38 dBm; JNAP independently listed the miner on the 2.4 GHz radio.
Thirty-four early samples included 957.0–986.7 kH/s in the latest ten; SHA
verification showed 455 checked, zero mismatches and zero dropped candidates.
These are startup observations, not a completed performance gate. Movement,
USB power cycling and fairness changed together, so root cause remains
confounded; do not attribute recovery solely to the router option.

`PMSAVE` acknowledged `ESP_OK` with best difficulty `0.702206`. A subsequent EN
reset, without USB provisioning, rejoined at -34 dBm, restored the same best,
authorized the pool and loaded all five Chain endpoints. Later live readings
were near 966–991 kH/s. That validates restart with stored settings, but EN reset
is not a charger-powered cold boot. The user confirmed moving the unit to a USB charger near the router. The USB
device disappeared from the laptop, JNAP independently listed the miner on
2.4 GHz with its current lease, and its HTTP API reported pool authorization,
restored best `0.702206052`, and a 10-second rate of `999.4349976 kH/s`.
This verifies the charger-powered cold boot and mining without the laptop USB
connection. The initial 60-second average was still warming up and is not used
as a completed benchmark result.

The original serial 20-minute observer was stopped before completion for the
requested restart and charger check. A separate API observer attempted a
fresh charger-powered 20-minute run after six consecutive in-band 10/60-second
rate samples. It checks reported rates, pool authorization, API reachability and
the independent work-counter delta. Serial SHA/Chain evidence is limited to the
pre-unplug test; this API run does not establish continuous post-unplug SHA or
TLS-event monitoring. The first API attempt stopped after about 40 seconds because one status read
exceeded its three-second limit. JNAP still showed an associated miner and the
next API response was authorized at 966.96/975.61 kH/s (10/60-second windows).
That failed observation is retained; it does not establish a Wi-Fi disconnect.
The second attempt also timed out, with a ten-second status limit. A subsequent
response took 0.113 seconds, remained authorized, and showed increasing work at
972.77/977.43 kH/s. All 12 ICMP probes responded during that later check. Both
failed observations are retained. The continued work-counter observer was
interrupted for the requested SD update as recorded above. There is no new
20-minute PASS. At that point the installed code was
unchanged; the common success-path reconnection safeguard in `92a0f4d` was
build-checked but not flashed. It is included in the SD update above. The older
image's saved-network success path already enables
automatic reconnection and is the path exercised in the successful EN test.
Secrets, full responses containing credentials and device logs remain excluded
from Git; JNAP diagnostic output is projected and redacted.

## Current repair and physical SD test

The owner confirmed **SD removed and USB reconnected**. With the card absent,
the SD-isolation binary below joined the saved network, authorized its wallet
worker with the pool and mined. A subsequent EN reset restored best difficulty
`0.139651`, rejoined and recovered all five Chain endpoints without provisioning.
This confirms automatic recovery in that configuration; card removal and USB
power cycling happened together, so it does not isolate their individual effects.

Complete native frame captures showed live Stats and Chain data: RAM, sampled
CPU use, internal chip temperature, pool authorization, a nonzero best candidate,
price, network difficulty and next retarget. Frame dumps were outside the timed
performance window because their serial traffic interrupts the monitor task.

The first fresh Mine run **failed** after about 300 seconds. It recorded 281
rolling samples, a 415–1001 kH/s range, 16 below 944.3 kH/s, a Wi-Fi disconnect
and an incomplete Chain refresh. The board reconnected automatically, but this
is insufficient for unattended-use acceptance. The earlier successful join does
not mean Wi-Fi stability is solved.

Source repair `599e170` addresses verified application defects:

- `merkle_root[65]` wrote past a 65-byte array. ASan/UBSan reproduce the stack
  overrun in the previous production source; the repaired source passes.
- Fixed-size coinbase concatenation silently truncated inputs above 511 hex
  characters. SHA now streams all four fragments through a bounded 64-byte
  workspace. Six independent SHA256d fixtures exercise short/long transactions,
  complete 13/32-branch merkle paths and 80-byte header byte ordering.
- Per-job payload and merkle logging no longer block the release mining path.
- Work increments, compaction and read snapshots share a mutex. Monitor and
  watchdog receive a coherent count; API totals include the current job, and
  checkpoints include completed megahashes from that job.
- Normal b/g/n rates at moderate power are attempted first; legacy and other
  power profiles remain bounded fallbacks. This is a recovery change to test,
  not proof of the physical cause of the radio failures.
- Flash/restore use the stable USB device link across ttyUSB re-enumeration.

The production-code host suite passes **132 cases** under ASan/UBSan, plus the
separate speaker tests. Eight restore safety scenarios and workflow lint pass.
Host mining-data tests require OpenSSL development headers; CI installs them.

Repair application SHA-256:
`ccb973bcbaac03902626019a4bc7221d13b0a2f688a516f3b8a1b7a987930ded`
(1,791,680 bytes). The build preceded the source commit, so the embedded revision
can say `b4805cf-dirty`; identify the tested application by its full digest.
Installation and native acceptance results are recorded below when complete.
Raw logs, failed runs, captures, credentials and backup images remain ignored.
No release, public visibility change or push is cleared while acceptance is open.

## Radio retest and location change

The first post-repair boot and an EN reset both failed association with the
card physically absent. An explicit Espressif RF-calibration reset returned
`ESP_OK`, but the next join still failed. This rules out claiming that the
application buffer repair or calibration alone fixed the observed radio issue.
The official [ESP-IDF RF calibration guide](https://github.com/espressif/esp-idf/blob/v4.4/docs/en/api-guides/RF_calibration.rst)
describes the selective calibration reset; this test did not erase the app's
Wi-Fi, pool, wallet or statistics configuration.

RF-diagnostic-capable normal application SHA-256:
`e8cfc6427ec8ea1be002f0fadf5e0f9febbf3412857fd0b35c9c0c15a0c6c870`
(1,792,256 bytes; source `4126b7e`, embedded `599e170-dirty`).

The owner then disclosed moving the miner farther from the router so the laptop
could charge. That confounds the failed joins after the first successful run.
Native AP scans during failures returned signals between -62 and -80 dBm;
proximity, power and SD state have not been independently isolated. Distance is
plausible and remains a testable explanation, not an established root cause.

A separate, read-only Wi-Fi-only diagnostic was built and flashed after a
fresh verified backup. Its SHA-256 was
`8e09df2be97f4a7030fa8cee1d38e70395d68c473d17d44c50e512c1105c9b97`.
It disables the backlight and omits mining, touch/display SPI, SD access and
sound. It was interrupted after about 49 seconds to restore the normal firmware
when the owner moved the unit back toward the router. It had not joined, but
**did not complete all four profiles**; do not report an isolation-test pass or
failure from that partial run. The temporary diagnostic is documented under
`firmware/diagnostics/wifi-only/` and is not the normal device firmware.

The normal e8cfc642 application was restored through a fresh verified full
backup; all images passed verification. After the owner returned near the
router, association still failed. Two WPA2 access points advertised the saved
network at approximately -58 and -70 dBm on different channels. These are scan
observations, not proof of successful bidirectional radio traffic. A manually
pinned alternate-AP attempt used channel zero and returned `NO_AP_FOUND`; it
does not certify an explicit known-channel attempt.

The owner requires laptop USB power and cannot perform a different-supply test.
Power remains unisolated, not a demonstrated cause. No further power-source or
physical relocation test is required to continue the authorized software work.

The next normal application disables automatic reconnection before starting
STA mode, uses RAM for driver credentials while retaining the existing SPIFFS
application configuration, scans all channels, and retries each matching AP
on its detected channel. It starts with normal b/g/n at the SDK's maximum power
and includes the previously untested maximum-power 802.11b fallback. BSSIDs
are not saved in the application configuration, so a cold boot rescans after
moving. Authentication timeouts no longer label the password as likely wrong.
The normal retry application was built, installed after a fresh verified 4 MB
backup, and all four images passed verification. Installed application SHA-256:
`82dca8181f64c04be39835fc3db2ea94e5076265988061d77bd2affe51644548`
(1,794,400 bytes). The scan saw channel 5 at -63 dBm and channel 1 at -71 dBm.
All **eight** profile/AP attempts completed without joining: the native log
recorded three `AUTH_EXPIRE`, four `NO_AP_FOUND`, and one application-initiated
`ASSOC_LEAVE`. The configured maximum-power profile's driver readback was
15.25–15.75 dBm; requested power is not proof of radiated power.

A subsequent EN reset sent the saved settings with only the stronger AP's BSSID
and known channel selected. That separate **25-second** attempt timed out and
recorded two `AUTH_EXPIRE` events. Neither test establishes a wrong password,
a bad supply, or a defective board. Laptop USB remained the power source and
the SD card remained absent. No fresh 20-minute mining run was started because
there was no joined network or authorized pool session.

Final source review additionally restored automatic reconnection on successful
serial and Setup paths, which can bypass the saved-network loop. This final
source safeguard is build-checked but is not in the installed 82dca818 image;
the failed cold-boot test above exercises the same retry logic. No further flash
was started while the owner needed to move the laptop back to its charger.
The installed normal firmware remains in its network recovery path. Hardware
acceptance, SD reinsertion and publication remain held until an actual network
join and sustained mining can be verified. AP-side authentication evidence or a
controlled physical radio/network test is still needed to narrow the cause.

## Earlier SD-isolation build and failure

The current source was consolidated from firmware fork commit `bf08ec8` into
the parent repository as ordinary source files. There is no required submodule.
The earlier results below identify an older binary and do not certify this build.

Current application SHA-256:
`d2c601e8cb409714b72ba47269231b1e3e7900bb1bdca13e87b28f71934dd495`.

The SD-isolation install completed on November 25 at approximately 22:48 EST after a
fresh, verified 4 MB backup. Bootloader, partitions, boot app and application
all passed esptool verification. The backup is local and ignored.

Implemented and host-tested changes include:

- Manual pages with stable-press, full-release and cooldown filtering; no
  automatic rotation unless the diagnostic command explicitly enables it.
- Every pool-confirmed share counted separately from a valid block, and best
  difficulty updated from software-verified candidates.
- Correct compact target decoding and full 256-bit little-endian comparison.
  The inherited comparison read a pointer instead of the target and used an
  unsigned reverse loop; these were correctness bugs, not cosmetic changes.
- Bounded fragmented Stratum framing and subscription waits, real authorization
  state, and progress detection independent of the UI's rate accumulator.
- Stats with measured rate windows, CPU sampling, RAM, temperature, Wi-Fi,
  SD rows and SHA verification counts. Numeric placeholders no longer roll
  through fake intermediate zeroes.
- One SD filesystem owner, measured CSV rows, streamed downloads and protected
  config paths; enabled statistics use committed NVS checkpoints every 5 minutes.
- Quiet boot/reconnect and nonblocking, prioritized mining event sounds.

The production-code host suite passes 126 cases: touch 9, SD paths 30, progress
watchdog 7, targets 37 and Stratum 43, with AddressSanitizer/UBSan clean. Speaker
tests pass separately. Both themes render 29 fixtures, including Stats and a
reopened Setup. The CI workflow was linted and updated to run these checks.

On this binary the display/touch bus initializes and LVGL uses 33,328 of 45,056
bytes at boot. SD initialization is deferred until Wi-Fi connects; the SD config
fallback mounts on demand only when internal configuration is missing.
Before physical card removal, Wi-Fi saw the
configured AP but failed authentication/association. A backup comparison confirmed
the stored SSID/password match the private device configuration; values are not
published. Four bounded legacy/mixed-mode and power profiles were tried without
a successful initial join. This is unresolved; proximity alone does not diagnose
the cause. Pool acceptance, live Chain/Stats, SD CSV writes, saved-stat recovery
and the 20-minute hashrate gate must be rerun on this exact binary after recovery.
At this point, no unplug-and-replug success had been observed.

The user authorized a firmware-only SD isolation test while away from the unit.
Over the first 126 seconds, all four join profiles ran, with 43 authentication
timeouts, zero SD mount attempts, zero mining rate readings and zero Chain
refreshes. The test did not resolve the failure. It checks SD initialization and
access; the inserted card remains electrically connected, so its power draw has
not been excluded. That physical test was later completed as recorded above.
The unit continues its bounded join attempts and portal/restart recovery loop.
No release or public repository publication is authorized until the hardware
acceptance checks pass.

An independent host check used the settings recovered from the verified backup.
The saved Wi-Fi credentials match the private device configuration. The saved
wallet matches the previously recovered device wallet and passes its Bitcoin
address checksum. Statistics persistence is enabled. The pool accepted a host
subscription and authorization with those settings and returned difficulty and
mining work. No share was fabricated or submitted by this check. This verifies
the stored configuration and pool reachability from the computer; it does not
establish that the board is connected, mining or receiving a payout.

## Chain feed diagnosis and fix

The certificate verification failure came from exhausted/fragmented heap during
RSA verification. Instrumenting `mbedtls_pk_verify_ext` exposed error `-17040`:
the RSA public-key operation could not allocate its big-number workspace.
One failing attempt had 12,312 bytes free but a largest block of only 1,076 bytes.
The higher-level verifier reported untrusted certificate signatures instead.

The pinned USERTrust root matches the root tested successfully with host
mbedTLS 2.28.4. Pausing the hardware miner between SHA chunks still produced
zero successful endpoints. Forcing the Chain task to use software SHA also
failed with the same RSA allocation error. The software-SHA experiment was
removed.

The LVGL reservation is now 44 KB instead of 56 KB on ESP32, releasing 12 KB
to the general heap. The 64-bit renderer uses 80 KB instead of 96 KB. The
native build boots with 26,660 bytes used in its 45,056-byte LVGL pool.
Certificate verification, the pinned root and the miner's normal SHA engine
locking remain enabled.

With Mine running, LVGL used 32,528 bytes with a 12,124-byte free block.
The general heap had about 76,852 bytes free and a largest block of 49,140
bytes, leaving room for certificate verification.

The first on-device fetch after this change succeeded for all five endpoints
in 3,938 ms while the hardware miner was running. The board rejoined its
existing network after automatic retries; authentication remains intermittent.

## Screens

Both themes rendered all 26 original fixtures successfully with the smaller pool.
Fixtures include cold data, waiting for the feed, kH/s and MH/s, six-figure and
near-million prices, midnight, alarm states, and reopening Setup.

Hash, Chain and Clock share Mine's header, two columns, numeric styling and
compact charts. Boot and Setup share the same frame and colours. Reopening
Setup now loads the retained Boot screen and deletes its previous Setup before
allocating the replacement, keeping the smaller LVGL pool within its limit.

## Firmware and flash verification

The earlier firmware changes were committed in the fork as `f8c6ef3`. That build was
flashed before that commit was created, so its generated Git version can
still identify parent `0f0cc75`; use the application digest below to identify
the tested binary.

Application SHA-256:
`03da04d96ebb25658f316e85e569371d91773c992d35c04f815336e8b80591f4`.

The flash helper verified the fresh 4 MB backup against the device, then wrote
and independently verified the bootloader, partition table, boot app and
application. Private backups, serial diagnostics and captured device frames
remain in ignored `hardware/backups/` and `logs/` directories.

## Restore safety

`scripts/restore_full_flash_backup.sh` uses esptool's `write_flash` command.
It requires an explicit 4 MB image, checks its checksum sidecar when present,
and takes and verifies a unique full backup of the current device before
writing. It verifies the restored image before reporting success. Both flash
helpers invoke esptool through the project Python environment.

All eight stubbed safety tests pass: successful ordering, failed read, short
backup, failed current-image verification, invalid input, checksum mismatch,
failed write, and failed restored-image verification. These tests never open
a serial device.

## Mine acceptance

The Mine page ran continuously for **1,200.474 seconds**, from 21:06:51 to
21:26:52 EST on November 25 (02:06:51–02:26:52 UTC on November 26).
The baseline is 994 kH/s; the 5% band is 944.3–1043.7 kH/s. Framebuffer
captures were deferred until after the timed window.

| Measurement | Result |
| --- | --- |
| Mean of 1,160 reported rolling rates | 972.624 kH/s, 2.151% below baseline |
| Independent hardware candidate estimate | 960.979 kH/s, 3.322% below baseline |
| Reported rolling range | 932–1003 kH/s |
| Readings below / above the 5% band | 11 / 0 |
| Hardware candidates verified during the window | 17,603 |
| SHA mismatches / dropped candidates | 0 / 0 |
| Resets, crashes or TLS verification errors | 0 |

Both full-run averages are within 5% of baseline. **The stricter requirement
that every rolling reading remain within 5% did not pass**: 11 of the 1,160
readings fell below 944.3 kH/s, reaching 932 kH/s (6.24% below baseline).
The runner's original `FAIL` status for that per-reading gate is retained in
the ignored result file; it has not been changed to an unconditional pass.

The independent estimate uses the change in checked plus dropped prefilter
candidates, multiplied by 65,536 and divided by elapsed seconds. Candidate
counts are statistical, so this is a cross-check of the reported rate.
All 11 recorded Chain refreshes, including warm-up, fetched all five endpoints.
There were no incomplete refreshes during the timed window.

Raw results and the four native framebuffer captures stay in ignored
`logs/panicminer-final-bench/`. The capture sequence returns the device to Mine.

## Python environment audit

The scoped tools use `./.venv/bin/python`; required dependencies are declared
in the root `requirements.txt`. A repository text audit found one historical
system-interpreter reference in the old monitor notes; it now directs current
tools to the project environment. No matching active invocation remains in
the audited source and documentation. GSD decisions and session state are updated
through its CLI; the scoped audit is in `.planning/evidence/panicminer-runtime/`.
