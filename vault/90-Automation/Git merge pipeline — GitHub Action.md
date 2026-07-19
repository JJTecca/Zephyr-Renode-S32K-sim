---
tags: [automation]
---
# Git merge pipeline — GitHub Action

**Flow:** `git commit` → `git push` → PR merge into `main` (code repo) → Action fires → Claude API
turns the diff into a Markdown note wikilinked against `20-Concepts/` → note committed into this
vault repo → Obsidian Git plugin pulls → **the graph has grown, hands-free.**

Why it works: the graph is drawn purely from `wikilinks`; the Action's prompt receives the list of
concept filenames and may link to nothing else, so every merge note attaches to existing nodes
instead of spawning orphans.

Files live in the code repo: `.github/workflows/vault-note.yml` + `.github/scripts/make_note.py`
(full listings in the tutorial doc shipped alongside this vault). Secrets needed:
`VAULT_PAT` (fine-grained, contents:write on the vault repo) and `ANTHROPIC_API_KEY`.
Output lands in `40-Experiments/code-log/YYYY-MM-DD-<commit-slug>.md` — see the example note there.
Tune later: `paths:` filter (only `src/**`, `models/**`), skip on `[no-vault]` in the commit message.
