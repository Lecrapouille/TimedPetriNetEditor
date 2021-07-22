# Timed Petri Net Editor

[This project](https://github.com/Lecrapouille/TimedPetriNetEditor) is a timed
Petri net and graph event editor made in C++11 and displayed with the
[SFML](https://www.sfml-dev.org/index-fr.php). The following picture is an
overview of the GUI. You can click on the figure to access to a youtube link
showing how to edit a basic net.

[![TimedPetri](doc/TimedPetri01.png)](https://youtu.be/sKL9lUGeBQs)

*Fig 1 - A timed Petri net (made with this editor)*

Why another Petri editor ? Because many Petri node editors in GitHub are no
longer maintained (> 7 years) or made in foreign languages (C#, Java), foreign
compiler (Visual Studio), foreign operating systems (Windows) or their code too
complex to add my own extensions. In the future, this editor will complete my
[MaxPlus](https://github.com/Lecrapouille/MaxPlus.jl) toolbox for Julia (still
in gestation) by adding a graphical interface (for the moment there is no
Petri net editors available for Julia).

Note: this editor does not manage colored Petri net or manage inhibitor arc ...
but can generates C++ code for GRAFCET (Sequential function chart) while this is
not currently its goal.

## Compilation

Prerequisite to compile this project are:
- g++ or clang++ compiler for C++11.
- SFML: `sudo apt-get install libsfml-dev`

To download the code source, compile them and launch the binary, follow these
steps in your Unix console.
```sh
git clone https://github.com/Lecrapouille/TimedPetriNetEditor --depth=1
cd TimedPetriNetEditor/src
./build.sh
./TimedPetriNetEditor
```

No makefile is used to compile this project since of the compactness of the code
source. Just a `build.sh` file to run is enough. Please, do not copy this binary
to a root directory such a `/usr/bin` since this application generates files on
the same folder.

## GUI Usage

The GUI does not offer any buttons, therefore all actions are made from the
mouse and the keyboard.
- `Left mouse button pressed`: add a new place. Note: this action is ignored if
  the mouse cursor is already inside another node (place or transition).
- `Right mouse button pressed`: add a new transition. Note: this action is ignored
  if the mouse cursor is already inside another node (place or transition).
- `Middle mouse button pressed`: start creating an arc either from the selected
  node (place or transition) as origin or, if no node was selected, the editor
  will find the correct type depending on the selected destination node.
- `Middle mouse button released`: end creating an arc either from the selected
  node (place or transition) as destination or, if no node is selected, the
  editor will find the correct type depending on the origin node.  In the case
  you have tried to link two nodes of the same type (two places or two
  transitions) then an intermediate node of opposite type will be created.
- `L` key: same action than the middle mouse button.
- `M` key: move the node (place or transition) selected by the mouse cursor.
- `Delete` key: remove a node (place or transition). Note: since all identifiers
  shall be consecutive (no gaps), the unique identifier of the last created
  node of the same type will receive the unique identifier of the deleted one.
- `Z` key: clear the whole Petri net.
- `+`, `-` keys: add/remove a token on the place selected by the mouse cursor.
- `PageUp`, `PageDown` keys: rotate CW or CCW the transition selected by the
  mouse cursor.
- `R` key: start (run) or stop the simulation. Note: if the simulation is
  stalled, then it will automatically stops and return to edition mode.
- `C` key: show the first critical circuit if and only if the Petri net is a
  graph event.
- `S` key: save the Petri net to a JSON file named `petri.json`. If one already
  exists it will be replaced.
- `O` key: load a Petri net from `petri.json` JSON file.
- `G` key: export the Petri net as Grafcet into a C++ header file named `Grafcet-gen.hpp`.
- `J` key: export the graph event into a Julia script file name `GraphEvent-gen.jl`.

## Limitations / Work in progress

- Adding an arc will generate a random duration (between 1 and 5).
- Node captions and time durations cannot yet be edited. Workaround: save the
  petri net to json file and edit with a text editor, then reload the file.
- No input node generating periodically tokens is yet made. Workaround: during
  the simulation the user can add new token to any desired places selected by
  the mouse cursor and the `+` key.
- Showing critical cycles for graph event does not support having transitions
  without predecessor (inputs). For example `T0 -> P0 -> T1`. Workaround: Either
  modify your net either by removing your inputs or make inputs cycling to
  themself with a `-inf` time (which makes duality issues).
- When exporting to Julia code to an implicit dynamic linear Max-Plus system,
  your graph event is not yet "cannonified" (meaning all Places with `N` tokens
  shall be split into `N` places holding each a single token). Therefore you
  shall not have places with more than one tokens and Places after Transition
  inputs or Places before Transitions outputs shall have no tokens.
- Not all error messages are displayed on the GUI. Read the logs on your Unix
  console.
- We cannot control on the file name for saving, loading and exporting the net.
- Cannot change the color of nodes and arcs.
- We cannot move or zoom the Petri net or select several nodes.

## What are Petri nets, timed Petri nets ?

Petri nets are one of several mathematical modeling languages for the
description of distributed systems and synchronization of their processes. It is
a class of discrete event dynamic system.

A Petri net is a directed bipartite graph that has two types of nodes: places
and transitions, respectively depicted as circles and rectangles. A place can
contain zero or any positive number of tokens, usually depicted either as black
circles or as numbers. Arcs, depicted as arrows. An arc allows to directed two
nodes of different type: either from a place to a transition or from a
transition to a place. In timed Petri nets, place -> transition arcs are
valuated with a strictly positive duration value which simulates the time
needed by the flace to perform the associated action. A transition is activated
if all places connected to it as inputs contain at least one token. When a
transition is activated a token is burnt from each of these places and added to
each places connected as outputs to this transition.

Usually, places depicts the states of the systems, tokens depict the resources
of the system and transitions depicts the synchronization of resources or states
(rendez-vous). Petri is another way to modelize state machines.

In the above figure 1, there are 5 transitions (`T0, T1, T2, T3, T4`) and 4
places (`P0, P1, P2, P3`). Places `P0` and `P1` have 1 token each, the place
`P1` has 2 tokens and the place `P3` has no token. The arc `T0 -> P1` has 3
units of times to simulate the fact that `P1` will need this duration to perform
its action. In the previous figure, transitions `T0, T1, T2, T4` are activated
but the transition `T3` is disabled.

Inputs and outputs of Petri net are transitions.

## What decision is made when several transitions can burn tokens from the same place ?

Question: In the above figure 1, the place `P1` has two tokens and has two
leaving arcs `P1 -> T1` and `P1 -> T2`. How transitions `T1` and `T2` will burn
tokens in `P1` and therefore to which transition tokens will transit ?

Answer: This kind of net is probably badly designed because it shows
concurrencies between resources and this kind of design should be avoided for
architecturing real systems. The execution of this kind of Petri nets is
nondeterministic: when multiple transitions are enabled at the same time, they
will fire in any order. Therefore you should adapt your Petri net to make
deterministic decision possible i.e. event graph (but maybe you want to simulate
system with concurrencies).

Nondeterministic execution policy is made at the discretion of the developer of
the editor and you will have different behavior depending on how the editor has
been developed. Currently in our case, the order when iterating on transitions
and arcs only depends of their order of creation. When several places are fired,
the maximum possible of tokens will be burn within a single step of the
animation cycle but, internally, we iterate over tokens one by one to help
dispatching them over the maximum number of arcs.  Therefore, in this particular
example, since `T1` has been created before `T2` (unique identifiers are
incremented by one from zero and there is no gap), the 1st token will go to `T1`
and the second will go to `T2`. If `P1` had a single token, the `T1` will always
be chosen. In the future we may ramdomize burns.

## What are graph events ?

An event graph is a special subclass of Petri net in which all places have a
single input arc and a single output arc. This property allows to forbide, for
example, the or-divergent branching (choice between several transitions) is
never occuring. In the above figure 1, the net is not an event graph since `P0`
has 3 incoming arcs and `P2` has two output arcs. The following figure 2 is an
event graph.

![EventGraph](doc/EventGraph01.png)

*Fig 2 - Event Graph (made with this editor)*

Thanks to this property, another way to represent event graphs in a more compact
form, is to merge places with their unique incoming and unique outcoming
arcs. We obtain the following figure 3, which is a more compact form than the
Petri net in figure 2. For example, the arc `P1/5/0` means the place `P1` with
the duration 5 and no tokens. Note that this editor does not yet manipulate this
compact form.

![Graph](doc/Graph01.png)

*Fig 3 - Compact form of the event graph despicted in figure 2*

Since, graphs can be represented by adjency matrices, and since, timed Petri
arcs hold two informations (duration and tokens), event graphs can be
represented by two MaxPlus matrices:  one matrix for durations `N` and the
second matrix for tokens `T`:

```
N = [ε   ε   1   ε
     0   ε   ε   ε
     ε   0   ε   0
     0   ε   ε   ε]

T = [ε   ε   5   ε
     5   ε   ε   ε
     ε   3   ε   1
     1   ε   ε   ε]
```

Let suppose that matrix indices start from `0`, then `N[0,2]` holds the value 1
token and `T[0,2]` holds the duration 5. `[0,2]` means the arc `T2 -> T0` in the
compact form (or arcs `T2 -> P0` and `P0 -> T0` in the classic Petri form).
Note that origin and destination is inversed, this is because the matrix
convention is generally the follow: `M . x` with `x` a column vector. This
editor can generate some Julia script.

Event graphs have an interesting since they can be modeled by implicit dynamic
linear MaxPlus systems which have the following form:

```
    X(n) = D ⨂ X(n) ⨁ A ⨂ X(n-1) ⨁ B ⨂ U(n)
    Y(n) = C ⨂ X(n)
```

In where the operator ⨁ is the usual multiplication in classic algebra, the
operator ⨂ is the usual maximum function in the classic algebra, `A, B, C, D`
are MaxPlus matrices, `X` is a MaxPlus column vector representing the system
states, `U` the column vector of system inputs and `Y` the system outputs. This
kind of systems are interesting for real-time systems because they can show to
the critical circuit of the system (in duration).

For more information, see my
[MaxPlus](https://github.com/Lecrapouille/MaxPlus.jl) which will, in the future,
contains tutorials explaining more deeply this part. This editor is the
graphical part of this toolbox. For the moment this editor can generate the
Julia script and shows the crictical circuit like shown in the next figure 4
where the circuit `T0, T1, T2` will consums 13 unit of time.

![Circuit](doc/Circuit01.png)

*Fig 4 - The critical circuit of the system colorized in orange*

## Simulation, inputs and ouptuts of an event graph ?

When running the simulation, tokens will transit along arcs. From places to
transitions they are directly teleported but from places to transitions, they
will move during the number of seconds indicated by the arc. Note: duration of
animations are not normalized. A fading effect will show you which arcs and
nodes are triggered.

Inputs generating periodically tokens is currently not implememted. For the
moment, during the simulation, you can place your mouse cursor to any place and
with `+` key you can add a token this will mimic input generating tokens.

For outputs, simply any transitions without successor will burn definitively
tokens.

## Description of the file format used for saving Petri net

JSON file format has been chosen for saving Petri net. This was initially made
to be compatible with [pnet-simulator](https://github.com/igorakim/pnet-simulator)
but it is no longer compatible. Here, an example of its content:

```json
{
  "places": ["P0,204,135,0", "P1,576,132,2"],
  "transitions": ["T0,376,132,0"],
  "arcs": ["P0,T0,0", "T0,P1,3"]
}
```

A Petri net is composed of three arrays (the `[ ]`): `Places`, `Transitions` and
`Arcs`. In this example, is stored in the json file a Petri net made of two
places, one tranisiton and two arcs. Places are defined as follow `"identifier,
X-coord, Y-coord, number of tokens"`. Transitions are defined as follow
`"identifier, X-coord, Y-coord, angle"` and Arcs are defined as follow `"identifier
origin node, identifier destination node"`.

Places and Transitions:
- their unique identifier (string) i.e. `P0`, `P1`, `T0`. First character shall
  be `P` (for Places) or `T` (Transitions).
- their X and Y coordinate (float) in the screeen i.e. `T0` is placed at
  `(376,132)`.
- for places only: the number of tokens they hold (zero or postive number)
  i.e. `P0` has 0 tokens while `P1` has two tokens.
- for transition only: the angle (in degree) of rotation when displayed.

Arcs:
- have no unique identifier.
- are directed links between two nodes of different type given they unique
  identifier (i.e. the first arc links the origin place `P0` to the destination
  transition `T0`. Therefore an arc cannot link two places or link two
  transitions.
- has unit of time (positive value) i.e. the arc `T0 --> P1` has 3 units of
  times (float). This time is only used for arc `Transition` to `Place` this
  means that for arc `Place` to `Transition` this value is not used but given
  in the json file to make easy its parsing.

Note: this project does not use third part json library for a home made token
splitter (see the `class Tokenizer` in the code).

## Note concerning unique identifiers

In this editor, unique identifiers are unsigned integers `0 .. N`. All unique
identifiers for a given type are consecutive. There shall have no gap between
id. This is important when generating matrices: indices will directly matches
unique identifiers. Arcs have no identifier because their relation of directing
nodes is unique since we do not manage multi arcs.

## Generate the implicit dynamic linear MaxPlus system

See section on event graph.

## See the critical circuit

See section on event graph.

## Generate C++ code file (Grafcet)

After watching this nice french youtube video https://youtu.be/v5FwJvtGaEw, in
where Grafcet is created manually in C for Arduino, I added my own code
generator for Grafcet, but since my project mainly concerns Petri net which are
more general to Grafcet, you have to complete manually missing methods in your
own C++ to make it compile:
- `initIO()` to let you init input/output of the system (ADC, PWM, GPIO ...)
- `X0()`, `X1()` ... to let you add the code for actions when places are
  activated (usually to update actuators).
- `T0()`, `T1()` ... to let you add the code of transitivity of the associated
  transition (usually condition depending on system sensors). Return `true`
  when the transition is enabled.

Here a small example on how to call your generated Grafcet. By default, the C++
namespace is `generated` and the C++ header file is `Grafcet-gen.hpp` but this
can be changed by parameters of the method `PetriNet::exportToCpp(filepath,
namespace)`.

```c++
// main.cpp
#include "Grafcet-gen.hpp"

namespace generated {

void Grafcet::initIO() { std::cout << "Init system, inputs, outputs" << std::endl; }
bool Grafcet::T0() const { return true; }
void Grafcet::X0() { std::cout << "Do actions of Place 0" << std::endl; }
...
// Idem for all transitions of the Grafcet

} // namespace generated

int main()
{
   generated::Grafcet g;
   g.debug();

   // The loop is for simulating time events of the system
   while (true)
   {
      g.update();
      g.debug();
	  sleep(x_ms);
   }

   return 0;
}
```

To be compiled with: `g++ -W -Wall -Wextra --std=c++11 main.cpp -o grafcet`

## Related projects

- (en, fr) https://github.com/Lecrapouille/MaxPlus.jl My MaxPlus toolbox for
  Julia, a portage of the http://www.scicoslab.org/ toolbox (seens Scilab is no
  longer maintained).
- (en) http://www.cmap.polytechnique.fr/~gaubert/HOWARD2.html the algorithm used
  for computing the MaxPlus egeinvalue used in ScicosLab MaxPlus toolbox and
  that I reused in this project.
- (en) http://www-sop.inria.fr/mistral/soft/ers.html An abandonned project of
  Petri net and discrete time event systems.
- (en) https://github.com/igorakim/pnet-simulator A online petri net editor that
  inspired my GUI.
- (fr) https://sites.google.com/view/apimou/accueil A Grafcet editor and code
  generation for Arduino.
- (fr) https://youtu.be/v5FwJvtGaEw A lesson about Grafcet and code for Arduino.
