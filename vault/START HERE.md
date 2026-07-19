# START HERE — how this vault works

This vault mirrors your thesis architecture, not a generic notes dump. One rule governs everything:

> **Every claim in the thesis must trace back to either a literature note or an experiment note.**

If you maintain that link discipline for a year, writing the thesis becomes assembly, not archaeology.

## The folders

| Folder | What lives here | When you touch it |
|---|---|---|
| `00-MOCs` | Maps of Content — index notes that link everything on a topic | Weekly, when reviewing |
| `10-Literature` | One note per paper, in **your own words** | Every time you read a paper |
| `20-Concepts` | Atomic notes — one idea per note ("denoising autoencoder", "ASIL-D") | Whenever a concept recurs across 2+ papers |
| `30-Architecture` | One note per figure + Architecture Decision Records (ADRs) | Whenever you make/change a design decision |
| `40-Experiments` | One note per experiment: hypothesis → config → result | Every experiment, no exceptions |
| `50-Thesis` | Chapter drafts that later export to LaTeX | From month ~3 onward |
| `99-Daily` | Daily log: tried / broke / next | Every working day, 3 minutes max |
| `_templates` | Note templates (wire these into the Templates core plugin) | Setup only |

## Setup checklist (do this now, ~20 minutes)

1. **Settings → Core plugins**: enable **Templates**, **Daily notes**, **Backlinks**, **Outgoing links**.
2. **Settings → Templates**: set template folder to `_templates`.
3. **Settings → Daily notes**: set new-file location to `99-Daily`, template to `_templates/tpl-daily`.
4. **Settings → Files & Links**: set "Default location for new notes" to *Same folder as current file*; turn ON "Automatically update internal links".
5. **Community plugins** (Settings → Community plugins → Browse), in priority order:
   - **Obsidian Git** — auto-backup the vault to a private GitHub repo. Non-negotiable for a thesis.
   - **Zotero Integration** — pulls paper metadata + PDF annotations into `10-Literature`.
   - **Dataview** — the MOCs and Experiment Index in this vault already contain Dataview queries; they light up once this is installed.
   - **Kanban** — for `50-Thesis/Milestones Board`.
   - *(later, optional)* Excalidraw, Templater.

## The three habits that make it "high quality"

1. **Link, don't file.** When writing any note, ask "what does this connect to?" and make `[[wiki-links]]`. The graph is the value; folders are just storage.
2. **Your words only** in literature notes. Copy-pasted abstracts are worthless in month 10. Three sentences you wrote beat three paragraphs you pasted.
3. **Close every day with the daily note.** What worked / what broke / next step. This is your experiment provenance and your "what was I doing?" recovery tool after any interruption.

Start by opening [[00-MOCs/HOME]].
