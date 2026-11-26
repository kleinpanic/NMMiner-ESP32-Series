# Contributing

The canonical source is this repository. Firmware changes belong in
`firmware/custom/NerdMiner-CYD-Custom/`; the former nested fork is historical.
The supported, tested unit is the ESP32-2432S028R with the panel configuration
recorded in PlatformIO. Other board definitions inherited from upstream do not
imply hardware validation here.

Reuse the root `.venv`, or create it with `uv venv .venv` on a fresh checkout.
Install dependencies with
`uv pip install --python ./.venv/bin/python -r requirements.txt`.
Run from the repository root:

```bash
./.venv/bin/python -m platformio run -d firmware/custom/NerdMiner-CYD-Custom -e ESP32-2432S028R
bash firmware/custom/NerdMiner-CYD-Custom/tools/tests/run.sh
bash tests/test_restore_full_flash_backup.sh
```

The C++ host suite requires a compiler and OpenSSL development headers. UI
rendering additionally needs libpng development headers:

```bash
make -C firmware/custom/NerdMiner-CYD-Custom/tools/uisim .deps/lvgl/lvgl.h
make -C firmware/custom/NerdMiner-CYD-Custom/tools/uisim THEME=1 render
make -C firmware/custom/NerdMiner-CYD-Custom/tools/uisim THEME=2 render
```

Record the installed image digest, power source, SD state, measured results and
remaining limitations in the runtime validation document. Host tests and renders
do not establish radio reliability, native touch behavior or pool acceptance.
Keep framebuffer dumps outside timed hashrate measurements.

Use the flash/restore helpers, which require a fresh verified full-flash backup
before writing. Never commit real Wi-Fi or pool credentials, wallet values,
licenses, raw device logs, backups or generated firmware. Keep fixture data
synthetic and retain upstream copyright and license notices.
