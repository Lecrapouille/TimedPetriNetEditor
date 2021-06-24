# TimedPetriNetEditor

A timed Petri net editor made in C++11 and displayed with the [SFML](https://www.sfml-dev.org/index-fr.php).

The following picture is an overview of the application. Since the project is currently in evolution this
screenshot may be different.

![Petri](doc/Petri.png)

## What is Petri net and what should you care of it ?

Petri net is one of several mathematical modeling languages for the description of distributed systems. It is a class of discrete event dynamic system. A Petri net is a directed bipartite graph that has two types of elements, places and transitions, depicted as white circles and rectangles, respectively. A place can contain any number of tokens, depicted as black circles. A transition is enabled if all places connected to it as inputs contain at least one token.

Why another Petri editor ? Because:
- Many Petri node editors in GitHub are no longer maintained (> 7 years) or made in foreign languages (C#, Java), foreign compiler (Visual Studio) or foreign operating systems (Windows) or their code is too complex.
- This editor will complete my [MaxPlus](https://github.com/Lecrapouille/MaxPlus.jl) toolbox for Julia (work in progress). Note: in the future I hope to be able to get this C++ application runable with Julia.

## Compilation

No makefile. Just a build.sh file:

```sh
cd PetriEditor/src
./build.sh
./TimedPetriNetEditor
```

Prerequisite are:

- g++ or clang++ compiler for C++11.
- SFML: `sudo apt-get install libsfml-dev`

## Usage

- Left mouse button pressed: add a place.
- Right mouse button pressed: add a transition.
- Middle mouse button pressed: add an arc with the selected place or transition as origin.
- Middle mouse button release: end the arc with the selected place or transition as destination.
  Note: the editor will create the good type of node (origin or destination) and the arc when
  releasing the button and if there is no present node.
- `L` key: add an arc with the selected place or transition as origin.
- `M` key: move the selected place or transition.
- `+`, `-` keys: add/remove a token on the place pointed by the mouse cursor.
- `R` key: run/end simulating simulation.
- `S`, `O` keys: save/open load a Petri net to/from petri.json file.
- `C` key: export the Petri net to C++ header file.
- `Delete` key: remove a place or transition or an arc. FIXME some possible issues
- `Z` key: clear the Petri net.

## JSON as file format for saving Petri net

Here is the content of the JSON file holding the Petri net:

```json
{
  "places": ["P0,204,135,0", "P1,576,132,2"],
  "trans": ["T0,376,132"],
  "arcs": ["P0,T0,0", "T0,P1,3"]
}
```

A Petri net is composed of three set of `Places`, `Transitions` and `Arcs`. In this example we
have two places, one tranisiton and two arcs. Places and Transitions are nodes and are defined by:
- their unique identifier (string) i.e. `P0`, `P1`, `T0`.
- their X and Y coordinate (float) in the screeen i.e. `T0` is placed at `(376,132)`.
- and for places the number of tokens they hold i.e. `P0` has 0 tokens while `P1` has two tokens.

Arcs have :
- no unique identifier.
- are directed links between two nodes of different type given they unique identifier (i.e. the first arc links the origin place `P0` to the destination transition `T0`. Therefore an arc cannot link two places or link two transitions.
- has unit of time (WIP: to be defined) i.e. the arc `T0 --> P1` has 3 units of times (float). This
time is only used for arc `Transition` to `Place` this means that for arc `Place` to `Transition` this
value is not used (while given in the json file to facilate the parsing.

Note: this project does not use third part json library for a home made token splitter (see the class Tokenizer
in the code). I used this format to be, at the origin, compatible with [pnet-simulator](https://github.com/igorakim/pnet-simulator) but soon it will be no longer compatible.

## Generate the implicit dynamic linear max+ system

Will generate a Julia script that this [package](https://github.com/Lecrapouille/MaxPlus.jl)
will understand and will compute critical cycles of the system.

## Generate C++ code file

After watching this french youtube video https://youtu.be/v5FwJvtGaEw in where Grafcet is created manually
in C for Arduino I added my own grafcet generator but since my project concerns Petri net which are more general to Grafcet, you have to complete missing methods in your own C++ to have a functional code
- `initIO()` to let you init input/output of the system (ADC, PWM, GPIO ...)
- `X0()`, `X1()` ... to let you add the code for the reaction when places are activated.
- `P0()`, `P1()` ... to let you add the code of transitivity of the associated transition.

Here a small example on how to call your grafcet. By default the C++ namespace is `generated` and
the C++ header file is `Grafcet.hpp` but this can be changed by parameters of the method
`PetriNet::exportToCpp(filepath, spancename)`.

```
#include "Grafcet.hpp"

namespace generated {

void Grafcet::initIO() {}
bool Grafcet::T0() const { return true; }
void Grafcet::X0() { std::cout << "Do actions of Place 0" << std::endl; }
void Grafcet::X1() { std::cout << "Do actions of Place 1" << std::endl; }

} // namespace generated

int main()
{
   generated::Grafcet g;
   g.debug();

   // The loop is for simulating time events of the system
   for (size_t i = 0u; i < 10u; ++i)
   {
      g.update();
      g.debug();
   }

   return 0;
}
```

Compilation: g++ -W -Wall --std=c++11 thisfile.cpp -o grafcet

## Related projects

- (en) https://github.com/Lecrapouille/MaxPlus.jl
- (en) https://github.com/igorakim/pnet-simulator
- (fr) https://sites.google.com/view/apimou/accueil
- (fr) Grafcet Arduino https://youtu.be/v5FwJvtGaEw
