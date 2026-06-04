# 08-07 SUMMARY — Public GitHub Fork

**Status:** Repo created + pushed; currently set PRIVATE per user ("don't post it til validated").

## Done
- Created `kleinpanic/NerdMiner-CYD-Custom` and pushed `main` (full fork history + our 8 phase commits) + tag `v1.0.0`.
- Remotes: `origin` = kleinpanic/NerdMiner-CYD-Custom, `upstream` = BitMaker-hub/NerdMiner_v2 (for future upstream merges).
- MIT license retained; `NOTICE` attributes BitMaker-hub and lists our additions.

## Visibility
- Initially pushed PUBLIC, then set **PRIVATE** via `gh api -X PATCH ... -f private=true` after the user asked to hold until on-device validation passes.
- **Re-publish step (pending):** after 08-08 live validation passes, flip back to public:
  `gh api -X PATCH repos/kleinpanic/NerdMiner-CYD-Custom -f private=false`

## Secret safety
- Full git-history secret sweep clean before push (see 08-06). No credentials/wallet in history.
