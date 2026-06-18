---
phase: 01-re-spec-the-ui-read-only
reviewed: 2026-06-17T00:00:00Z
depth: standard
files_reviewed: 2
files_reviewed_list:
  - scripts/nmminer_str_xref.py
  - scripts/check_spec_screens.py
findings:
  critical: 0
  warning: 3
  info: 4
  total: 7
status: issues_found
---

# Phase 1: Code Review Report

**Reviewed:** 2026-06-17
**Depth:** standard
**Files Reviewed:** 2
**Status:** issues_found

## Summary

Both scripts were reviewed against correctness (string->l32r xref math, SPEC.md
structural assertions), the load-bearing D-11 no-redistribution boundary,
boundary robustness, and style match to `nmminer_ss_decode.py`.

**D-11 boundary: PASS (the load-bearing property holds).** Grep for the
grep-enforced markers (`share accepted`, `%ldms`, `accepted`, decompiled-body
markers) over both committed scripts returns nothing. The only firmware-derived
strings present are the 6 screen CLASS-NAME identifiers + `ss_render` (explicitly
allowed by D-11), plus VMA addresses and structural logic. No verbatim closed
firmware literals are hardcoded. The xref design deliberately requires
firmware literals to be supplied at runtime via CLI / gitignored `--extra-from`
file, which is the correct architecture for the constraint.

No Critical findings. Three Warnings (a real xref correctness gap on the +1-byte
fallback, a widget-row heuristic that silently mis-counts, and a file-handle
leak on the bin reads) and four Info items (style / robustness polish).

## Warnings

### WR-01: +1-byte SPrice fallback is too narrow and can mis-resolve

**File:** `scripts/nmminer_str_xref.py:45-55`
**Issue:** The xref does `irom.find(struct.pack('<I', drom_addr))` — the FIRST
occurrence of the 4-byte literal in `irom`. Two correctness gaps:

1. **First-match only.** A given DROM address literal may appear in the literal
   pool of more than one function, or the same 4-byte little-endian pattern may
   occur incidentally inside an unrelated instruction stream / literal. `find`
   returns only the first byte offset; the caller treats it as authoritative.
   For a screen-location tool whose whole purpose is "this literal -> that
   function," silently taking the first hit can point at the wrong function
   with no signal to the user. The proven method (CONTEXT.md §RE pipeline)
   expects a literal-pool hit, not an arbitrary first byte match.
2. **Fallback is a fixed +1, exact only.** Pitfall 3 is generalized in the
   docstring as "the literal may point one byte past the string start," but the
   code tries exactly `drom_addr` then exactly `drom_addr+1`. If a literal points
   into the middle of a longer string (e.g. a shared-suffix pointer, common in
   string-table packing), both probes miss and the tool reports `NOT FOUND` for a
   string that is, in fact, referenced. The +1 magic number also has no symbolic
   name.

**Fix:** At minimum, surface multiplicity instead of hiding it — find all
occurrences and report count, and make the fallback range explicit:
```python
def resolve_string(drom, irom, search_str):
    search_bytes = search_str.encode("ascii") if isinstance(search_str, str) else search_str
    off = drom.find(search_bytes)
    if off == -1:
        return None
    for delta in range(0, MAX_LITERAL_SKEW + 1):  # MAX_LITERAL_SKEW = 1 today
        drom_addr = DROM_VMA + off + delta
        needle = struct.pack("<I", drom_addr)
        hits = [m for m in _find_all(irom, needle)]
        if hits:
            return (drom_addr, [IROM_VMA + p for p in hits])  # caller prints all + a warning if >1
    return None
```
If first-match is intentionally sufficient for this RE workflow, document that
assumption in the docstring; right now the code presents a single answer as if
it were unique when it is not.

### WR-02: widget-row counting heuristic silently under/over-counts

**File:** `scripts/check_spec_screens.py:33-46`
**Issue:** `widget_row_count` is the gate behind the `>= MIN_WIDGET_ROWS` (5)
assertion for the Miner/Clock geometry tables — the core "SPEC is
evidence-bearing, not just headers" check (RE-03). The header-skip heuristic is
fragile:
- A row is treated as a header ONLY if it matches `\bslot\b|\bpurpose\b|\bwidget\b`
  AND contains `x` AND `y` (case-insensitive). If the table's header uses
  different column names (e.g. `| Field | X | Y | W | H | Confidence |` with no
  "slot/purpose/widget" word), the header row is counted as a data row,
  inflating the count by one.
