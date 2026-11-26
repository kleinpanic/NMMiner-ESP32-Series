# Firmware host regression tests

Run `bash tools/tests/run.sh` from this firmware checkout after its PlatformIO
dependencies have been installed. The runner uses GCC/Clang C++17, AddressSanitizer
and UndefinedBehaviorSanitizer. Binaries stay under ignored
`tools/uisim/build-tests/`.

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
five real seconds. All worker/password values are synthetic fixtures.

The SD path tests exercise canonical paths, repeated normalization, and rejection
of traversal, control characters and ambiguous separators. The pool watchdog
tests exercise cumulative progress at different sampling phases, actual stalls,
resets, recovery, timer rollover and work-counter rollover.

The mining target tests decode the production compact target helper against
genesis and current target vectors, truncated small exponents, the largest
non-overflowing exponents, signed/zero/overflow rejection, and exact/below/above
256-bit thresholds. Comparisons include conflicting high and low bytes to catch
an accidental endianness reversal.

These tests do not establish radio reliability, physical touch calibration,
display operation, or a real pool accepting a share. Native hardware validation
is required for those claims.
