#!/usr/bin/env python3
"""Generate per-file codebase notes in the vault (deterministic, no LLM).

For every source file under the code dirs, write one Obsidian note under
`vault/70-Codebase/` containing:
  - the file's own header summary (Description / Layer),
  - dependency wikilinks (from #include and Renode @includes),
  - wikilinks to any existing vault concept whose name appears in the summary.

It wipes and rewrites the folder each run, so a CI job can keep the Obsidian
graph in sync with the code on every merge -- the graph grows as the code does.
"""
from __future__ import annotations
import re
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
OUT = REPO / "vault" / "70-Codebase"
CODE_DIRS = ["firmware", "sim", "scripts", ".github/workflows", ".github/scripts"]
EXTS = {".c", ".h", ".cpp", ".py", ".resc", ".repl", ".yaml", ".yml", ".robot", ".sh"}


def rel(p: Path) -> str:
    return p.relative_to(REPO).as_posix()


def slug(relpath: str) -> str:
    return relpath.replace("/", "-")


def source_files():
    for d in CODE_DIRS:
        root = REPO / d
        if not root.exists():
            continue
        for p in sorted(root.rglob("*")):
            if p.is_file() and p.suffix in EXTS:
                yield p


def extract_summary(text: str, stem: str) -> tuple[str, str]:
    """Best-effort (description, layer) from the file's header comment."""
    desc = layer = ""
    m = re.search(r"Description:\s*(.+)", text)
    if m:
        desc = m.group(1).strip(" */")
    m = re.search(r"Layer:\s*(.+)", text)
    if m:
        layer = m.group(1).strip(" */")
    if not desc:  # fall back to the first meaningful comment/docstring line
        for line in text.splitlines():
            s = line.strip().lstrip("#/*\"' ").strip()
            if len(s) > 10 and not s.startswith(("!", "-", "=")):
                desc = s
                break
    return desc, layer


def deps(text: str, basemap: dict[str, str]) -> list[str]:
    hits = []
    for m in re.finditer(r'#include\s+"([^"]+)"', text):
        b = Path(m.group(1)).name
        if b in basemap:
            hits.append(basemap[b])
    for m in re.finditer(r'@[\w./-]*?([\w.-]+\.(?:repl|py|resc))', text):
        b = Path(m.group(1)).name
        if b in basemap:
            hits.append(basemap[b])
    return sorted(set(hits))


def concept_links(text: str, concepts: list[str]) -> list[str]:
    low = text.lower()
    return [c for c in concepts if re.search(r"\b" + re.escape(c.lower()) + r"\b", low)]


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    for old in OUT.glob("*.md"):
        old.unlink()

    files = list(source_files())
    basemap = {p.name: slug(rel(p)) for p in files}
    cdir = REPO / "vault" / "20-Concepts"
    concepts = sorted(p.stem for p in cdir.glob("*.md")) if cdir.exists() else []

    for p in files:
        text = p.read_text(errors="ignore")
        relpath = rel(p)
        me = slug(relpath)
        desc, layer = extract_summary(text, p.stem)
        dep = [f"[[{s}]]" for s in deps(text, basemap) if s != me]
        con = [f"[[{c}]]" for c in concept_links(f"{desc} {p.stem}", concepts)]

        out = [f'---\ntags: [codebase]\nsource: "{relpath}"\n---', f"# {relpath}", ""]
        if layer:
            out += [f"**Layer:** {layer}", ""]
        out += [desc or "_(no header summary yet -- add a header comment)_", ""]
        if dep:
            out += ["## Depends on", " · ".join(dep), ""]
        if con:
            out += ["## Concepts", " · ".join(con), ""]
        (OUT / f"{me}.md").write_text("\n".join(out))

    print(f"wrote {len(files)} codebase notes -> {rel(OUT)}")


if __name__ == "__main__":
    main()
