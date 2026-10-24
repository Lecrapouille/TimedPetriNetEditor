# TimedPetriNetEditor — Feuille de route

Objectif : finaliser l'éditeur, corriger les défauts identifiés, et en extraire une bibliothèque C++ embarquable qui exécute un réseau de Petri en appelant des fonctions (inspirée de [Symmetri](https://github.com/thorstink/Symmetri)).

---

## 1. Comment fonctionne Symmetri

### 1.1 L'idée en une phrase

**Une transition = une fonction C++.** Quand une transition est franchissable, Symmetri
consomme ses jetons d'entrée, appelle la fonction, puis dépose les jetons de sortie.
La valeur de retour de la fonction (`Success`, `Failed`, ou une couleur perso) devient
la **couleur** des jetons produits, et c'est cette couleur qui décide quelle transition
suivante pourra les consommer.

C'est donc bien une machine à états, mais :

- plusieurs états peuvent être actifs en même temps (parallélisme natif) ;
- le choix de la branche suivante est fait par le **résultat de la fonction**, pas par
  une condition évaluée à part.

### 1.2 Les quatre ingrédients

| Ingrédient | Type Symmetri | Rôle |
|---|---|---|
| Le réseau | `Net = unordered_map<Transition, pair<inputs, outputs>>` | Pour chaque transition : liste de `(place, couleur)` en entrée et en sortie |
| Le marquage initial | `Marking = vector<pair<Place, Token>>` | Jetons de départ |
| Le marquage but | `Marking` | Si atteint → l'exécution termine en `Success` |
| Les callbacks | `registerCallback("nom", fonction)` | La fonction appelée quand la transition tire |

