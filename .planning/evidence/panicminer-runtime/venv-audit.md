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
