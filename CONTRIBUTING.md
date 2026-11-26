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
bash tests/test_flash_v2_0_02.sh
./.venv/bin/python -m unittest discover -s tests -p test_miner_endurance.py
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

Stop other serial consoles before native capture. The capture tool opens the
port without a reset unless explicitly requested, preserves an exclusive raw
capture and writes parsed `result.json` once per second. A fresh directory is
required; output is restricted to ignored `logs/`:

```bash
./.venv/bin/python scripts/capture_miner_runtime.py --out logs/<run> --reset --allow-commands
```

With `--allow-commands`, writing `PMMEM`, `PMNEXT`, `PMMINE` or `PMFEED` plus a
newline to `logs/<run>/command.txt` sends that diagnostic through the existing
serial owner. Avoid `PMDUMP` during performance measurements. Ctrl-C stops
capture without erasing or reconfiguring the miner.

`scripts/check_miner_endurance.py` independently validates the API sample JSON,
native observer JSON and raw serial log from a qualification run:

```bash
./.venv/bin/python scripts/check_miner_endurance.py logs/<run>/usb-endurance.json logs/<run>/result.json logs/<run>/serial.log
```

It requires a full 20 minutes, both rolling rates and the work-counter mean
within 5% of 994 kH/s, continuous samples/native coverage, one observed boot,
pool authorization, SD record progress, wallet refreshes and complete feed/SHA
checks. An abort, restart or allocation failure invalidates the attempt even
if the observer's fault array missed it. Keep all three input artifacts private;
the validator prints only check results and summary measurements.

Use the flash/restore helpers, which require a fresh verified full-flash backup
before writing. Never commit real Wi-Fi or pool credentials, wallet values,
licenses, raw device logs, backups or generated firmware. Keep fixture data
synthetic and retain upstream copyright and license notices.
