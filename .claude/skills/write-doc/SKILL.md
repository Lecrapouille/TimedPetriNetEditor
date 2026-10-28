---
name: write-doc
description: Write or rewrite the documentation of TimedPetriNetEditor (README.md, doc/en/*.md, doc/fr/*.md, examples/**/README.md). Use whenever a chapter, a README, an example description or a screenshot of the documentation is created or changed, or when the user asks to improve, extend, reorganize or review the documentation.
---

# Writing the documentation of TimedPetriNetEditor

The documentation is what makes the project understood, then used. It is
**exhaustive, progressive and pedagogical**: it goes from the theory of each
family of nets to what the editor offers to solve its problems, with many
examples. A reader who never saw a Petri net shall be able to start at
chapter 1 and finish as a user of the (max,+) analysis, of GRAFCET or of the
runtime.

## 0. Grill first

Before writing or deeply rewriting a chapter, a README or an example, run the
`grilling` skill on the plan of the piece (its audience, the notions it
teaches and in which order, its examples, its depth, what it leaves to other
chapters). Skip only the questions the author already settled (in this
conversation, in `doc/README.md`, `doc/en/README.md` or in this file). Facts are found in the
code, never asked.

## 1. What each piece is for

| Piece | Role | Never |
|-------|------|-------|
| `README.md` | Say literally what the project is (an editor and simulator of Petri nets, timed event graphs with the (max,+) algebra, GRAFCET and GEMMA, extended Petri nets, a runtime calling your functions), show it, give its story, a quick start and the map of the documentation | slogans or wordplay unrelated to Petri nets ("Draw it. Run it."), marketing comparisons that cannot be checked |
| `doc/en/NN-*.md` and `doc/fr/NN-*.md` (learn) | Teach one family of nets: theory, then the editor, then examples; keep English and French in sync | lists of windows, menus or keys (those go to the editor reference) |
| `doc/en/NN-*.md` and `doc/fr/NN-*.md` (reference) | Be complete: every option, field, command, format | tutorials that repeat the learning chapters |
| `doc/README.md` | Hub with links to `doc/en/README.md` and `doc/fr/README.md` | |
| `examples/<Family>/<Name>/README.md` | What the example models, which notion it teaches, what to try, what to observe | copying the theory: link to the chapter |
| net `description` (JSON) | Shown on the canvas above the net: a 1 to 3 sentence lesson of what to do and what to look at | more than ~300 characters |

## 2. Shape of a learning chapter

1. Title with one emoji, then **What you will learn** and **Prerequisites**
   (the chapters before).
2. **Theory**, progressive: intuition with a small drawing (ASCII or a
   screenshot), then the definition, then the formal rule, then a worked
   example computed by hand, step by step, with every number shown.
3. **In the editor**: how the editor shows and solves it, with a screenshot
   of the same example and the menu path (`*Analysis > Critical circuit*`).
   Check that the editor gives the same numbers as the hand computation.
4. **Try it**: the examples of `examples/` that use the notion, with what to
   do and what to observe.
5. **Pitfalls** and **Going further** (books of the bibliography, other
   chapters).
6. **Next** link.

Theory is not optional: definitions (tuples, Pre/Post, incidence matrix,
state equation, semiring, eigenvalue ...) are given with their notation,
explained in words, and used on an example. History and names (Petri 1962,
Commoner, Karp-Miller, Cohen-Quadrat-Viot, Howard, IEC 60848, GEMMA by ADEPA,
Symmetri) are given where they help.

## 3. Bilingual layout

- Chapters live in `doc/en/` (English) and `doc/fr/` (French); shared assets in `doc/pics/` (symlinked as `doc/en/pics` and `doc/fr/pics`).
- After the `#` title of each chapter, add the language switcher:
  `[![fr](https://lecrapouille.github.io/icons/fr.png)](../fr/NN-….md) [en](../en/NN-….md)`
- When you change a chapter, update **both** languages (or the one the user asked for, and note the other).
- Links from `examples/` point to `doc/en/…` (English anchors).

## 4. Rules of writing

- English in `doc/en/`, French in `doc/fr/`: simple and precise: short sentences, common words, one idea per
  paragraph; readers are often French.
- Every output, number, equation and screenshot comes from a real run of the
  editor, of `tpne-check` or of a program of `examples/`: never invented.
  Run it, paste it.
- Every feature written exists in the code of this commit (grep it). Say what
  the editor does not do, too.
- Emoji: one per heading of level 1 and 2 at most, and in tables of features;
  none in the middle of sentences.
- Links are relative; files as `[name](path)`; menus in italics
  `*File > Export*`; keys in bold **Space**.
- A notion is defined before it is used; the glossary has every word in bold.

## 5. Examples

- One folder per example, grouped by family:
  `examples/{Petri-Nets,Extended-Petri-Nets,Timed-Petri-Nets,Timed-Event-Graphs,GRAFCET,Runtime}/<Name>/`
  with `<Name>.json` (or `.flowshop`, `.teg`), `<Name>.png`, `README.md`, and
  `<Name>.cpp` when the runtime runs it.
- Each net has a `description` (and the document one for several nets) that
  tells, on the canvas, what to do and what to look at.
- `examples/README.md` is the gallery: one line and one picture per example,
  by family, with the notions each one teaches.
- Small "textbook" examples teach one notion each (a weight, an inhibitor
  arc, a conflict ...); real-life examples combine them.

## 6. Screenshots

Made by the editor itself, never by hand, so that they can be made again:

```sh
./build/TimedPetriNetEditor -s out.png net.json            # the net only
./build/TimedPetriNetEditor -w out.png net.json            # the whole window
./build/TimedPetriNetEditor -r -d 3 -s out.png net.json    # after 3 s of simulation
./build/TimedPetriNetEditor -f "Critical circuit" -w out.png net.json   # a tab in front
```

`tools/screenshots.sh` makes all the pictures again (1400x900 window, `-g`):

- each net of `examples/` gives `<folder>/<name>.png`; a `screenshot.args`
  file next to the net gives other options (`-w -r -d 3`);
- each line `<picture.png> <options> <net>` of `tools/screenshots/NN-*.txt`
  (one list per chapter) gives a picture of `doc/pics/`, named
  `NN-<Subject>.png` after its chapter. `-s` or `-w` is written alone: the
  script adds the picture after it.

Add every new picture to a list. Look at each picture before using it: a
cut net, an empty window or a notification is a failed picture.

## 7. Before finishing

- Build and run what the chapter shows; links checked
  (`grep -o '([^)]*\.md[^)]*)'` and the files exist).
- `doc/README.md` (the index) and `Glossary.md` updated.
- The tests that load examples still pass (`make -C tests`, then
  `../build/TimedPetriNetEditor-UnitTest` from `tests/`).