Le réseau peut aussi être chargé depuis un fichier PNML ou GRML (formats d'autres éditeurs).

### 1.3 Exemple : une machine à états « prendre un objet »

```
 (Prêt) ──► [detecter] ──► (Objet vu) ──► [saisir] ──Success──► (En main) ──► [deposer] ──► (Fini)
                                              │
                                              └──Failed──► (Raté) ──► [reessayer] ──► (Objet vu)
```

```cpp
using namespace symmetri;

Net net = {
  {"detecter",  {{{"Pret", Success}},      {{"ObjetVu", Success}}}},
  {"saisir",    {{{"ObjetVu", Success}},   {{"Saisie", Success}}}},
  // deux transitions lisent la même place, mais pas la même couleur :
  {"deposer",   {{{"Saisie", Success}},    {{"Fini", Success}}}},
  {"reessayer", {{{"Saisie", Failed}},     {{"ObjetVu", Success}}}},
};

Marking initial = {{"Pret", Success}};
Marking goal    = {{"Fini", Success}};

auto pool = std::make_shared<TaskSystem>(4);          // pool de threads
PetriNet app(net, "commande_42", pool, initial, goal);

app.registerCallback("detecter", [] { camera.detect(); });          // void → Success
app.registerCallback("saisir",   [] { return gripper.grasp()        // Token
                                          ? Success : Failed; });
app.registerCallback("deposer",  [] { arm.place(); });
// "reessayer" n'a pas de callback : il tire instantanément (DirectMutation)

Token result = fire(app);   // bloque jusqu'au but, un deadlock ou une annulation
auto log = getLog(app);     // journal : (case_id, transition, état, horodatage)
```

Point clé : `saisir` produit un jeton dans `Saisie` **coloré par son résultat**.
Si c'est `Success`, seul `deposer` peut le consommer ; si c'est `Failed`, seul
`reessayer` le peut. C'est le mécanisme de branchement.

### 1.4 La boucle d'exécution (le cœur à comprendre)

```
           ┌──────────────── thread "Petri loop" (seul à toucher le marquage) ───────────────┐
           │                                                                                 │
 file de   │  1. dépile tous les "reducers" (lambdas Petri& → void) et les applique          │
 reducers ─┼─►2. but atteint ?            → Success, fin                                      │
 (lock-    │  3. calcule les transitions franchissables, triées par priorité               │
  free)    │  4. pour chacune : retire les jetons d'entrée, puis                               │
   ▲       │       - callback synchrone  → l'appelle tout de suite, dépose les sorties         │
   │       │       - callback asynchrone → l'envoie au pool de threads                          │
   │       │  5. rien de franchissable ET rien en cours ? → Deadlocked                         │
   │       └─────────────────────────────────────────────────────────────────────────────────┘
   │
   └── pool de threads : exécute la fonction, puis poste un reducer
       « dépose N jetons de couleur <résultat> dans les places de sortie »
```

- Taskflow (https://github.com/taskflow/taskflow) ne remplace pas la boucle : il
  exécute un graphe de tâches figé, sans marquage ni jetons multiples ni choix par
  couleur. Il peut seulement servir de pool de threads. Le runtime dépend donc d'une
  interface `Executor { post(std::function<void()>) }` avec `InlineExecutor`
  (Arduino, tests déterministes), `ThreadPoolExecutor` (maison) et, en option,
  `TaskflowExecutor`.

Conséquences :

- **Pas de verrou sur le marquage** : un seul thread le modifie, les workers ne font
  que poster des messages (pattern Redux / acteur).
- **Une transition dure le temps de sa fonction** : les jetons d'entrée disparaissent
  au début, les jetons de sortie apparaissent à la fin. La place implicite « en cours »
  est la liste `scheduled_callbacks`.
- **Fin d'exécution explicite** : `Success` (but atteint), `Deadlocked` (plus rien à
  faire), `Canceled`, ou `Failed`.

### 1.5 Les callbacks : effacement de type à la Sean Parent

`Callback` accepte **n'importe quel type** `T` pour lequel une fonction libre
`fire(const T&)` existe (trouvée par ADL). Les autres points d'extension sont optionnels
et ont une version par défaut :

```cpp
Token fire(const T&);          // obligatoire (lambda, foncteur, objet…)
void  cancel(const T&);        // défaut : ne fait rien
void  pause(const T&);
void  resume(const T&);
bool  isSynchronous(const T&); // défaut : false → exécuté sur le pool
Eventlog getLog(const T&);
```

Comme `PetriNet` implémente lui-même `fire`, `cancel`, etc., **un réseau peut être le
callback d'une transition d'un autre réseau** : hiérarchie gratuite (sous-réseaux,
macro-étapes GRAFCET).

### 1.6 Autres détails utiles

- **Priorités** : `PriorityTable` (`int8_t` par transition) pour résoudre les conflits.
- **Transitions d'entrée** : `getInputTransitionHandle("t")` renvoie une
  `std::function<void()>` qu'un code externe (callback réseau, ISR, message ROS…) appelle
  pour injecter un événement.
- **Journal** : chaque transition logge `Scheduled`, `Started` puis son résultat, avec
  horodatage. Exploitable pour du *process mining* ou un replay.
- Licence BSD-3 : on peut s'inspirer et réutiliser du code librement (en citant).

### 1.7 Ce qu'on garde, ce qu'on fait mieux

| Symmetri | Pour notre lib |
|---|---|
| Jetons colorés par le résultat | **On garde.** C'est l'idée centrale. |
| Boucle mono-thread + reducers | **On garde.** |
| Callbacks par effacement de type + ADL | **On garde.** |
| Réseaux imbriqués | **On garde.** |
| Places/transitions en `std::string` partout dans l'API | Noms à la construction, **indices** à l'exécution |
| Pas de temps sur les arcs/transitions | **Ajout** : durées et graphes d'événements temporisés (notre spécialité) |
| Pas de gardes séparées | **Ajout** : gardes optionnelles (réceptivités GRAFCET) |
| Pas d'analyse statique | **Ajout** : (max,+), Howard, accessibilité, invariants |
| Un marquage par objet `PetriNet` | **Séparer** `Net` (immuable, partagé) et `Instance` (marquage) |
| `fire(app)` bloque le thread appelant | **Ajout** : mode `step()` non bloquant pour s'intégrer à une boucle existante (ImGui, ROS, Arduino) |

---

## 2. Phase 0 — Hygiène

- [x] Retirer `compile_commands.json` du dépôt et l'ajouter au `.gitignore`.
- [x] Mettre à jour ou supprimer `doc/gui.md` et `doc/mqtt.md` (liens barrés dans le README).
- [ ] Traduire le `TODO` en issues GitHub étiquetées (`bug`, `grafcet`, `editor`, `core`).
- [x] Ajouter `.clang-format` et `.clang-tidy` ; activer `-Wall -Wextra -Wconversion` en CI
      (déjà dans les drapeaux par défaut de MyMakefile).
- [x] CI : ajouter un job ASan + UBSan qui lance les tests.

## 3. Phase 1 — Extraire un cœur propre `tpne-core`

But : `src/PetriNet` compilable seul, sans ImGui ni coordonnées, consommable par CMake.

### 3.1 Séparer le modèle et la vue

- [x] Retirer `x`, `y` de `Node` ; créer `struct Layout { std::vector<Vec2> places, transitions; }`
      possédé par le `Document` de l'éditeur. Écart : le `Layout` est possédé par le `Net`
      (`net.layout()`, `net.position(node)`), à part des nœuds, pour rester aligné sans effort
      sur les ajouts, suppressions, copies et annulations ; l'`Engine` et les algorithmes
      ne le lisent pas. L'API du cœur ne prend plus de coordonnées : `addPlace(tokens, caption)`,
      `addTransition(caption)`, la position se donne par `net.position(node)`.
- [x] Déplacer `findPlaceAt` / `findTransitionAt` dans l'éditeur (`placeAt()`, `nodeAt()` de `Drawable`).
- [x] Déplacer `ForceDirected` dans l'éditeur (il ne manipule que le layout), ainsi que
      `arrangeTopDown()` (`Editor/TopDownLayout`).
- [x] Format JSON : garder la compatibilité de lecture, écrire layout et topologie dans deux objets distincts (`"net"` / `"layout"`) : révision 2, objet `"layout"` dans chaque réseau.

### 3.2 Corriger la gestion mémoire

- [x] Remplacer `Node& from/to` dans `Arc` par des indices typés :
      `struct NodeId { Node::Type type; size_t id; }` (l'identifiant est l'indice).
- [x] Remplacer `arcsIn` / `arcsOut` (pointeurs bruts) par des listes d'adjacence
      d'indices recalculées dans une seule fonction privée (`net.arcsIn(node)`).
- [x] Supprimer les `operator=` par destruction + placement new ; retirer les membres
      `const` qui les rendaient nécessaires (ou rendre les types non copiables).
- [x] Remplacer `reinterpret_cast<Place&>` par `static_cast` (ou disparaît avec les indices).
- [x] Suppression d'un nœud : stratégie explicite (compactage + table de renommage,
      ou tombstones) et test dédié (`TestRemoveNodeRenumbers`).
