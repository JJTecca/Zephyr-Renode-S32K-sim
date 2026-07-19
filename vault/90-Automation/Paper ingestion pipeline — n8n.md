---
tags: [automation]
---
# Paper ingestion pipeline — n8n (the scheduler that grows the graph)

**Flow (nightly):** Cron → arXiv + Semantic Scholar APIs → dedupe → Claude relevance-scores each
abstract → survivors become literature notes wikilinked against `20-Concepts/` → GitHub node commits
them into `10-Literature/` → Obsidian Git pulls → new paper nodes appear already wired into the graph.

Node-by-node:
1. **Schedule Trigger** — daily 07:00.
2. **HTTP Request (arXiv)** — `http://export.arxiv.org/api/query?search_query=(all:"software-defined vehicle" OR all:"fault prediction" OR all:"anomalous sound detection" OR all:"tinyml") AND cat:eess.SY OR cat:cs.LG&sortBy=submittedDate&max_results=20` (Atom XML → XML node to JSON).
3. **HTTP Request (Semantic Scholar)** — `/graph/v1/paper/search?query=cross-ECU fault prediction self-healing&fields=title,abstract,year,externalIds,url&limit=20`.
4. **Merge + Code (dedupe)** — key = arXiv id / DOI; store seen keys in workflow static data.
5. **HTTP Request (Claude #1, filter)** — send title+abstract+thesis one-liner; demand strict JSON `{relevant, score, reason}`; **IF node** drops score < 6. This gate is what keeps the graph curated.
6. **HTTP Request (Claude #2, note)** — prompt includes the `20-Concepts/` filename list; demand a full literature note: YAML frontmatter (`title, authors, year, arxiv, status: unread, score, tags: [paper]`), 5-sentence summary, "Relevance to my thesis" paragraph, wikilinks only from the list.
7. **GitHub node — Create file** in `10-Literature/<year>-<firstauthor>-<slug>.md` on the vault repo.
No scraping of Perplexity or search UIs: fragile, against ToS, and unnecessary — arXiv/S2/OpenAlex are clean structured APIs.
Reading flow afterwards: the [[Home]] Dataview queue surfaces `status: unread` sorted by score;
when you deeply read one, add it to Zotero and upgrade the note via the Zotero Integration plugin.
