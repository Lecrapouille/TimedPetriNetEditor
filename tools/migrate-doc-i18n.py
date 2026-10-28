#!/usr/bin/env python3
"""Move doc/*.md to doc/en/, fix asset paths, add language switcher, symlinks."""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DOC = ROOT / "doc"
EN = DOC / "en"
FR = DOC / "fr"

ICON_FR = "https://lecrapouille.github.io/icons/fr.png"
ICON_EN = "https://lecrapouille.github.io/icons/en.png"


def lang_bar(filename: str) -> str:
    return (
        f"[![fr]({ICON_FR})](../fr/{filename}) "
        f"[![en]({ICON_EN})](../en/{filename})\n"
    )


def fix_paths(text: str) -> str:
    text = text.replace("](pics/", "](../pics/")
    text = text.replace("](../examples/", "](../../examples/")
    return text


def insert_lang_bar(text: str, filename: str) -> str:
    if ICON_FR in text:
        return text
    lines = text.splitlines(keepends=True)
    if not lines or not lines[0].startswith("#"):
        return lang_bar(filename) + text
    rest = lines[1:]
    if rest and rest[0].strip() == "":
        rest = rest[1:]
    return lines[0] + "\n\n" + lang_bar(filename) + "\n" + "".join(rest)


def main() -> None:
    EN.mkdir(exist_ok=True)
    FR.mkdir(exist_ok=True)

    for path in sorted(DOC.glob("*.md")):
        if path.name == "README.md":
            continue
        dest = EN / path.name
        if not dest.exists():
            path.rename(dest)

    # README.md last (hub replaces root index content later)
    readme_src = DOC / "README.md"
    if readme_src.exists() and readme_src.is_file():
        content = readme_src.read_text(encoding="utf-8")
        readme_src.unlink()
        dest = EN / "README.md"
        dest.write_text(content, encoding="utf-8")

    for path in sorted(EN.glob("*.md")):
        text = fix_paths(path.read_text(encoding="utf-8"))
        text = insert_lang_bar(text, path.name)
        path.write_text(text, encoding="utf-8")

    hub = f"""# 📚 Documentation TimedPetriNetEditor

[![fr]({ICON_FR})](fr/README.md) [![en]({ICON_EN})](en/README.md)

- [Documentation française](fr/README.md)
- [English documentation](en/README.md)
"""
    (DOC / "README.md").write_text(hub, encoding="utf-8")

    for path in sorted(EN.glob("*.md")):
        if path.name == "README.md":
            continue
        link = DOC / path.name
        if link.is_symlink() or link.exists():
            link.unlink()
        link.symlink_to(Path("en") / path.name)

    print(f"Moved {len(list(EN.glob('*.md')))} files to doc/en/")


if __name__ == "__main__":
    main()
