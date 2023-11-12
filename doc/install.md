# How to compile and install the project?

This project is developed in C++14 and compiled with a Makefile (it does not use CMake)
for Linux, Mac OS X and Web(if compiled with Emscripten).

Here are prerequisites to compile this project are:
- The GUI is made with [Dear ImGui](https://github.com/ocornut/imgui),
  [raylib](https://github.com/raysan5/raylib) used as Dear ImGui backend, and other
  Dear ImGui widgets (ImPlot).
- [MQTT mosquitto](https://github.com/eclipse/mosquitto): `sudo apt install
  libmosquitto-dev mosquitto mosquitto-clients`.
- Optionally, libdwarf: `sudo apt-get install libdw-dev` (needed when compiling
  this project in debug mode because it will use
  [backward-cpp](https://github.com/bombela/backward-cpp) for showing the stack
  trace in case of segfault) else in the Makefile replace `BUILD_TYPE = debug`
  by `BUILD_TYPE = release`.

Optionally, a [Julia 1.x](https://julialang.org/) code binding the C++ API can be
optionally used for working in synergy with my [(max,+) toolbox](https://github.com/Lecrapouille/MaxPlus.jl).
You can install Julia from https://julialang.org/downloads but this does not impact the standalone graphical editor.

## Clone the project

```sh
git clone https://github.com/Lecrapouille/TimedPetriNetEditor --depth=1 --recursive
```

The recursive argument is important: it allows to clone my Makefile helper.

## Compile the project for Linux and Mac OS X

```sh
cd TimedPetriNetEditor/
make clean
make download-external-libs
make compile-external-libs
make -j8
```

Compiled files are in `build` folder.

Where:
- `make download-external-libs` is needed to be done once for downloading
  third-parts libraries (it replaces older).
- `make compile-external-libs` is needed to be done once for compiling third-parts libraries.
- `-j8` is to be adapted to the number of CPU cores of your computer (8 cores in my case).

## Compile the project for Emscripten

Install the Emscripten environament, and type:

```sh
cd TimedPetriNetEditor/
make download-external-libs
emmake make compile-external-libs
emmake make -j8
```

## Compile with alternative compiler

You can change your compiler with `CXX=clang++-13` for example. You can add `V=1`
for verbosity.

```sh
cd TimedPetriNetEditor/
make clean
make download-external-libs
make CXX=clang++-13 compile-external-libs
V=1 make CXX=clang++-13 -j8
```

## Installation, execution

Once compiled, you can run the TimedPetriNetEditor application, you can:
```sh
./build/TimedPetriNetEditor
```

For Mac Os X a bundle application is present.

For Web page: you can do either:
- `make run` (**note:** small but currently it rebuilds the whole project each time).
- `cd build; emrun TimedPetriNetEditor.html`

For Julia developers, you can use this editor in synergy with my Julia
[(max,+)](https://github.com/Lecrapouille/MaxPlus.jl) toolbox. The easier way to
achieve it is to you install TimedPetriNetEditor on your operating system (this
will install a shared library needed that can be found by Julia **note:** not yet
made with Mac OS X):

```sh
sudo make install
```

For example on Linux:
```
*** Installing: doc => /usr/share/TimedPetriNetEditor/0.3.0/doc
*** Installing: examples => /usr/share/TimedPetriNetEditor/0.3.0/examples
*** Installing: data => /usr/share/TimedPetriNetEditor/0.3.0/data
*** Installing: libs => /usr/lib
*** Installing: pkg-config => /usr/lib/pkgconfig
*** Installing: headers => /usr/include/TimedPetriNetEditor-0.3.0
*** Installing: src => /usr/include/TimedPetriNetEditor-0.3.0
```

Once installed, you can call directly the Petri net editor:
```sh
TimedPetriNetEditor
```

## Command line

`TimedPetriNetEditor` takes an optional Petri file in the command line
for example one inside `data/examples`:

```sh
TimedPetriNetEditor petri.json
```

## Debug with Visual Studio Code

Type F5 key to launch the application inside a debugger.
You can modify the `.vscode/launch.json` for example to modify
with file to start with a differnet Petri file.

## Julia integration

Once installed in your operating system, you can directly from
the [Julia](https://github.com/JuliaLang/julia) REPL (this part
is described in detail in a dedicated [document](julia.md)):

```sh
julia> include("src/julia/TimedPetriNetEditor.jl")
counter (generic function with 1 method)

julia> pn = petri_net()
PetriNet(0)

julia> editor!(pn)
```

If you do not desire to install TimedPetriNetEditor on your operating system,
you will have to adapt the `DEFINES` in Makefile to indicate the path of the
`data/` folder (to find the fonts). You will also have to manually modify this
Julia file to indicate the correct path of the shared library
`libtimedpetrineteditor.so`.
