# 🧰 16. Utiliser la bibliothèque en C++


[![fr](https://lecrapouille.github.io/icons/fr.png)](../fr/16-Library.md) [![en](https://lecrapouille.github.io/icons/en.png)](../en/16-Library.md)

Ce que vous trouverez ici : comment un programme utilise les réseaux sans l'éditeur : charger un fichier, exécuter un GRAFCET, calculer les dates d'un graphe d'événements temporisé, le simuler, dessiner un réseau dans votre propre fenêtre Dear ImGui, exécuter vos fonctions, vérifier les propriétés d'un réseau.

L'éditeur est constitué de ces bibliothèques (C++20) :

| Bibliothèque | Sources | Requiert | Rôle |
|---------|---------|-------|------|
| `libTimedPetriNet` | `src/PetriNet` | | réseaux, règle de tir, moteur, simulation, moteur GRAFCET ; pas de format de fichier, pas de graphisme |
| `libTimedPetriIO` | `src/IO` | Net | formats de fichiers : `loadFromFile()`, `saveToFile()`, importateurs et exportateurs ([chapitre 15](15-File-Formats.md)) |
| `libTimedPetriMaxPlus` | `src/MaxPlus` | Net, IO | algèbre (max,+), Howard, circuit critique, systèmes linéaires, équations, flowshops, export Julia |
| `libTimedPetriAnalysis` | `src/Analysis` | Net | graphes d'accessibilité et de couverture, deadlocks, bornes, vivacité, invariants (§8) |
| `libTimedPetriRuntime` | `src/Runtime` | Net | runtime : une transition est une fonction de votre programme (§6) |
| `libTimedPetriGUI` | `src/Editor` | all | l'éditeur |

Un programme qui exécute un réseau sur une carte n'a besoin que de `libTimedPetriNet` et `libTimedPetriRuntime` : il construit son réseau dans le code (§6) ou l'éditeur écrit ce code (*File > Export > C++ functions for the runtime*).

[RobotIK](https://github.com/Lecrapouille/Robotik) pilote ses cellules robot avec la première.

---

## 1. Compiler contre la bibliothèque

Après `make`, les bibliothèques sont dans `build/linux-gcc-debug/lib/`, avec leurs fichiers pkg-config, qui donnent les dossiers d'inclusion des sources et des dépendances. Un programme `main.cpp` :

```sh
export PKG_CONFIG_PATH=$PWD/build/linux-gcc-debug/lib/pkgconfig
g++ -std=c++20 main.cpp $(pkg-config --cflags --libs TimedPetriMaxPlus TimedPetriIO TimedPetriNet) \
    -Wl,-rpath,$PWD/build/linux-gcc-debug/lib -o main
```

`-lTimedPetriMaxPlus` seulement pour §3 (temps de cycle, dates des équations (max,+)). Appelez `registerMaxPlusFormats()` une fois pour charger et enregistrer des fichiers `.flowshop` et enregistrer des fichiers `.jl` avec `loadFromFile()` et `saveToFile()`.

Un projet construit avec [MyMakefile](https://github.com/Lecrapouille/MyMakefile) 3 peut compiler les bibliothèques avec ses propres cibles à la place. `TimedPetriNet.mk` les déclare : `TimedPetriNet` (les réseaux), `TimedPetriIO` (les formats), `TimedPetriMaxPlus`, `TimedPetriRuntime`, `TimedPetriAnalysis` et `TimedPetriViewer` (§5, qui demande aussi une cible `imgui`) :

```makefile
TARGETS += TimedPetriNetEditor monappli
TimedPetriNetEditor.GIT := https://github.com/Lecrapouille/TimedPetriNetEditor.git
TimedPetriNetEditor.VERSION := v0.8.0
TimedPetriNetEditor.MAKEFILES := TimedPetriNet.mk external/external.mk
monappli.DEPENDS := TimedPetriRuntime TimedPetriIO
```

## 2. Exécuter un GRAFCET

Le séchoir du [chapitre 10](10-GEMMA.md#4-tutorial-the-gemma-of-a-dryer), sans l'éditeur. Le programme écrit les entrées, appelle `step(dt)` une fois par cycle, et lit les étapes et les sorties :

```c++
#include "PetriNet/GrafcetEngine.hpp"
#include "IO/Imports/Imports.hpp"
#include <iostream>
using namespace tpne;

int main()
{
    std::vector<Net> nets;
    std::cout << importAllNetsFromJSON(nets, "data/examples/GEMMA-Dryer.json"); // error, or nothing
    GrafcetEngine e;
    e.graphs({ &nets[0], &nets[1], &nets[2] }); // they run together
    e.start();                                  // false on errors: see e.errors()
    auto show = [&](char const* what) {
        std::cout << what << "  t=" << e.time() << "  steps:";
        for (Net* n : e.graphs())
            for (auto& p : n->places()) if (p.tokens) std::cout << " " << n->name << ":X" << p.id;
        std::cout << "  outputs:";
        for (auto& o : e.outputs()) if (o.second) std::cout << " " << o.first;
        std::cout << "\n";
    };
    auto press = [&](char const* in) { e.setInput(in, 1); e.step(0.1f); e.setInput(in, 0); e.step(0.1f); };
    e.step(0.1f); show("start");
    press("Rearm"); show("Rearm");
    press("Ack"); show("Ack");
    for (int i = 0; i < 30; ++i) e.step(0.1f); show("3 s later");
    press("Ack"); show("Ack (Auto)");
    e.setInput("Temperature", 85); e.step(0.1f); show("Temperature = 85");
    press("Start"); show("Start");
    press("Stop"); show("Stop");
    for (int i = 0; i < 30; ++i) e.step(0.1f); show("3 s later");
}
```

```text
start  t=0.1  steps: Production:X0 Control:X0 Safety:X0  outputs:
Rearm  t=0.3  steps: Production:X0 Control:X0 Safety:X1  outputs:
Ack  t=0.5  steps: Production:X0 Control:X1 Safety:X1  outputs: Left Motor
3 s later  t=3.5  steps: Production:X0 Control:X2 Safety:X1  outputs:
Ack (Auto)  t=3.7  steps: Production:X0 Control:X3 Safety:X1  outputs: Heat Oven
Temperature = 85  t=3.8  steps: Production:X0 Control:X4 Safety:X1  outputs:
Start  t=4  steps: Production:X1 Control:X4 Safety:X1  outputs: Heat Oven Right Motor
Stop  t=4.2  steps: Production:X2 Control:X4 Safety:X1  outputs: Heat Oven Right Motor
3 s later  t=7.2  steps: Production:X0 Control:X4 Safety:X1  outputs:
```

| `GrafcetEngine` | Rôle |
|-----------------|------|
| `graphs(nets)` | les GRAFCET à exécuter ensemble (références croisées, ordres de forçage) |
| `start()`, `stop()`, `reset()` | compile les réceptivités et démarre ; arrête et restaure les étapes initiales |
| `setInput(name, value)`, `inputs()` | écrit et lit les entrées (entiers) |
| `step(dt)` | un cycle : le temps passe, ordres de forçage, évolution jusqu'à stabilité, actions |
| `output(name)`, `outputs()` | les sorties |
| `firings()` | les transitions tirées pendant le dernier cycle |
| `stepTime(net, id)`, `transitionTimer(net, id)` | depuis combien de temps une étape est active ; le délai d'une transition |
| `receptivity(net, id)->timers()` | l'état des opérateurs de temps (`30s/X1`) d'une transition |
| `onWarning` | appelé avec les avertissements (cible de forçage inconnue, graphes instables) |

`DocumentInfo` (troisième argument de `importAllNetsFromJSON`) donne les descriptions et les types des entrées et sorties.

## 3. Temps de cycle et dates au plus tôt

```c++
#include "MaxPlus/Algorithms.hpp"
#include "MaxPlus/Flowshop.hpp"

Net net;
importFlowshop(net, "examples/Flowshops/simple.flowshop");
std::vector<std::vector<double>> dates;     // dates[t][k]
earliestDates(net, 3, dates);               // the Gantt chart: 3 cycles
CriticalCycleResult cc = findCriticalCycle(net);   // cc.durations: λ
std::cout << showDaterEquation(net, "", true, false).str();
```

Cela donne les nombres du [chapitre 7](07-Timed-Event-Graphs.md#6-tutorial-how-fast-is-my-factory) : `M1_C1: 0 6 12`, `λ = 6`, `M2_C1(k) = max(2 + M1_C1(k), M2(k - 1))`. `toSysLin()` et `toAdjacencyMatrices()` donnent les matrices, `operationOf()` décompose `M1_C2` en sa machine et son convoyeur.

## 4. Simuler un réseau temporisé

`Engine` (`PetriNet/Engine.hpp`) exécute un réseau de Petri ou un réseau de Petri temporisé comme une simulation à événements discrets : il possède le marquage et ne modifie pas le réseau. Le temps avance d'événement en événement, donc les dates sont exactes quels que soient les pas passés à `advance()` ; ce sont les dates au plus tôt de l'équation dater (max,+) pour un graphe d'événements temporisé.

```c++
#include "PetriNet/Engine.hpp"

Engine::Options options;
options.policy = Engine::Policy::Random;   // or Priority, FirstDeclared
options.seed = 42;                         // 0: drawn, engine.seed() gives it
Engine engine(net, options);
engine.onFire = [](size_t t, double date, size_t k) { /* ... */ };
engine.reset();                            // marking of the net, date 0
engine.advanceTo(30.0);                    // or advance(dt), update(clock)
// engine.marking(), engine.flights() (tokens on their way),
// engine.firingDates()[t], engine.isDeadlocked()
```

`options.manual = true` ne fait tirer une transition que lorsqu'elle est armée (`engine.arm(t)`), une fois, comme les clics de l'éditeur sur un réseau de Petri. `Transition::delay` fait attendre une transition, activée, avant de tirer ; `Transition::priority` ordonne les conflits avec `Policy::Priority`.

`Simulation` est la simulation de l'éditeur par-dessus : démarrée et arrêtée, avancée à chaque image, le marquage affiché dans les places du réseau et restauré à la fin, GRAFCET inclus.

```c++
#include "PetriNet/Simulation.hpp"

Simulation sim(net);
sim.onTrace.connect([](std::string const& s) { std::cout << s << "\n"; });
sim.start();
for (int i = 0; i < 300; ++i)
    sim.step(0.05f);              // 0.05 time unit per step
// sim.firingDates(transition_id): the dates of its firings
```

```text
t = 0.00  M1 starts P1 (k = 1), until 2.00
t = 2.00  M2 starts P1 (k = 1), until 3.00
t = 2.00  M1 starts P2 (k = 1), until 5.00
```

## 5. Dessiner un réseau dans votre fenêtre

`NetViewer` (`src/Editor/NetViewer.hpp`, bibliothèque `TimedPetriViewer`) dessine un réseau, en lecture seule, dans la fenêtre Dear ImGui courante de votre programme : l'utilisateur peut panoramiquer, zoomer et ajuster. Votre programme possède le réseau et son marquage, le viewer peut donc afficher un GRAFCET qui tourne ailleurs.

```c++
tpne::NetViewer viewer;                 // keep it from frame to frame
tpne::NetViewer::Options options;       // captions, actions, highlighted arcs ...
viewer.draw("##grafcet", net, options); // inside ImGui::Begin() ... End()
```

## 6. Exécuter vos fonctions : le runtime

Le runtime (`Runtime/Runtime.hpp`, `-lTimedPetriRuntime`) exécute un réseau dessiné dans l'éditeur en appelant vos fonctions : **une transition est une fonction**. Quand une transition peut tirer, le runtime prend ses jetons d'entrée, appelle sa fonction, et quand la fonction se termine donne les jetons de sortie, **colorés par son résultat** (`Success`, `Failed` ou une couleur à vous) sauf si l'arc impose sa propre couleur. Un arc depuis une place demande une couleur : c'est ainsi que le résultat d'une fonction choisit la suite.

```text
 (Ready) -> [detect] -> (Seen) -> [grasp] --Success--> (Held) -> [place] -> (Done)
                           ^                 \
                           +---- [retry] <----Failed
```

```c++
#include "IO/IO.hpp"
#include "Runtime/Runtime.hpp"

Net net;                                         // drawn in the editor
loadFromFile(net, "pick.json");                  // arcs from Held: Success / Failed

Runtime rt(net, std::make_shared<ThreadPoolExecutor>(4));
rt.on("detect", [] { camera.detect(); });               // nothing: Success
rt.on("grasp",  [] { return gripper.grasp(); });        // bool: Success / Failed
rt.on("place",  [] { arm.place(); });
// "retry" has no function: it fires at once (rt.unregisteredTransitions())
rt.guard("place", [](Context const& c) { return !estop; });

auto order = rt.instantiate("order_42", {}, { { "Done", Success } });  // goal
Color result = rt.run(*order);       // blocking: Success, Deadlocked, Canceled, Failed
// or, without blocking, from the loop of your program (ImGui, ROS, Arduino):
while (rt.step(*order)) { /* ... */ }
```

- Une fonction ne renvoie rien (Success), un `bool` (Success ou Failed) ou une `Color`. Tout type fonctionne avec une fonction libre `Color fire(T const&)`, et optionnellement `cancel()`, `pause()`, `resume()`, `isSynchronous()` (trouvées par ADL).
- `InlineExecutor` (par défaut) appelle les fonctions dans le fil de `run()` ou `step()` ; `ThreadPoolExecutor` sur des threads ; `TaskflowExecutor` sur [Taskflow](https://github.com/taskflow/taskflow) avec `TPNE_WITH_TASKFLOW`.
- Seul le fil de `run()` ou `step()` touche le marquage : les autres threads postent des requêtes (fin d'une fonction, `inject()`, `cancel()`).
- Une transition sans place d'entrée tire quand `rt.inject(*order, "button")` le demande (un bouton, un message).
- Réseaux de Petri temporisés : les arcs avec une durée donnent leurs jetons plus tard ; `Transition::delay` attend, activée, avant de tirer. Un timeout est une transition temporisée en conflit avec la lente, menant à une place d'erreur.
- `order->log()` est le journal : chaque transition `Scheduled`, `Started`, `Completed` avec son résultat et sa date. `Options::journal` reçoit chaque ligne au fil de l'eau : enregistrez-la avec `toJsonLine()` (`Runtime/Journal.hpp`, JSON lines). Après un crash, `rt.restore("order_42", journal, Runtime::Recovery::Restart)` reconstruit l'instance à partir du journal et rappelle les fonctions qui tournaient (`Recovery::Fail` les termine avec `Failed`).
- Sans fichier (une carte) : construisez le réseau dans le code, les positions ne servent pas : `Place& ready = net.addPlace(1u, "Ready"); Transition& detect = net.addTransition("detect"); net.addArc(ready, detect);` (et `net.findArc(...)->weight`, `->kind`, `->color = net.colors.add("Blue")`).
- `saveJournal(order->log(), net.colors, "orders.xes")` écrit le journal en JSON lines, CSV ou XES (process mining), selon l'extension.
- `JournalPublisher` (`Runtime/JournalPublisher.hpp`, `-lzmq`) publie le journal : l'éditeur montre le programme en cours ([chapitre 18](18-Remote-Control.md#4-follow-a-running-program)).
- Un réseau peut être la fonction d'une transition d'un autre réseau (macro-étape) : `parent.on("pick", SubNet(child_runtime, { { "Done", Success } }))` exécute une instance du réseau enfant à chaque tir ; sa fin (`Success`, `Deadlocked`, `Canceled`, `Failed`) est le résultat de la transition, et annuler le parent l'annule.

## 7. Parcourir la structure d'un réseau

Les identifiants sont des indices : `net.places()[3].id == 3`, les clés sont `"P3"`, `"T0"`. Un arc stocke ses deux extrémités en `NodeId` (type et identifiant), donc un réseau copié ne référence jamais les nœuds de l'original. Le réseau donne les nœuds et les arcs autour d'un nœud :

```c++
for (Transition const& t : net.transitions())
{
    for (Arc const& arc : net.arcsIn(t))          // input places of t
        std::cout << net.placeOf(arc).key << " -> " << t.key << "\n";
    bool fireable = net.canFire(t);               // enabled and receptivity
}
Node const& from = net.node(arc.from);            // place or transition
```

Supprimer un nœud garde les identifiants `0 .. n-1` : le dernier nœud du même type prend l'identifiant (et la clé) de celui supprimé, et ses arcs le suivent. Les références aux nœuds et aux arcs sont invalides après une suppression. Les algorithmes donnent les arcs par index dans `net.arcs()` (`CriticalCycleResult::arcs`, les arcs erronés de `isEventGraph()`, `NetViewer::Options::highlighted_arcs`).

---

## 8. Vérifier les propriétés d'un réseau

```c++
#include "Analysis/Analysis.hpp"            // -lTimedPetriAnalysis

Properties p = properties(net);             // explores up to 100000 markings
if (p.deadlock)                             // the marking and the path to it
    std::cout << "deadlock after " << p.deadlock_path.size() << " firings\n";
std::string error;
auto pinv = pInvariants(net, error);        // y.C = 0, y >= 0 (minimal supports)
std::cout << report(net);                   // all of it, as text
```

`reachabilityGraph()` et `coverabilityGraph()` (Karp-Miller) donnent les graphes de marquages, `toDot()` les écrit pour Graphviz. Les durées ne sont pas prises en compte ; Karp-Miller refuse les arcs inhibiteurs et reset et les capacités. `tpne-check` est la même chose en terminal ou en CI : son code de sortie vaut 1 lorsqu'un réseau peut deadlocker.

---

Suivant : [11. De l'éditeur au code](12-Code-Generation.md), tout le parcours avec un exemple. Les mots sont dans le [glossaire](Glossary.md) ; livres et autres outils dans la [bibliographie](Bibliography.md).