- [x] Passer les `FIXME` de `Net` (`generateArcsInArcsOut`, `resetReceptivies`) en privé
      (`resetReceptivies` reste public : le moteur GRAFCET et l'import l'appellent).

### 3.3 Supprimer les singletons

- [x] `Sensors::instance()` → objet `Sensors` passé à la `Simulation` (ou dans un `Context`)
      (déjà fait : les entrées sont dans le `GrafcetEngine` du document).
- [x] `NetRegistry::instance()` → registre possédé par le document / le runtime (déjà fait).
- [x] `Net::Settings::maxTokens` / `firing` (globaux) → `Place::capacity` donnée par le réseau.
- [x] Test : deux réseaux dans le même processus (`TestCapacityIsPerNet`).

### 3.4 Séparer le marquage du réseau

- [x] `Net` devient la topologie immuable pendant l'exécution (l'`Engine` possède le
      marquage ; `Simulation` le recopie dans les places pour l'affichage).
- [x] `Marking` = `std::vector<size_t>` (ou multiset coloré, cf. phase 4) : `PetriNet/Firing.hpp`.
- [x] Matrices `Pre` / `Post` creuses (réutiliser `SparseMatrix`) ; fonction pure
      `Marking fire(Net const&, Marking const&, TransitionId)` (+ `consume` / `produce`
      pour les réseaux temporisés ; les listes d'adjacence servent de Pre/Post creuses,
      `incidenceMatrices()` les donne en `SparseMatrix` pour les analyses).

