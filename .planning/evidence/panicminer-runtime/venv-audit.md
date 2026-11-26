# Python environment audit — 2026-11-25

November 26 portal/failover, RGB and native egress follow-up: build, full-backup
installation, host tests, private-value scans and device qualification all used
the existing root `./.venv/bin/python`. The tracked-source invocation audit found
zero forbidden interpreter/install patterns. No Python dependency or environment
change was needed. The local suite passes 108 tests and 29 subtests; public CI
passes 107 tests and 29 subtests with one explicit private-image skip.

November 26 public CI follow-up: `tests/test_ss_decode.py` now uses the root
virtual-environment shebang and adds independently constructed public format
fixtures. No dependency changed. All Python execution used `./.venv/bin/python`.
The updated local suite passes 108 tests and 29 subtests; an isolated public
decoder checkout passes six tests with one explicit private-image skip. A
requested missing or incomplete stock-image directory remains a hard failure.
The scoped forbidden-invocation scan of the changed test is clean.

All host Python invoked during this work used the parent `./.venv/bin/python`.
The firmware build used `../../../.venv/bin/python -m platformio` from the fork.
Root requirements pin esptool 4.8.1, pyserial 3.5, Pillow 13.0.0 and PlatformIO
6.1.19. The consolidated root CI installs those requirements into `.venv` with
uv and invokes PlatformIO through that interpreter. Historical firmware CI is
retained under the source directory; root CI is the active workflow.

A recursive text check of tracked source and documentation for forbidden system
interpreter/install patterns returned no matches after the historical monitor
notes and CI were corrected. Ignored environments, build output, secrets, logs,
backups and upstream experimental checkouts were excluded. The pre-existing
untracked `build_fidb_examples.py` was outside this change and left untouched.
No system Python package installation was performed.

The job-buffer repair repeated the source/document scan with zero forbidden
invocation matches. Its PlatformIO build, regression orchestration, device
reader, backup/flash and benchmark tools all use the root `.venv` interpreter.
OpenSSL is a host C++ test dependency installed by the Linux package manager;
no Python package was installed outside the project environment.

The November 26 SD/TLS and recovery follow-up repeated the tracked-source
audit with zero forbidden invocation patterns. All native observers, backup/
flash operations, private scans and build orchestration used the root
`./.venv/bin/python`. Restore and historical stock-flash safety tests use shell
stubs at a sandboxed `.venv/bin/python` path and never invoke system Python.

The parser-memory repair adds the standard-library-only endurance evidence
validator, native serial parser and API sampler with fourteen negative/positive fixture tests. These run through the root
virtual environment, including CI. The tracked-source invocation audit passes;
no dependency or system-environment change was needed.

The reusable native capture CLI also uses root-venv pyserial (already pinned in
requirements.txt), restricts artifacts to ignored logs/ and needs no new package.
The API sampler uses only the standard library, selects measured fields without
wallet values or saved settings, and runs through the same root environment.

The full Python suite exposed undeclared dashboard/test dependencies in the
root environment. Flask, requests, pytest and pytest-cov are now pinned in root
requirements; the historical tests requirements file includes that source of
truth. uv installed them only in the existing root `.venv`. The dashboard
launcher now resolves the root and executes its virtual-environment Python,
including when called from another directory. A real local startup passed.
All 91 Python tests and 15 subtests pass; dashboard statement coverage is 95%.
The tracked-source forbidden-invocation audit again found zero matches.
These host results do not establish native hardware acceptance.

Factory packaging now consumes PlatformIO's resolved upload manifest, including
the SDK's boot_app0 image. Five root-venv tests exercise actual packaging code,
missing images, invalid layouts and alternate app offsets without serial access.
The full root-venv Python suite passes 96 tests and 22 subtests. This change uses
only the standard library and introduces no environment or dependency change.

The download-completeness repair also uses only the standard library. Its full
root-venv suite passes 99 tests and 27 subtests, including truncated CSV and
invalid Content-Length rejection. All build, flash, capture and qualification
commands continue to use the root virtual environment.

The feed-window checker repair uses only the standard library and adds two
regressions for out-of-window and untimestamped feed evidence. The complete
root-venv pytest suite and unittest discovery pass. A repeated tracked-source
forbidden-invocation scan found zero matches; no dependency or environment
change was needed. Builds, capture, installation and qualification continue
through the root virtual environment.

The native source-port reuse checker uses only the standard library and runs
through the root virtual environment. Four real host TCP fixture tests cover
complete JSON/CSV, invalid and truncated responses, EOF timing and socket
cleanup without retaining wallet values. The complete root-venv Python suite
passes 105 tests and 29 subtests. A scoped forbidden-invocation audit of the new
checker, tests, root requirements and CI found zero matches; no dependency or
environment change was needed. Native behavior is checked separately.
