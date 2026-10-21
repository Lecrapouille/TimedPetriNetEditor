# Generate C++ code file (GRAFCET aka sequential function chart)

GRAFCET (GRAphe Fonctionnel de Commande Etapes-Transitions in French),
aka SFC (Sequential Function Chart) in English.

After watching this nice French YouTube video https://youtu.be/v5FwJvtGaEw, in
which GRAFCET is created manually in C for Arduino, I extended my editor for
generating GRAFCET in a single C++ header file. Since my project mainly concerns
timed Petri net, the editor is not intended to follow all the GRAFCET norms
(GitHub pull requests are welcome).

Inside the editor, set your net type to GRAFCET. Transitions captions are the
receptivities, written with the IEC 60848 notation (old files used reverse
polish notation; they are converted when loaded). *Help > GRAFCET
receptivities* has the same table, *Help > GRAFCET actions* the timing diagram
of each qualifier, and the control panel of the editor shows the inputs as
push buttons, selectors, sensors ... (see doc/save.md, doc/gui.md):

| What                       | Syntax                                     |
|----------------------------|--------------------------------------------|
| And, or, not               | `a . b`, `a + b`, `/a` (also `*`, `!`, `and`, `or`, `not`) |
| Constants                  | `1`, `0`, `true`, `false`                  |
| Step 42 is active          | `X42`                                      |
| Step of another graph      | `Safety:X3`                                |
| Rising / falling edge      | `↑a` (`^a`, `rising(a)`), `↓a` (`falling(a)`) |
| Time                       | `3s/X2`, `2s/a/5s` (units `ms`, `s`, `min`, `h`) |
| Integer input              | `[count >= 3]` (`=`, `!=`, `<`, `<=`, `>`, `>=`) |
| Inputs                     | `Dcy`, `sensor_2`, `holding(cube)` ...     |

Precedence: not, then and, then or. An empty receptivity is always true.

How the GRAFCET runs (class `GrafcetEngine`, usable without the editor):
- all the transitions that can fire fire together; a step activated and
  deactivated at the same time stays active;
- after a change of inputs, graphs evolve until they are stable before actions
  are computed (an edge is seen during one instant only);
- actions: `N` continuous, `S`/`R` stored by name, `D` delayed, `L` limited,
  `SD`, `DS`, `SL` stored and timed, `P1`/`P0` pulse on activation /
  deactivation. An action can have a condition (same syntax);
- forcing orders (`Production{init}`, `{*}`, `{}`, `{2,8}`, the colon of
  `Production:{init}` is optional) are continuous actions with priority: the
  forced graph takes the situation and does not evolve while forced.

The example `data/examples/Gemma.json` is the GRAFCET of a dryer split by the
GEMMA into three graphs: Securite (emergency stop, rearm), Controle (modes:
validation after a failure, heating, automatic or manual production) and
Production (left motor 30 s, drying until Acy, right motor 30 s). Its control
panel has the emergency stop, the push buttons, the Auto / Manu selector and
a temperature sensor heated by the oven.

The C++ export below does not support edges and time operators yet.


Example `a X0 ! .` means `and(a, not(X0))`.

Note: Tempo, raising or falling edge and states of steps are not yet managed. Actions code are not yet managed by the editor.

Once the C++ code generated, you will have to write manually the missing methods in your own C++ file:
- `P0()`, `P1()` ... to let you add the code for actions when places are
  activated (usually to update actuators). You have to implement one method
  for each place.
- if your net is not a GRAFCET. `T0()`, `T1()` ... to let you add the code of the
  transitivity (boolean logic)
  of the associated transition (usually, condition depending on system
  sensors). Return `true` when the transition is enabled, else return
  `false`. With GRAFCET net the code of receptivities are generated.

![TrafficLight](doc/pics/TrafficLight.png)

*Fig 6 - Traffic Light (made with this editor).*

Here is a small example of how to call your generated GRAFCET as `Grafcet-gen.hpp`
with a traffic light depicted by the following [net](examples/TrafficLight.json) :
- `Red1`, `Green1` and `Orange1` are the three colors of the first light.
- `Red2`, `Green2` and `Orange1` are the three colors of the second light.
- `P6`, `T0`, and `T3` allows turning green one of the lights.

By default, the C++ namespace is `generated` but this can be changed by
parameters of the method `PetriNet::exportToCpp(filepath, namespace)`. Let us
program the main.cpp file:

```c++
// main.cpp
#include "Grafcet-gen.hpp"
#include <chrono>
#include <thread>

using namespace std::chrono_literals;
namespace generated {

bool a = true;

bool Grafcet::T0() const { return a; }
bool Grafcet::T1() const { return true; }
bool Grafcet::T2() const { return true; }
bool Grafcet::T3() const { return !a; }
bool Grafcet::T4() const { return true; }
bool Grafcet::T5() const { return true; }
void Grafcet::P0() { std::cout << "Red 1" << std::endl; }
void Grafcet::P1() { std::cout << "Green 1" << std::endl; }
void Grafcet::P2() { std::cout << "Orange 1" << std::endl; }
void Grafcet::P3() { std::cout << "Red 2" << std::endl; }
void Grafcet::P4() { std::cout << "Green 2" << std::endl; }
void Grafcet::P5() { std::cout << "Orange 2" << std::endl; }
void Grafcet::P6() { a = a ^ true; }

} // namespace generated

int main()
{
   generated::Grafcet g;
   g.debug();

   // Add here init of your sensors

   // The loop is for simulating the runtime loop of your task
   while (true)
   {
      std::cout << "=========\n";

      // Add here the reading of your sensors. For example:
      // a = digitalRead(3);

      // Do a single GRAFCET iteration. This will call internally
      // T0(), T1(), .. P0() ...
      g.update();

      // Uncomment for displaying states of the GRAFCET
      // g.debug();

      //Let's suppose here the time step is 1 Hz.
      std::this_thread::sleep_for(1000ms);
   }

   return 0;
}
```

To be compiled with: `g++ --std=c++14 -W -Wall -Wextra main.cpp -o TrafficLight`
To run: `./TrafficLight` You will see output such as:
```
=========
Red 1
Red 2
=========
Red 1
Green 2
=========
Red 1
Orange 2
=========
Red 1
Red 2
=========
Green 1
Red 2
=========
Orange 1
Red 2
=========
^C
```

The variable `a` is used to commute when the light turns green. Ideally, remove
the code of `a = a ^ true;` of `Grafcet::P6()` and in the `while` loop, before
`g.update();`, implement a real sensor: for example with an Arduino `a =
digitalRead(3);` and for actions such `P0()` ... update your actuators (i.e
`digitalWrite(4, HIGH);`).

Do not forget that OR-divergence `P6 -> T0` and `P6 -> T3` shall be mutually
exclusive `T0() const { return a; }` and `T3() const { return !a; }` else if
both return `true` you will see that both lights are simultaneously green and
you will not like this kind of system in real life :)

```
=========
Red 1
Red 2
=========
Green 1
Green 2
=========
Orange 1
Orange 2
```