### 3.5 Build

- [x] Avec Makefile compiler les lib `tpne::core`, `tpne::maxplus`, `tpne::editor`
      (`libTimedPetriNet`, `libTimedPetriMaxPlus` dans `src/MaxPlus`, `libTimedPetriGUI`).
- [x] Ne **pas** compiler le cœur avec `-ffast-math` mais garder la compatibilité (`-fno-fast-math`).
- [x] Remplacer le sentinel `NaN` « pas de durée » par `std::optional<double>`
      (`std::optional<float>`, comme le reste des durées du format de fichier ; `durationOrZero()`).

## 4. Phase 2 — Sémantique et simulation

### 4.1 Découper `Simulation`

- [x] `Engine` (cœur) : sémantique pure, aucun float d'animation (`PetriNet/Engine.hpp`).
- [x] `TokenAnimator` (éditeur) : interpolation visuelle des jetons sur les arcs,
      à partir des vols (`Engine::Flight`) de l'`Engine`.
- [x] `GrafcetEngine` : règles d'évolution GRAFCET (franchissement simultané,
      actions, forçages), à part de l'`Engine` (règles IEC 60848 différentes).
- [x] Valeur propre / Howard : hors simulation, dans `tpne::maxplus` (`Algorithms` ;
      `Simulation` garde seulement λ calculé au démarrage pour la courbe).

### 4.2 Temps

- [x] Horloge injectable : `TimeSource = std::function<double()>` (`realTimeSource()` ou simulée).
- [x] Temps en `double` ou `std::chrono` ; plus d'accumulation de `float dt`.
- [x] Simulation à événements discrets (file triée par date) pour les réseaux temporisés : avance directement à la prochaine date, sans dépendre du framerate.

### 4.3 Conflits et déterminisme

- [x] Remplacer `shuffleTransitions` par une politique explicite :
      `Priority`, `FirstDeclared`, `Random(seed)` (menu Simulation > Conflicts).
- [x] Champ `priority` sur `Transition`, éditable dans l'inspecteur.
- [x] Graine aléatoire enregistrée → simulations rejouables.

### 4.4 Extensions du formalisme (TODO existant)

- [x] `TypeOfNet` ne décrit que la façon dont le réseau évolue (`PetriNet`,
      `TimedPetriNet`, `TimedEventGraph`, `GRAFCET`) ; pas de `ColorPetriNet` ni
      d'`ExtendedPetriNet`. Les extensions sont portées par les arcs et les places
      (`ArcKind { Normal, Inhibitor, Reset }`, `weight`, `capacity`, `color`).
      `features(net)` les calcule et `allows(type, feature)` les valide :
      `TimedEventGraph` et `GRAFCET` n'en acceptent aucune. Les analyses se
      désactivent selon les extensions utilisées, pas selon le type.
- [x] Plus tard (après §3.1) : retirer `TimedEventGraph` du cœur, devenu
      `TimedPetriNet` + `isEventGraph()` + option d'affichage compact dans le `Layout`
      (`Layout::compact`, `convertToEventGraph()` ; le JSON garde le type « Timed event graph »
      pour les anciens lecteurs, relu en `TimedPetriNet` compact).
- [x] Arcs inhibiteurs.
- [x] Arcs de reset.
- [x] Poids sur les arcs (si pas déjà complets partout).
- [x] Capacité des places (ou contrôler `maxTokens` statique → par place).

### 4.5 Tests

- [x] Tests unitaires `Engine` : franchissement, conflits, priorités, inhibiteurs
      (`EngineTests`, `FiringTests`, `ExtensionsTests`, `SimulationTests`).
- [x] Tests GRAFCET : chaque règle d'évolution de la norme IEC 60848 (`TestGrafcetRules`).
- [x] Tests de non-régression : charger chaque fichier de `data/examples`, simuler
      N pas avec une graine fixe, comparer au journal de référence (golden files) :
      `tests/golden/`, politique « premier déclaré » (le tirage aléatoire de la STL
      diffère entre Linux et macOS), `TPNE_UPDATE_GOLDEN=1` pour les réécrire.

