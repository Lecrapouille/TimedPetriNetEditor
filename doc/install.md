# How to compile and install the project?

This project is developed in C++11. The GUI is made with the
[SFML](https://www.sfml-dev.org/index-fr.php) and [Dear ImGui](https://github.com/ocornut/imgui).
The project is compiled with a Makefile (no CMake). Optionally, a [Julia 1.x](https://julialang.org/) code
binding the C++ API can be optionally used for working in synergy with my
[(max,+) toolbox](https://github.com/Lecrapouille/MaxPlus.jl) (ideally Julia upper to 1.8).

Prerequisites to compile this project are:
- g++ or clang++ compiler for C++11.
- [SFML](https://www.sfml-dev.org/): `sudo apt-get install libsfml-dev`
- [MQTT mosquitto](https://github.com/eclipse/mosquitto): `sudo apt install
  libmosquitto-dev mosquitto mosquitto-clients`.
- Optionally, libdwarf: `sudo apt-get install libdw-dev` (needed when compiling
  this project in debug mode because it will use
  [backward-cpp](https://github.com/bombela/backward-cpp) for showing the stack
  trace in case of segfault) else in the Makefile replace `BUILD_TYPE = debug`
  by `BUILD_TYPE = release`.
- Optionally, you can install Julia from https://julialang.org/downloads but
  this does not impact the standalone graphical editor.

To download the code source of the
[TimedPetriNetEditor](https://github.com/Lecrapouille/TimedPetriNetEditor)
project, type the following command on a Linux console:

```sh
git clone https://github.com/Lecrapouille/TimedPetriNetEditor --depth=1 --recursive
```

The recursive argument is important: it allows to clone my Makefile helper.

To compile the project:
```sh
cd TimedPetriNetEditor/
make download-external-libs
make -j8
```

Where `make download-external-libs` is needed once to download third-parts
libraries and where `-j8` is to be adapted to the number of CPU cores of your
computer (8 cores in my case).

To run the TimedPetriNetEditor application, you can:
```sh
./build/TimedPetriNetEditor
```

For Julia developers, you can use this editor in synergy with my Julia
[(max,+)](https://github.com/Lecrapouille/MaxPlus.jl) toolbox. The easier way to
achieve it is to you install TimedPetriNetEditor on your operating system (this
will install a shared library needed that can be found by Julia):

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

Or directly from the [Julia](https://github.com/JuliaLang/julia) REPL (this part
is described in detail in the dedicated section of this document):

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
