# 5. Exports and code generation

What you will find here:

- the file formats the editor reads and writes (*File > Import*,
  *File > Export*);
- how to turn a GRAFCET into C++ code for a small computer (Arduino ...).

---

## 1. Formats

| Format | Extension | Import | Export | For |
|--------|-----------|:------:|:------:|-----|
| JSON | `.json` | yes | yes | the format of the editor ([chapter 6](06-File-Format.md)) |
| Flowshop | `.flowshop` | yes | | a factory as a table of processing times ([chapter 3](03-Timed-Nets-and-Max-Plus.md#3-tutorial-how-fast-is-my-factory)) |
| Timed event graph | `.teg` | yes | yes | a compact text format (§6) |
| Petri Net Markup Language | `.pnml` | yes | yes | the standard exchange format of Petri net tools |
| Arduino GRAFCET | `.hpp` `.h` | | yes | C++ code of a GRAFCET (§7) |
| Julia | `.jl` | | yes | the matrices of a timed event graph for [MaxPlus.jl](https://github.com/Lecrapouille/MaxPlus.jl) |
| Petri LaTeX | `.tex` | | yes | a drawing for a document (§2) |
| Grafcet LaTeX | `.tex` | | yes | a GRAFCET drawing for a document (§2) |
| Graphviz | `.gv` `.dot` | | yes | a drawing by Graphviz (§3) |
| Draw.io | `.drawio.xml` | | yes | an editable drawing in [Draw.io](https://app.diagrams.net) |
| PN-Editor | `.pns` ... | | yes | [pn-editor](https://gitlab.com/porky11/pn-editor) (§4) |
| Symfony | `.yaml` | | yes | a [Symfony workflow](https://symfony.com/doc/current/components/workflow.html) |

## 2. LaTeX

Two exports write a `.tex` file to compile with `pdflatex file.tex`:

- **Petri LaTeX** uses the `petri` library of TikZ: places, transitions,
  arcs, tokens.
- **Grafcet LaTeX** (GRAFCET only) draws the steps as squares (double for an
  initial step) and the transitions as bars with their receptivity.

![LaTeX](pics/Latex.png)

*A canonical event graph exported to LaTeX.* On Linux, install LaTeX with
`sudo apt-get install texlive-base`.

## 3. Graphviz

Writes a `.dot` file; view it with `xdot file.dot`
(`sudo apt-get install xdot`).

![Graphviz](pics/Graphviz.png)

## 4. pn-editor

Writes the files of [pn-editor](https://gitlab.com/porky11/pn-editor) (to
install by hand).

![PnEditor](pics/pn-editor.png)

## 5. Julia

Writes the (max,+) matrices of a timed event graph as a Julia script for
MaxPlus.jl. The tables of *Analysis* also have a *Copy as Julia* button.
[Chapter 8](08-Julia.md) drives the editor from Julia.

## 6. Timed event graph (`.teg`)

A compact text format for timed event graphs, from the
[Howard algorithm](http://www.cmap.polytechnique.fr/~gaubert/HOWARD2.html)
project:

```text
TimedEventGraph <number of transitions> <number of arcs>
<from transition> <to transition> <duration> <tokens>
...
```

One line per arc between two transitions (the hidden place holds the
tokens). Durations are numbers with a decimal part, the other values whole
numbers.

## 7. C++ code of a GRAFCET

*File > Export > Arduino GRAFCET* writes one C++ header with a class
`grafcet::Grafcet`. It follows the rules of chapter 4: each call of
`update(dt)` reads the sensors, computes which transitions fire, updates the
steps, computes and writes the outputs.

For `data/examples/GRAFCET.json` (receptivities `A . /B`, `A . B`), the
generated code contains:

```c++
// Grafcet-gen.hpp (extract)
namespace grafcet {
class Grafcet
{
public:
    void update(float dt)
    {
        ...
        // 2. Compute transition firing conditions
        T[0] = X[0] && R0();  // A . /B
        T[1] = X[0] && R1();  // A . B
        ...
        // 3. Update step states
        X[0] = T[2] || T[3] || (X[0] && !T[0] && !T[1]) || m_init;  // P0
        ...
    }
    bool isStep0Active() const { return X[0]; }  // P0
    void forceInit();                           // forcing orders
    ...
private:
    // Sensor reading methods (implement these for your hardware)
    bool readSensorA() { return A; }  // TODO: read from GPIO
    bool readSensorB() { return B; }  // TODO: read from GPIO
    // Receptivity (transition guard) methods
    bool R0() const { return !!((A && !(B))); }  // A . /B
    ...
};
} // namespace grafcet
```

What you write:

- the bodies of `readSensor...()` (for example `return digitalRead(3);` on
  an Arduino) and of the output writing methods (`digitalWrite(4, HIGH);`);
- the loop that calls `update(dt)` at a fixed period.

The file also contains a test program. Try it on your computer:

```sh
g++ -std=c++17 -x c++ -DGRAFCET_STANDALONE_TEST -DGRAFCET_DEBUG Grafcet-gen.hpp -o grafcet_test
./grafcet_test
```

```text
[GRAFCET] Reset to initial state
GRAFCET 'GRAFCET' simulation started.
Press Ctrl+C to stop.

[GRAFCET] Active steps: X0
[GRAFCET] Active steps: X0
```

Limits of the generated code (future work): the edges (`↑a`), the time
operators (`3s/X2`) and the conditions of the actions are not generated yet.
Use the GRAFCET engine of the library ([chapter 10](10-Library.md)) when you
need them. The idea of the generator comes from this video:
[Programmation d'un Grafcet en C pour Arduino](https://youtu.be/v5FwJvtGaEw).

---

Next: [6. File format](06-File-Format.md).