## 5. Phase 3 — GRAFCET

- [x] Extraire les méthodes GRAFCET de `Simulation` vers `Grafcet.hpp/cpp` (déjà fait :
      `GrafcetEngine`, `Simulation` ne fait que déléguer).

## 6. Phase 4 — Runtime embarquable « à la Symmetri » : `tpne-runtime`

### 6.1 Jetons colorés

- [x] `Token` = couleur (`uint8_t` indexé dans une table de noms), constantes
      `Success`, `Failed`, `Canceled`, couleurs utilisateur (`PetriNet/Color.hpp`, `net.colors`).
- [x] Arcs d'entrée : `(place, couleur attendue)` ; couleur par défaut = « n'importe
      laquelle » pour rester compatible avec les réseaux classiques (`Arc::color`).
- [x] Arcs de sortie : couleur = résultat du callback, ou couleur fixée sur l'arc.

### 6.2 API

```cpp
auto net = tpne::load("pick.json");         // dessiné dans l'éditeur

tpne::Runtime rt(net, tpne::ThreadPool(4));
rt.on("detecter", [] { camera.detect(); });
rt.on("saisir",   [] { return gripper.grasp() ? tpne::Success : tpne::Failed; });
rt.guard("deposer", [](tpne::Context const& c) { return !estop; }); // optionnel

auto inst = rt.instantiate("commande_42");  // un marquage par instance
rt.inject(inst, "bouton_start");            // transition d'entrée

// au choix :
auto res = rt.run(inst);                    // bloquant, comme fire(app)
while (rt.step(inst, now())) { /*...*/ }    // non bloquant, dans ta boucle
```

- [x] `Callback` par effacement de type + points d'extension ADL (`fire`, `cancel`,
      `pause`, `resume`, `isSynchronous`) : `Runtime/Callback.hpp` ; exécuteurs
      `InlineExecutor`, `ThreadPoolExecutor`, `TaskflowExecutor` (option `TPNE_WITH_TASKFLOW`).
- [x] Un `Runtime` est lui-même un `Callback` → réseaux imbriqués (macro-étapes) :
      `SubNet(runtime, goal)` comme fonction d'une transition.
- [x] `unregisteredTransitions()` : liste les transitions sans fonction (utile au démarrage).

### 6.3 Boucle d'exécution

- [x] Thread propriétaire du marquage + file de reducers (lock-free, ex. `moodycamel::ConcurrentQueue`)
      (file protégée par un mutex, sans dépendance ; une file lock-free reste possible).
- [x] Callbacks synchrones exécutés en ligne, asynchrones sur le pool.
- [x] États finaux : `Success` (marquage but), `Deadlocked`, `Canceled`, `Failed` (des couleurs,
      comme chez Symmetri : un sous-réseau rend son état comme résultat).
- [x] Transitions temporisées : durée fixe de l'arc/transition gérée par la file
      d'événements (sans thread qui dort).
- [x] Timeout = transition concurrente temporisée vers une place d'erreur
      (documenter le motif dans les exemples) : `examples/runtime/MicroServices.cpp`.

### 6.4 Journal et persistance

- [x] Journal `(instance, transition, état, date)` comme Symmetri (`instance->log()`,
      écrit au fil de l'eau par `Options::journal`).
- [x] Sérialisation du journal (JSON lines) ; reconstruction du marquage par replay
      (`Runtime/Journal.hpp`, `Runtime::restore()`).
- [x] Reprise après crash : marquage + transitions en cours → politique (relancer / Failed)
      (`Runtime::Recovery::Restart`, `Recovery::Fail`).

### 6.5 Cibles

- [x] Desktop / micro-services : pool de threads (`PickAndPlace.cpp`, `MicroServices.cpp`).
- [x] ROS 2 : exécuteur qui appelle `step()` dans un timer, callbacks = actions ROS
      (`Ros2Node.cpp`, non compilé ici : il faut `rclcpp`).
- [x] Arduino : mode mono-thread sans allocation dynamique après l'init,
      `step()` dans `loop()`, pas d'exceptions ni de RTTI (`TrafficLight.cpp`, compilé avec
      `-fno-exceptions -fno-rtti` ; restent des allocations pour `inject()` et les jetons
      en route : un vrai AVR sans STL demandera une génération de code).

