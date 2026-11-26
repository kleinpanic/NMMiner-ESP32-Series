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
validator and nine negative/positive fixture tests. Both run through the root
virtual environment, including CI. The tracked-source invocation audit passes;
no dependency or system-environment change was needed.
