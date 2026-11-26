# Python environment audit — 2026-11-25

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