- Conversely the `"x" in s.lower() and "y" in s.lower()` test is extremely loose:
  almost any English-text table row contains the letters x and y somewhere
  (e.g. a "Purpose: display hashrate" cell has neither, but
  "Purpose: hashrate display" + a "y"-bearing word would). A *data* row that
  happens to contain a slot/purpose/widget word plus stray x and y letters is
  silently SKIPPED, undercounting.
- The separator regex `^\|[\s:|-]+\|?$` is fine, but combined with the above the
  net count is unpredictable across reasonable table phrasings. Because this is a
  smoke check meant to fail loudly on a thin SPEC, a miscount in EITHER direction
  defeats its purpose (false pass on a thin table, or false fail on a good one).

**Fix:** Count data rows structurally and skip exactly the header by position,
not by content matching:
```python
def widget_row_count(sec):
    rows = [ln.strip() for ln in sec.splitlines() if ln.strip().startswith("|")]
    rows = [r for r in rows if not re.match(r"^\|[\s:|-]+\|?$", r)]  # drop separators
    # first remaining pipe-row is the header; the rest are data rows
    return max(0, len(rows) - 1)
```
This removes both the column-name dependency and the stray-letter false-skip.

### WR-03: bin files opened without a context manager (handle leak)

**File:** `scripts/nmminer_str_xref.py:70-71`
**Issue:** `drom = open(drom_path, "rb").read()` / `irom = open(...).read()`
leak the file objects (relying on GC/refcount to close them). The sibling style
reference `nmminer_ss_decode.py:71` does the same one-liner, so this matches
local style — but `check_spec_screens.py:57` already uses the bad pattern too
(`t = open(path).read()`). For a short-lived CLI this won't cause a runtime bug,
hence Warning not Critical, but it is the one place the two new scripts diverge
from clean boundary I/O. Note also `check_spec_screens.py:57` opens the SPEC in
the platform default text encoding; a SPEC.md containing non-ASCII (degree signs
in geometry, em-dashes) will raise `UnicodeDecodeError` on a non-UTF-8 locale.

**Fix:**
```python
with open(drom_path, "rb") as f: drom = f.read()
with open(irom_path, "rb") as f: irom = f.read()
```
And for the SPEC read, pin the encoding:
```python
t = open(path, encoding="utf-8").read()
```

## Info

### IN-01: docstring comments on self-evident code in `resolve_string`

**File:** `scripts/nmminer_str_xref.py:48-49`
**Issue:** The inline comment explaining the SPrice fallback is justified (it
encodes a non-obvious Pitfall-3 invariant, which CLAUDE.md explicitly permits).
Keep it. No change — flagged only to confirm it is the *correct* kind of comment
(hidden invariant) and not a self-evident-code violation.
**Fix:** None — leave as is.

### IN-02: `isinstance(search_str, str)` branch is dead for all real call sites

**File:** `scripts/nmminer_str_xref.py:35-38`
**Issue:** Every caller in `main()` passes `str` (CLI arg, `CORE_IDENTIFIERS`,
or stripped file lines). The `else: search_bytes = search_str` branch (bytes
path) is never exercised. Per CLAUDE.md "no abstraction for one-time operations"
/ "trust internal guarantees," this is speculative generality.
**Fix:** Drop the branch: `search_bytes = search_str.encode("ascii")`. If a
non-ASCII identifier is ever searched this will raise at the boundary, which is
acceptable (all identifiers are ASCII by construction).

### IN-03: inconsistent error-path style vs the sibling decoder

**File:** `scripts/nmminer_str_xref.py:66-68`
**Issue:** Existence is checked with `os.path.exists` then the file is opened
separately — a TOCTOU-flavored two-step (harmless here, but the decoder
`nmminer_ss_decode.py` just opens and lets the OSError propagate, which is the
leaner local style). Mixed styles across the two new scripts:
`check_spec_screens.py` correctly uses try/except OSError (line 56-59), while
`nmminer_str_xref.py` pre-checks with `os.path.exists`.
**Fix:** For style parity, drop the `os.path.exists` pre-check and wrap the reads
in `try/except OSError` like `check_spec_screens.py` does, or accept the
divergence as deliberate. Low impact.

### IN-04: `--all` over an empty/garbage bin prints only "NOT FOUND" with exit 0

**File:** `scripts/nmminer_str_xref.py:90-99`
**Issue:** If given empty or non-firmware bins, every identifier prints
`-> NOT FOUND` and the process still exits 0. A caller scripting around this
tool cannot distinguish "ran, found nothing" from success. Minor, since this is
an interactive RE helper, but a non-zero exit when zero of `--all` resolves
would make it CI-safe.
**Fix (optional):** Track a `found` count in the loop; `sys.exit(1)` if
`to_search` was non-empty and nothing resolved.

---

_Reviewed: 2026-06-17_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
