# 2. Getting started

What you will learn:

- build the editor and open an example;
- the windows of the editor;
- draw a Petri net with the mouse only, run it, save it.

---

## 1. Build

On Linux (Fedora: `sudo dnf install zeromq-devel`, Debian:
`sudo apt install libzmq3-dev`; or build without remote control with
`make TPNE_ZEROMQ=0`):

```sh
git clone https://github.com/Lecrapouille/TimedPetriNetEditor --depth=1 --recursive
cd TimedPetriNetEditor/
make download-external-libs
make compile-external-libs
make -j8
```

The editor is `build/TimedPetriNetEditor`. `sudo make install` installs it.

## 2. Open an example

```sh
./build/TimedPetriNetEditor data/examples/ProducerConsumer.json
```

![Editor](pics/Editor-ProducerConsumer.png)

The windows:

```text
 ┌ menus ─ Run Pause x1 ─ type of net ─ Fit Grid Snap ──────────────────────┐
 │ ┌ the net ───────────────────────────┐ ┌ inspector ───────────────────┐ │
 │ │ type of the net (and description)  │ │ Places · Transitions · Arcs  │ │
 │ │                                    │ │ Actions · Document           │ │
 │ │        places, transitions         │ ├──────────────────────────────┤ │
 │ │                     notifications ►│ │ Control panel · Analysis     │ │
 │ └────────────────────────────── zoom ┘ │ (GRAFCET, event graphs)      │ │
 │ ┌ Console · Timing diagram ──────────┐ │                              │ │
 │ └────────────────────────────────────┘ └──────────────────────────────┘ │
 └ status bar: what the mouse would do now ─────── counts, position, zoom ─┘
```

- **The net**: one window per net of the file. A file can hold several nets
  (a GEMMA has three, side by side).
- **The inspector**: the captions, tokens, durations ... of the active net.
- **The console**: what happened. A message appears also a few seconds in
  the corner of the nets.
- **The status bar**: always says what each mouse button would do now. Read
  it when you do not know what to do.

`F1` opens the help. [Chapter 7](07-Editor-Reference.md) lists everything.

## 3. Tutorial: your first net

`File > New > Petri net`. The rule of the mouse is simple:
**the left button draws, the right button moves**.

1. **Left click** in the void: a place `P0` appears.
2. **Right click** in the void, on its right: a transition `T0` appears.
3. **Left drag** from `P0` to `T0`: an arc. Release a drag in the void and
   the editor creates a node of the other kind there, already linked: left
   drag from `T0` to the right, release: a place `P1` and the arc `T0 → P1`.
4. Put the mouse on `P0` and press `+`: one token.
5. **Right drag** a node to move it. Right drag in the void moves the view;
   the wheel zooms.
6. **Double click** a node to rename it: write `Ready` on `P0`.

```text
   Ready          T0           P1
    (●)  ──────►  ▭  ──────►  ( )
```

Now run it:

7. Press **Space**. The canvas gets a green frame and the console says:

   ```text
   Simulation has started! | Left click on a transition to fire it | Keys + and - on a place add or remove tokens | Space stops the simulation
   ```

8. **Left click** `T0`: the token goes to `P1`. Click again: nothing, `T0` is
   no longer enabled.
9. Press **Space** again: the simulation stops and the net comes back to its
   first marking.

Save with `Ctrl+S`. The file is described in [chapter 6](06-File-Format.md).

## 4. Try the other kinds

Change the kind of the net in the toolbar:

- *Timed Petri net*: double click an arc going out of a transition and give
  it a duration. Run: the tokens travel along the arcs.
- *GRAFCET*: places become steps, and you write conditions on the
  transitions. Chapter 4 is a full tutorial.

Other examples are in `data/examples/` ([list](../data/examples/README.md)).

---

Next: [3. Timed nets and (max,+) algebra](03-Timed-Nets-and-Max-Plus.md) —
how fast does a system go, and why.
