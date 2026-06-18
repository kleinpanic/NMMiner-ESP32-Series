# Phase 08 — Security Findings (to fix in 08-06 hardening)

Source: automated commit security review of 08-05 dashboard proxy commits, 2026-06-03.
File: `apps/nmminer-control/server.py`. All MEDIUM. Fix in plan 08-06 (security/optimization) since 08-05 owns server.py during its run.

1. **Path traversal pass-through** on `/api/sd/list`, `/api/sd/download`, `/api/sd/delete` — `path` from `request.args` proxied straight to the miner.
   Fix: server-side validation before proxying — require leading `/`, reject any `..` segment and `\x00`, normalize with `posixpath.normpath`, constrain to an allowlisted SD subtree. Minimum guard: `if not path.startswith('/') or '..' in path.split('/') or '\x00' in path: abort(400)`. Require CSRF/confirmation on delete.

2. **Reflected XSS via proxied `Content-Type`** on download — upstream content-type is echoed to the browser.
   Fix: always override `Content-Type: application/octet-stream`, add `X-Content-Type-Options: nosniff`, force `Content-Disposition: attachment`. Do not trust the miner's headers for arbitrary downloads.

3. **Content-Disposition filename smuggling** — fallback filename interpolated from `path.split('/')[-1]` without sanitization.
   Fix: `werkzeug.utils.secure_filename` (or whitelist `[^A-Za-z0-9._-]` → `_`, reject CR/LF) and wrap in RFC 6266 form.

Defense-in-depth note: firmware-side `sdstore::sanitizePath()` (08-03) is the primary guard; the proxy must ALSO validate (boundary validation at every layer).
