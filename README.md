# TimedPetriNetEditor

A timed Petri net editor made in C++11 and displayed with the [SFML](https://www.sfml-dev.org/index-fr.php).

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

- Left mouse button pressed: add a place.
- Right mouse button pressed: add a transition.
- Middle mouse button pressed: add an arc with the selected place or transition as origin.
- Middle mouse button release: end the arc with the selected place or transition as destination.
- `L` key: add an arc with the selected place or transition as origin.
- `M` key: move the selected place or transition.
- `+` key: add a token on the place pointed by the mouse cursor.
- `-` key: remove a token on the place pointed by the mouse cursor.
- `R` key: m_simulating simulation.
- `E` key: end simulation.
- `S` key: save the Petri net as petri.json file.
- `O` key: load the Petri net from petri.json file. TODO
- `Delete` key: remove a place or transition or an arc. TODO
- `C` key: clear the Petri net.