## 7. Phase 5 — L'éditeur comme IDE du runtime

- [x] Éditeur de couleurs de jetons et de couleur attendue sur les arcs (clic droit sur un
      arc, colonne de la table des arcs, couleurs du réseau dans « Document », couleurs
      dessinées sur les arcs).
- [x] Mode « connexion » : le runtime publie ses événements sur ZeroMQ (sens inversé
      du remote actuel), l'éditeur affiche le marquage en direct (`Runtime/JournalPublisher.hpp`,
      en-tête seul : seul le programme qui l'inclut lie libzmq ; *Debug > Connect*, option `-c` ;
      `examples/runtime/Debugger.cpp`).
- [x] Rejouer un journal dans l'éditeur (barre de temps, pas à pas avant/arrière) : fenêtre
      *Journal* (simulation, fichier `.jsonl`, programme connecté), *Replay* par `Runtime::restore()`.
- [x] Surligner les transitions sans callback (à partir d'un manifeste exporté par le runtime) :
      `toManifest()`, reçu en direct ou *Debug > Open a manifest*.
- [x] Import/export PNML compatible Symmetri (pour comparer sur les mêmes réseaux) : `.pnml`
      (standard + `<toolspecific>`) et `.symmetri.pnml` ; GRML (GreatSPN) en plus.
- [x] Remplacer l'export « code GRAFCET C++ » par un export « squelette de callbacks »
      pour `tpne-runtime` : nouvel export `.cpp` ; l'export GRAFCET est gardé et refait (§10).
- [x] Bugs listés : centrage du graphe (correct ; les boucles d'un graphe d'événements
      compact sont maintenant dessinées), sélection d'arcs en mode graphe d'événements
      (supprimer plusieurs arcs renumérotait les places cachées : arcs retrouvés par leurs
      deux transitions).
- [x] Tester le remote ZeroMQ (test d'intégration en CI avec un client Python) :
      `tests/remote/test_remote.py` (remote de l'éditeur sous `xvfb-run`, et journal publié).

## 8. Phase 6 — Analyses (l'avantage sur Symmetri et Symfony)

- [x] Graphe d'accessibilité (borné) et de couverture (Karp-Miller) : bibliothèque
      `tpne::analysis` (`src/Analysis`, `libTimedPetriAnalysis`). Le temps n'est pas
      considéré ; Karp-Miller refuse les arcs inhibiteurs, de reset et les capacités
      (règle non monotone), le graphe d'accessibilité les suit.
- [x] Détection de deadlocks (marquage et chemin le plus court qui y mène) et de
      transitions mortes.
- [x] Bornitude (Karp-Miller quand l'exploration s'arrête), vivacité L4 (composantes
      fortement connexes terminales du graphe d'accessibilité complet).
- [x] P-invariants et T-invariants (semi-flots minimaux, algorithme de Farkas ; arcs
      normaux seulement, signalé sinon). Onglet « Properties » de la fenêtre Analysis
      de l'éditeur, pour tous les types de réseaux.
- [x] Commande CLI `tpne-check net.json` utilisable en CI (code de sortie ≠ 0 si deadlock) :
      `tools/tpne-check`, `--max-states N`, `--allow-deadlock` (un marquage but est un
      deadlock), lancée en CI sur des exemples.

## 9. Phase 7 — Publication

- [x] Doc : un guide « de l'éditeur au code » avec l'exemple pick-and-place
      (`doc/11-From-Editor-To-Code.md`, sorties réelles).
- [x] Exemples : micro-services (HTTP fictif), robot (pick-and-place ; Robotik l'utilise
      de son côté), Arduino (feu tricolore), plus `Debugger.cpp` et `Benchmark.cpp`.
- [x] Benchmark contre Symmetri sur la même machine (`examples/runtime/Benchmark.cpp` et
      `SymmetriBenchmark.cpp`, résultats dans `examples/runtime/README.md`) : Symmetri plus
      rapide sur les petits réseaux (0,6 µs contre 1,0 µs pour 6 franchissements) ; TPNE
      4 à 6 fois plus rapide sur `auto.pnml` (1921 transitions) ; les cycles sans durée
      (`n1`, `sat`) : arrêtés par TPNE, sans fin chez Symmetri (mémoire épuisée).

## 10. Suite (liste du 24 octobre 2026)

- [x] Éditeur : clic droit sur une place (jetons, nombre max de jetons), sur un arc (poids,
      type, couleur, durée) ; double clic sur un arc : durée et poids (et jetons de la place
      d'un graphe d'événements).
- [x] Plus de logs sur la console : la simulation émet `onFired`, la fenêtre *Journal* les montre.
- [x] Génération GRAFCET → C++ refaite : toutes les GRAFCET d'un document dans une classe,
      mêmes règles que le moteur (situation stable, fronts, `t1/a/t2`, délais, actions et
      conditions, forçages), sans bibliothèque (Arduino), débogage visuel par `GRAFCET_DEBUG`
      (ZeroMQ, ou `Serial` + `tools/serial-bridge.py`) ; vérifiée cycle par cycle contre le
      moteur (`tests/codegen/run.sh`, en CI). Elle a révélé un bug du moteur : une transition
      vers une étape déjà active était bloquée (règle de capacité de Petri, pas IEC 60848).
- [x] Fenêtre d'analyse non (max,+) : onglet *Properties* pour tous les réseaux (résumé,
      deadlock et chemin rejouable dans le Journal, bornes, invariants surlignés, marques
      sur le réseau, graphe des marquages en Graphviz) ; `tpne-check --dot`.
- [x] Squelette C++ de fonctions pour le runtime (export `.cpp`, compilé en CI sur les exemples).
- [x] Backend raylib retiré : GLFW3 partout, y compris le web (boucle Emscripten, WebGL 2,
      page `shell.html` ; build web non vérifié ici faute d'Emscripten).
- [x] Projet en C++20 (`Makefile.common`, exemples ; le code GRAFCET généré reste C++11).
- [x] `Flowshop` (et l'import `.flowshop`) déplacé dans `src/MaxPlus`, enregistré par
      `registerMaxPlusFormats()`.
- [x] Imports/exports sortis du cœur : `libTimedPetriIO` (`src/IO`, `loadFromFile()`,
      `saveToFile()`) ; le cœur et le runtime n'en dépendent pas.
- [x] Export du réseau et du journal : le réseau par la lib IO (optionnelle), le journal par
      le runtime (`saveJournal()` : JSON lines, CSV, XES pour le process mining) ;
      `MicroServices.cpp` le montre.
- [x] Revue des imports/exports : échappements (Graphviz, draw.io, LaTeX, Symfony, PNML),
      poids, types d'arcs et couleurs exportés, LaTeX à l'échelle, draw.io centré, noms
      Symfony uniques, poids PN-Editor ; extension la plus longue choisie (`saveToFile("net.drawio.xml")`
      était refusé) ; nouveaux formats SVG et GRML ; validé sur tous les
      exemples (Graphviz, xmllint, YAML, relecture JSON/PNML/GRML).
- [x] Performances : `findArc()` par l'adjacence, ajout d'arcs et de nœuds sans
      reconstruire l'adjacence (chargement linéaire au lieu de quadratique), activation
      incrémentale des transitions dans le runtime.

---

## Ordre conseillé

1. Phase 0 → Phase 1 : sans cœur propre, rien d'autre n'est stable.
2. Phase 4 en version minimale (synchrone seulement) pour valider l'API sur un
   vrai cas (Robotik).
3. Phase 2 (temps, déterminisme, tests) puis le pool asynchrone de la phase 4.
4. Phase 5 : l'éditeur devient le débogueur du runtime.
5. Phases 3, 6, 7 en parallèle selon l'envie.
