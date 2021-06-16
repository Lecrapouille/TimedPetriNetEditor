# PetriEditor

A Petri net editor made in C++11 and displayed with the [SFML](https://www.sfml-dev.org/index-fr.php).

Why another Petri editor ? Because:
- Many Petri node editors in GitHub are no longer maintained (> 7 years) or made in foreign languages (C#), foreign compiler (Visual Studio) or foreign operating systems (Windows) or are too complex.
- I will try to make one in Julia language (none exists) and I need one for my [MaxPlus](https://github.com/Lecrapouille/MaxPlus.jl) toolbox.

## Prerequisite

- g++ or clang++ compiler.
- SFML: `sudo apt-get install libsfml-dev`

## Compilation

No makefile. Just:

```
cd PetriEditor/src
./build.sh
./PetriNetEditor
```

## Usage

- Right click: add a transition
- Left click: add a place
- Left click and Left Control key : add an arc from a place or transition
- Left click and Left Shift key : add an arc from a place or transition
- Middle click: remove a place or a transition or an arc
- `+` key: add a token on the place pointed by the mouse cursor
- `-` key: remove a token on the place pointed by the mouse cursor
- `r` key: m_simulating simulation
- `e` key: end simulation
- `c` key: clear the Petri net

## Work In Progress

- Modify the speed of animated tokens depending on the arc size to make them arrive at the same time.
- Move/remove nodes not yet implemented.
- Fix arrow magnitude.
- Allow to modify the name of nodes.
- Timed Petri.
- Generate implicit dynamic linear max+ system code for https://github.com/Lecrapouille/MaxPlus.jl
