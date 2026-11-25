---
phase: 01-re-spec-the-ui-read-only
type: security-audit
asvs_level: 1
block_on: high
status: PASS
threats_total: 6
threats_closed: 6
threats_open: 0
audited: 2026-06-17
---

# Phase 1 Security Audit — RE-Spec the UI (read-only)

**Verdict: PASS.** All 6 declared threats CLOSED. No HIGH findings. Phase may ship.

Read-only RE phase. Sole load-bearing control is the no-redistribution boundary
(D-11). Every mitigation was verified by running the declared check against the
implemented artifacts on disk — not by reading intent.

## Threat Verification

| Threat ID | Category | Disposition | Verdict | Evidence |
|-----------|----------|-------------|---------|----------|
| T-01-01 | Information Disclosure | mitigate | CLOSED | `grep -RnE 'share accepted\|%ldms\|FUN_400\|local_[0-9a-f]+ =\|undefined4 '` on both committed scripts returns empty (exit 1). `CORE_IDENTIFIERS` (nmminer_str_xref.py:24-32) holds only the 6 screen class names + `ss_render`. Runtime-supply contract enforced: positional arg / `--extra-from <file>` (lines 82-96); no firmware literal baked in. |
| T-01-02 | Information Disclosure | mitigate | CLOSED | `git ls-files hardware/recovered-assets/` == 0; `git diff --cached` shows no recovered-assets path. `git check-ignore` confirms SPEC.md, WORKBENCH.md, *.bin, *.elf, xref-extra-strings.txt all ignored via .gitignore:25. Ghidra project at `~/ghidra_projects` (outside repo); no `.gpr`/`.rep` tracked. |
| T-01-SC | Tampering (supply chain) | mitigate | CLOSED | WORKBENCH.md (on disk, gitignored, 29 lines) records provenance rows for Ghidra 11.3.1 + dynacylabs v11.3.1: URL + release tag + asset filename + SHA256 + install path. Both from official GitHub orgs (NationalSecurityAgency, dynacylabs). "v12.0 intentionally not used" + "yath/ghidra-xtensa not installed" notes present (stale-ref guard). |
| T-01-03-01 | Information Disclosure | mitigate | CLOSED | SPEC.md gitignored (.gitignore:25), markdown-only, never `#include`'d. RE-04 smoke: 0 tracked + 0 staged under hardware/recovered-assets/. |
| T-01-03-02 | Information Disclosure | mitigate | CLOSED | Full Plan-03 leakage scan `grep -RnE 'FUN_400\|local_[0-9a-f]+ =\|undefined4 \|lv_obj_set_pos(\|share accepted\|%ldms'` (with comment-line exclusion) on both scripts returns empty. check_spec_screens.py embeds only section/class identifiers + structural regexes. |
| T-01-03-03 | Tampering (mis-carved noise) | accept | CLOSED | Accepted risk. Disposition `accept` per plan; mis-carved `lvgl_*.png`/`jpg_*` are inert local files under gitignored hardware/recovered-assets/, documented in SPEC.md as NOT recovered art. No mitigation expected; documentation-only. Logged here as the accepted-risk record. |

## License / IP

- No closed firmware binary or modified image is committed anywhere:
  `git ls-files | grep -iE '\.(bin|elf|ss)$|app_factory|drom|irom|cyd-backup'` returns empty.
- No license-verification code path touched (N/A — read-only, no firmware written or flashed).
- Only the two declared identifier-only scripts are new committed code
  (`scripts/nmminer_str_xref.py`, `scripts/check_spec_screens.py`); both pass the literal scan.

## Functional Confirmation (mitigation actually works, not just absent)

The identifier-only contract was confirmed live, not just by absence-of-string:
- `nmminer_str_xref.py Miner320x240` -> 0x400d1960; `Clock320x240` -> 0x400d1864 (reproduces known addresses).
- `--all` resolves all 6 screen class names + `ss_render` to IROM addresses, including the
  SPrice +1-byte offset case — using only the baked identifiers, with zero firmware literals committed.

## Unregistered Flags

None. No new attack surface appeared during implementation beyond the registered threats.
Phase introduces no network, device-write, credential, or untrusted-input surface (read-only RE).

## Commands Run

```
grep -RnE 'share accepted|%ldms|FUN_400|local_[0-9a-f]+ =|undefined4 ' scripts/nmminer_str_xref.py scripts/check_spec_screens.py
grep -RnE 'FUN_400|local_[0-9a-f]+ =|undefined4 |lv_obj_set_pos\(|share accepted|%ldms' scripts/check_spec_screens.py scripts/nmminer_str_xref.py | grep -v '^[^:]*:[0-9]*: *#'
git ls-files hardware/recovered-assets/ | wc -l        # 0
git diff --cached --name-only | grep '^hardware/recovered-assets/'   # none
git check-ignore hardware/recovered-assets/{SPEC.md,WORKBENCH.md,app_factory.bin,drom.bin,irom.bin,nmminer-app.elf,xref-extra-strings.txt}
git ls-files | grep -iE '\.(bin|elf|ss)$|app_factory|drom|irom|cyd-backup'   # none
grep -nE -i 'sha256|11\.3\.1|v11\.3\.1|dynacylabs|https?://' hardware/recovered-assets/WORKBENCH.md
python3 scripts/nmminer_str_xref.py drom.bin irom.bin Miner320x240   # 0x400d1960
python3 scripts/nmminer_str_xref.py drom.bin irom.bin --all          # 6 screens + ss_render
```
