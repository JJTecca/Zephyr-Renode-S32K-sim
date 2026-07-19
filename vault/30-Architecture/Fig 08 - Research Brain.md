---
tags: [architecture, figure]
---
# Figure 08 — Research Brain

## What it shows (one paragraph)
L7 · process, not runtime. Ingest (Zotero + Better BibTeX · arXiv/Semantic Scholar · W&B/[[MLflow]]) → this vault (MOCs · literature notes · ADRs · daily log · thesis draft) → export (LaTeX/Typst → diploma thesis · IEEE/SAE paper). Every thesis claim links back to a literature note and a W&B run — the provenance chain. The vault lives inside the platform monorepo under `vault/` (Git-synced); the platform mirrors ADRs into `docs/adr/`.

## Design decisions embodied here
- *(process plane — no runtime ADRs)*

## Open questions about this figure
-

## Experiments that validate this figure
-

## Changes log
- 2026-07 — initial version (v1 diagrams)
- 2026-07 — **v2 sync to the July system map** (S32N & S32K5 removed; no NPU on S32K3)
