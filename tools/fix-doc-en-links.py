#!/usr/bin/env python3
"""Fix relative paths to doc/en/ from each markdown file."""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def doc_prefix(path: Path) -> str:
    depth = len(path.relative_to(ROOT).parts) - 1
    return "../" * depth + "doc/en/"


def fix_file(path: Path) -> bool:
    text = path.read_text(encoding="utf-8")
    prefix = doc_prefix(path)
    new = re.sub(r"(?:\.\./)+doc/en/", prefix, text)
    new = re.sub(r"(?<!\w)doc/((?:0[0-9]|1[0-8])-[^)\s#]+|Glossary|Bibliography)", rf"{prefix}\1", new)
    if new != text:
        path.write_text(new, encoding="utf-8")
        return True
    return False


def main() -> None:
    changed = 0
    for path in sorted(ROOT.rglob("*.md")):
        if path.parts[-2:] == ("doc", "en") or "doc/fr" in str(path):
            continue
        if fix_file(path):
            changed += 1
            print(path.relative_to(ROOT))
    print(f"Updated {changed} files")


if __name__ == "__main__":
    main()
