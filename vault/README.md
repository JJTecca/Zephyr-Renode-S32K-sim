# SDV Thesis Vault — Fault Prediction & Self-Healing (K1 edge → K3 zonal → cloud AI)

Open this folder in Obsidian: **Open folder as vault**. Then install community plugins:
**Git**, **Dataview**, **Templater**, **Zotero Integration**, **Kanban** (optional), **Excalidraw** (optional).
Enable Dataview's "Enable JavaScript queries" is NOT needed — all queries here are plain Dataview.

Start at [[Home]].

## Folders
- `00-MOCs` — maps of content (entry points into the graph)
- `10-Literature` — one note per paper (28 seeded, bot-extendable)
- `20-Concepts` — the controlled vocabulary; automation may ONLY link to these
- `30-Architecture` — architecture v2 + one note per figure + ADRs
- `40-Experiments` — experiment notes + `code-log/` (written by the GitHub Action)
- `50-Thesis` — chapter outline and drafts
- `60-Roadmap` — July→December plan, metrics, scope decisions
- `90-Automation` — the n8n and GitHub-Action pipelines, step by step
- `99-Daily` — daily logs
- `_templates` — Templater templates used by you and by the bots

## Automation contract (important)
Bots (GitHub Action, n8n) write Markdown into `10-Literature/` and `40-Experiments/code-log/`
and must wrap concepts in `wikilinks` **only from the filenames in `20-Concepts/`**.
That is the whole mechanism by which the graph grows automatically.
