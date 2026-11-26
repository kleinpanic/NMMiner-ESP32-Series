# Firmware host regression tests

Run `bash tools/tests/run.sh` from this firmware checkout after its PlatformIO
dependencies have been installed. The runner uses GCC/Clang C++17, AddressSanitizer
and UndefinedBehaviorSanitizer. Binaries stay under ignored
`tools/uisim/build-tests/`.
Linux hosts also need the OpenSSL development package (`libssl-dev`). CI installs
it with the system C++ build dependencies; no Python crypto package is used.

The touch tests compile the production `src/touchPager.h`: stable presses,
held fingers, short loss of contact, full release, cooldown, direction changes,
the center dead band, invalid coordinates and the 32-bit clock rollover.

The Stratum tests compile the production `src/stratum.cpp` with the same
ArduinoJson dependency used by the firmware. Small stubs replace only the host's
missing Arduino serial and Wi-Fi interfaces. Scenarios include boolean pool
acceptance/rejection, server errors, malformed JSON, method routing, subscription,
authorization IDs and credential log redaction, parsed jobs and difficulty, and
eight-digit submitted nonces. A deterministic clock and scheduled TCP fragments
exercise subscription retries, early difficulty notices, unrelated response IDs,
near-deadline delivery, truncated frames and disconnected sockets without waiting
five real seconds. A near-limit padded job checks whitespace handling and unchanged
input while compile-time signatures prevent redundant message/document copies.
All worker/password values are synthetic fixtures.

The SD path tests exercise canonical paths, repeated normalization, and rejection
of traversal, control characters and ambiguous separators. The API's production
guard also rejects case variants of private configuration/secrets paths,
trailing-dot/space aliases and tilde short-name aliases. Public filenames retain
their case and internal spaces. The pool watchdog
tests exercise cumulative progress at different sampling phases, actual stalls,
resets, recovery, timer rollover and work-counter rollover.

The mining target tests decode the production compact target helper against
genesis and current target vectors, truncated small exponents, the largest
non-overflowing exponents, signed/zero/overflow rejection, and exact/below/above
256-bit thresholds. Comparisons include conflicting high and low bytes to catch
an accidental endianness reversal.

The mining-data tests compile the production `src/utils.cpp`, replacing only
the platform SHA API with OpenSSL. Six independent SHA256d fixtures check short
and long coinbases, 13/32-level merkle paths and full 80-byte header byte ordering.
The old production code fails with a stack-buffer-overflow at its merkle-string
terminator; the repaired code passes under ASan/UBSan. The long fixtures also
detect the former silent 512-character coinbase truncation. Release builds must
prepare jobs without dumping payloads to the blocking serial port.

Mining-batch tests check exact completed-work accounting and cancellation at
bounded publication intervals. The continuous-job policy also runs against a
real shared-pointer queue: temporary queue exhaustion preserves the nonce
cursor, while a stopped or replaced full generation discards old work, including
generations with equal low bytes. These checks do not establish native speed.

The Wi-Fi packet-budget tests check Arduino's default dynamic counts, preservation
of lower/static limits, unlimited RX conversion and repeated application. They
do not establish RF reliability or SDK memory behavior on the board.

The bounded-transfer tests exercise the production SD-download block loop with
exact byte comparisons, failed/invalid reads, partial/invalid socket writes and
disconnects. They verify that no-progress I/O terminates; native downloads are
still required to establish SPI, Wi-Fi and display coexistence.

The SD-store tests compile production `src/sdstore.cpp` with host filesystem and
SPI boundaries. They verify that SD uses the display's existing SPI object,
retains its pin routes, survives mount failures and retries, and never stops the
display bus when unmounting. They also check capacity, CSV headers/rows and
failed-write accounting. They cannot prove electrical pin routing or card
compatibility; those require a native test with the actual card inserted.

The runtime Wi-Fi tests cover retry spacing, the initial association grace
period and clock wraparound. Wallet tests cover worker-suffix removal, address
shape validation and checked signed satoshi arithmetic. TLS record-buffer tests
exercise two-slot allocation, repeated zeroed reuse, exhaustion, multiplication
overflow, foreign-pointer release and unequal buffer sizes. The latter also
passes a separate C++11 sanitizer build matching the board's language level.
These allocation tests do not replace repeated native HTTPS/SD operation or
minimum-unused-stack checks.

The TLS scratch tests compile the existing LVGL TLSF allocator and the
production scratch wrapper under C++11/ASan/UBSan. They run 1,000 differently
sized allocation/free cycles, check zeroed reuse and pool integrity, and cover
coalescing, exhaustion, invalid initialization, ownership and overflow. Unused
allocator entry points are discarded at link time as in firmware; no allocator
stub replaces the implementation.

These tests do not establish radio reliability, physical touch calibration,
display operation, or a real pool accepting a share. Native hardware validation
is required for those claims.

The Chain HTTP tests compile the production transport helper with small HTTP/TLS
boundary fakes. Nine scenarios check one fresh-connection transport retry,
bounded repeated failure, HTTP 404/429/503 without blind retries, begin failure,
empty-body rejection, response cleanup and retention of the successful result.
The actual pinned-CA path is exercised by repeated native feed/wallet cycles.

SD transfer policy cases cover temporary low-memory wait/resume, exact heap/block
bounds, a closed descriptor and the total deadline. Native checks additionally
exercise client cancellation, busy-download refusal and status queries during
an export; host tests alone do not establish SDK socket behavior.
