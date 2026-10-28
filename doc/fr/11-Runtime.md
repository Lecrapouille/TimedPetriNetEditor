# 11. ⚙️ Le runtime : une transition est une fonction


[![fr](https://lecrapouille.github.io/icons/fr.png)](../fr/11-Runtime.md) [![en](https://lecrapouille.github.io/icons/en.png)](../en/11-Runtime.md)

**Ce que vous allez apprendre**

- l'idée empruntée à [Symmetri](https://github.com/thorstink/Symmetri) : votre programme est piloté par un réseau de Petri, chaque transition appelle une de vos fonctions, et le résultat de la fonction choisit la suite ;
- le fonctionnement interne du runtime : le réseau et ses instances, la boucle qui possède le marquage, les exécuteurs, les couleurs des résultats ;
- gardes, événements injectés, durées, temporisations, priorités, réseaux dans les réseaux ;
- le journal d'une instance, et comment reprendre après un plantage ;
- tout le chemin d'un dessin à un robot en marche, puis cinq programmes écrits à côté des réseaux de [`examples/`](../../examples/README.md).

**Prérequis** : [3. Réseaux de Petri](03-Petri-Nets.md) et [5. Réseaux de Petri étendus](05-Extended-Petri-Nets.md) (jetons colorés) ; [6. Réseaux de Petri temporisés](06-Timed-Petri-Nets.md) pour les durées. Il faut savoir lire du C++.

---

## 1. Pourquoi piloter un programme avec un réseau de Petri ?

Un programme qui commande un robot, une machine ou une chaîne de services web fait plusieurs choses en parallèle, attend des réponses, réessaie quand quelque chose échoue, et ne doit jamais finir dans un état que personne n'a prévu. Écrit à la main avec des threads, des drapeaux et des callbacks, cette logique est dispersée dans le code et difficile à vérifier. Un réseau de Petri la met dans un seul dessin :

- une **place** est un état ou un stock (« un objet est vu », « la pince est libre ») ;
- une **transition** est une action (« saisir l'objet ») ;
- les **jetons** indiquent quelles actions peuvent tourner maintenant, et plusieurs jetons signifient plusieurs choses en parallèle.

Dessinez le réseau dans l'éditeur, vérifiez-le ([chapitre 4](04-Analysis.md) : pas d'interblocage, places bornées), puis laissez un petit moteur l'exécuter : ce moteur est le **runtime**. Votre programme ne fournit que les fonctions des transitions.

### 1.1 L'idée de Symmetri

[Symmetri](https://github.com/thorstink/Symmetri) (Thomas Horstink, licence BSD-3) exécute des réseaux de Petri ainsi. Son idée tient en une phrase :

> **Une transition est une fonction.** Quand une transition peut franir, le runtime prend ses jetons d'entrée, appelle la fonction, puis place les jetons de sortie. La valeur renvoyée par la fonction (`Success`, `Failed`, ou une couleur à vous) devient la **couleur** des jetons produits, et cette couleur décide quelle transition pourra les prendre ensuite.

C'est une machine à états, mais :

- plusieurs états peuvent être actifs en même temps (le parallélisme des réseaux de Petri est gratuit) ;
- la branche suivante est choisie par le **résultat de la fonction**, pas par une condition calculée ailleurs.

TimedPetriNetEditor a commencé comme éditeur et analyseur. Il a ensuite repris l'idée de Symmetri et écrit son propre runtime, la bibliothèque `libTimedPetriRuntime` (`src/Runtime/`), qui garde ce qui rend Symmetri bon et ajoute ce que l'éditeur savait déjà : durées, gardes, analyse, et l'éditeur comme débogueur visuel ([chapitre 13](13-Debugging.md)).

### 1.2 Un exemple, toutes les idées

```text
 (Ready) ─► [detect] ─► (Seen) ─► [grasp] ─► (Held) ──Success──► [place] ─► (Done)
                           ▲                    │
                           └──── [retry] ◄──────┘ Failed
```

`grasp` donne son jeton à `Held` **coloré par son résultat**. L'arc `Held → place` demande des jetons `Success`, l'arc `Held → retry` des jetons `Failed` : un seul des deux peut prendre le jeton. Quand la pince glisse, l'objet revient à `Seen` et on retente `grasp`. `retry` n'a pas de fonction : elle franit tout de suite.

![Pick and place](../../examples/Runtime/PickAndPlace/PickAndPlace.png)

*Le réseau [`PickAndPlace.json`](../../examples/Runtime/PickAndPlace/PickAndPlace.json) dans l'éditeur : les couleurs demandées par les arcs sont écrites dessus.*

## 2. Anatomie du runtime

### 2.1 Le réseau, le runtime, les instances

| Objet | Rôle | Qui le possède |
|-------|------|----------------|
| `Net` | La structure : places, transitions, arcs, poids, couleurs, durées. Dessinée dans l'éditeur et chargée avec `loadFromFile()`, ou construite dans le code. Elle ne change pas tant que le runtime vit. | votre programme |
| `Runtime` | Le réseau et les fonctions de ses transitions (`on()`), leurs gardes (`guard()`), l'exécuteur, les options. | votre programme |
| `Instance` | Une exécution du réseau : son **marquage coloré**, sa date, son journal, son résultat. Un runtime peut exécuter plusieurs instances en parallèle (une par commande, par client, par cellule robot). | `runtime.instantiate()` |

```c++
#include "IO/IO.hpp"
#include "Runtime/Runtime.hpp"
using namespace tpne;

Net net;
loadFromFile(net, "examples/Runtime/PickAndPlace/PickAndPlace.json");
Runtime runtime(net, std::make_shared<ThreadPoolExecutor>(4u));
runtime.on("detect", [] { camera.detect(); });      // nothing returned: Success
runtime.on("grasp",  [] { return gripper.grasp(); }); // bool: Success or Failed
runtime.on("place",  [] { arm.place(); });
auto order = runtime.instantiate("order_42", {}, { { "Done", Success } });
Color result = runtime.run(*order);
```

Une fonction est trouvée par le **nom** de sa transition (sa légende, ou sa clé `T2`). Plusieurs transitions avec la même légende partagent la fonction (les trois `End of call` des appels d'urgence).

### 2.2 Marquage initial, objectif, résultat

`instantiate(id, initial, goal)` :

- **initial** : les jetons au départ, sous la forme `{ place, color, count }`. Vide : les jetons dessinés dans le réseau, colorés `Success`.
- **goal** : quand ces jetons sont présents, l'instance se termine avec `Success`. Vide : l'instance se termine quand plus rien ne peut se produire.

Une instance se termine avec l'un de ces **résultats** (ce sont aussi des couleurs) :

| Résultat | Quand |
|----------|-------|
| `Success` | le marquage objectif est atteint |
| `Deadlocked` | plus rien ne peut se produire, et aucun jeton `Failed` ne reste (sans objectif, c'est la fin normale) |
| `Failed` | plus rien ne peut se produire et un jeton `Failed` reste que personne n'a pris ; ou un cycle de transitions a franit plus de `Options::max_firings_per_date` fois (10000) à la même date |
| `Canceled` | `runtime.cancel(instance)` a été appelé |

« Plus rien ne peut se produire » signifie : aucune transition ne peut franir, aucune fonction ne tourne, aucun jeton n'est en route le long d'un arc avec une durée, aucun délai ne compte, le réseau n'a pas de transition source que `inject()` pourrait faire franir, et aucune transition n'attend sa garde (§3.1).

### 2.3 Les couleurs

Un jeton a une **couleur** : un petit nombre, nommé dans la table `net.colors`. Les premières sont intégrées : `Success`, `Failed`, `Canceled`, `Deadlocked` ; les vôtres sont listées dans le fichier (`"colors": ["Good", "Bad"]`) ou ajoutées avec `net.colors.add("Good")`.

- Un **arc d'entrée** (place → transition) prend des jetons de sa couleur seulement ; sans couleur, il prend n'importe quel jeton.
- Un **arc de sortie** (transition → place) donne des jetons de sa propre couleur s'il en a une, sinon de **résultat de la fonction**.
- Une fonction ne renvoie rien (`Success`), un `bool` (`true` : `Success`, `false` : `Failed`) ou une `Color` (n'importe laquelle de la table).

Les autres extensions du [chapitre 5](05-Extended-Petri-Nets.md) sont aussi respectées : poids, arcs inhibiteurs, arcs de reset, capacités (comptant les jetons en route).

### 2.4 La boucle : un seul propriétaire, pas de verrou

```text
       ┌────────────── thread de run() ou step() : le seul à toucher le marquage ─────────────┐
       │                                                                                         │
 file  │ 1. prendre les requêtes postées par les autres threads (fin d'une fonction, inject, cancel) │
 d'at- │ 2. jetons arrivant à cette date (arcs avec une durée)                                  │
 ten- ─┼►3. objectif atteint ?                              → Success, la fin                       │
 tes  │ 4. choisir une transition qui peut franir (priorité, politique), prendre ses jetons d'entrée, et │
   ▲  │      - fonction synchrone : l'appeler maintenant, donner les jetons de sortie                      │
   │  │      - sinon : la poster à l'exécuteur                                                    │
   │  │ 5. répéter 4 jusqu'à ce que plus rien ne puisse franir à cette date ; plus rien à attendre ? → la fin   │
   │  └─────────────────────────────────────────────────────────────────────────────────────────┘
   │
   └── exécuteur (un thread du pool) : appelle la fonction, puis poste la requête
       « donner les jetons de sortie, colorés par ce résultat »
```

Le marquage d'une instance n'est modifié que par le thread qui appelle `run()` ou `step()`. Les fonctions qui tournent sur d'autres threads ne le touchent jamais : quand elles se terminent, elles **postent une requête** (une petite fonction `Instance& → void`, un *reducer*), et le propriétaire l'applique à son tour suivant. C'est le schéma de Redux ou des acteurs : le marquage n'a **pas besoin de verrou**, et une fonction peut être lente sans bloquer les autres.

Conséquence : **une transition dure le temps de sa fonction**. Ses jetons d'entrée disparaissent quand elle démarre (`Scheduled`), ses jetons de sortie apparaissent quand la fonction se termine (`Completed`). Le runtime vérifie toutes les transitions une fois, puis seulement celles autour des places dont les jetons ont changé : sur un réseau de 1921 transitions, cela le rend rapide (§9).

### 2.5 Exécuteurs : où tournent les fonctions

| Exécuteur | Les fonctions tournent | Pour |
|-----------|------------------------|------|
| `InlineExecutor` (défaut) | dans le thread de `run()` ou `step()`, tout de suite, l'une après l'autre | tests (déterministes), micro-contrôleurs, fonctions courtes |
| `ThreadPoolExecutor(n)` | sur `n` threads | bureau, services, fonctions lentes (caméra, réseau) |
| `TaskflowExecutor(n)` | sur les threads work-stealing de [Taskflow](https://github.com/taskflow/taskflow), avec `-DTPNE_WITH_TASKFLOW` | programmes utilisant déjà Taskflow |

Taskflow ne remplace pas la boucle : il exécute un graphe fixe de tâches, sans marquage, choix ni jetons. Ici il prête seulement ses threads. Toute classe avec `void post(std::function<void()> task)` peut être un exécuteur (`Runtime/Executor.hpp`).

### 2.6 Fonctions de n'importe quel type

`on()` prend un `Callback`, qui accepte **n'importe quel type** (l'effacement de type de Sean Parent) : une fonction, un lambda, ou tout objet `T` pour lequel une fonction libre `fire(T const&)` existe dans l'espace de noms de `T` (trouvée par ADL). Des fonctions libres optionnelles ajoutent des comportements ; sans elles, les défauts s'appliquent :

| Fonction libre | Appelée quand | Défaut |
|----------------|---------------|--------|
| `Color fire(T const&)` | la transition franit (obligatoire, sauf si `T` est callable) | |
| `void cancel(T const&)` | l'instance est annulée pendant que la fonction tourne | rien |
| `void pause(T const&)`, `void resume(T const&)` | `runtime.pause()`, `runtime.resume()` | rien |
| `bool isSynchronous(T const&)` | pour savoir où l'appeler | `false` : postée à l'exécuteur ; `true` : appelée tout de suite par la boucle |

```c++
namespace robot {
struct Grasp { Gripper* gripper; };
Color fire(Grasp const& g)   { return g.gripper->grasp() ? Success : Failed; }
void cancel(Grasp const& g)  { g.gripper->open(); }
}
runtime.on("grasp", robot::Grasp{ &gripper });
```

Un réseau entier peut être un tel objet : c'est ainsi que les réseaux s'imbriquent (§3.6).

## 3. Contrôler le flux

### 3.1 Gardes

Une **garde** est une condition supplémentaire d'une transition, fournie par le programme : la transition ne franit que tant qu'elle renvoie `true`. Elle voit le réseau, l'instance, son marquage et sa date (`Context`) :

```c++
runtime.guard("place", [](Context const& c) { return !estop && c.tokens("Held", Success) > 0u; });
```

Une transition **activée par le marquage mais bloquée par sa garde** ne termine pas l'instance : la garde peut s'ouvrir plus tard (arrêt d'urgence relâché, capteur, temps). `run()` regarde alors les gardes à nouveau toutes les 10 ms ; avec `step()`, elles sont évaluées à chaque appel. Les gardes sont évaluées dans le thread de la boucle : gardez-les courtes, et utilisez des variables atomiques quand d'autres threads modifient ce qu'elles lisent.

### 3.2 Sources et événements injectés

Une transition **sans place d'entrée** (une source) ne franit jamais toute seule dans le runtime : elle franit une fois à chaque demande du programme, depuis n'importe quel thread :

```c++
runtime.inject(*instance, "button");   // a button, a message, an interrupt
```

C'est ainsi que le monde extérieur parle au réseau : un opérateur, la réponse d'un service web, un message ROS. Tant que le réseau a une source, une instance ne se termine jamais toute seule (un événement peut encore arriver) : terminez-la avec `cancel()`.

### 3.3 Temps : durées et délais

Dans un **réseau de Petri temporisé** ([chapitre 6](06-Timed-Petri-Nets.md)) :

- un arc de sortie avec une **durée** donne ses jetons ce temps après le tir (les jetons sont « en route », et comptent pour les capacités) ;
- une transition avec un **délai** ne franit qu'après avoir pu franir pendant ce temps.

Le runtime garde les dates dans une liste d'événements : aucun thread ne dort pour une durée. Dans un réseau de Petri non temporisé, les durées sont ignorées.

### 3.4 Temporisations

Une temporisation est une **transition retardée en conflit** avec la lente :

```text
 (Waiting) ──► [Receive] ◄── (Answer) ◄── injected by the service
     │
     └──► [timeout, delay 0.5 s] ──► (Error)
```

Si la réponse arrive la première, `Receive` prend le jeton `Waiting` et la temporisation ne peut plus franir. Sinon, après 0,5 s, `timeout` le prend. Voir [`examples/Runtime/MicroServices`](../../examples/Runtime/MicroServices/README.md) et l'exemple éditeur [`Timeout`](../../examples/Timed-Petri-Nets/Timeout/Timeout.json).

### 3.5 Conflits et priorités

Quand plusieurs transitions peuvent franir, `Options::policy` choisit :

| Politique | Choisit |
|-----------|---------|
| `Priority` (défaut) | la plus haute `Transition::priority`, puis la première déclarée |
| `FirstDeclared` | le plus petit identifiant |
| `Random` | au hasard (`Options::seed`, 0 : tirage aléatoire) |

Avec les couleurs, la plupart des conflits disparaissent : les arcs choisissent par la couleur du jeton.

### 3.6 Un réseau dans un réseau

Un réseau peut être la fonction d'une transition d'un autre réseau, comme une macro-étape :

```c++
auto pick = std::make_shared<Runtime>(pick_net);
pick->on("grasp", ...);
cell.on("pick", SubNet(pick, { { "Done", Success } }));   // Runtime/SubNet.hpp
```

Chaque tir de `pick` exécute une nouvelle instance du sous-réseau jusqu'à sa fin ; son résultat (`Success`, `Deadlocked`, `Canceled`, `Failed`) est le résultat de la transition, donc il colore les jetons et choisit la branche du parent. Annuler, mettre en pause ou reprendre le parent fait de même pour les instances du sous-réseau en cours.

## 4. run() ou step()

| | `run(instance)` | `step(instance, now)` |
|-|-----------------|-----------------------|
| Bloque | jusqu'à la fin de l'instance | jamais : fait ce qui est dû à la date `now`, puis revient |
| Temps | le temps réel (`Options::clock`) | la date que vous donnez (secondes depuis le premier appel), ou `Options::clock` |
| Pour | un programme dont le travail principal est le réseau | un programme avec sa propre boucle : Dear ImGui, un timer ROS, le `loop()` d'un Arduino, une simulation |
| Renvoie | le résultat | `false` quand l'instance est terminée |

Avec `step()` et une date que vous augmentez vous-même, le temps est **simulé** : une exécution de plusieurs heures prend des microsecondes, et c'est identique à chaque run (les exemples `BoundedBuffer` et `TrafficLight` fonctionnent ainsi).

## 5. Le journal

Chaque instance écrit un **journal** : pour chaque tir d'une transition, trois lignes `Scheduled` (jetons pris), `Started` (fonction appelée), `Completed` (jetons donnés, avec le résultat) et leurs dates. Une transition sans fonction n'a pas de `Started`.

```text
{"instance":"order_42","transition":"grasp","event":"Scheduled","date":0.05015729}
{"instance":"order_42","transition":"grasp","event":"Started","date":0.05015729}
{"instance":"order_42","transition":"grasp","event":"Completed","result":"Failed","date":0.150359935}
{"instance":"order_42","transition":"retry","event":"Scheduled","date":0.150359935}
{"instance":"order_42","transition":"retry","event":"Completed","result":"Success","date":0.150359935}
```

- `instance->log()` le donne à la fin ; `Options::journal` reçoit chaque ligne au fil de l'eau (pour le sauver au fur et à mesure, ou le publier à l'éditeur) ; `Options::log = false` ne garde pas de journal (micro-contrôleur).
- `saveJournal(log, net.colors, "orders.xes")` (`Runtime/Journal.hpp`) l'écrit en lignes JSON (`.jsonl`), CSV (`.csv`) ou XES (`.xes`, pour les outils de process mining) : voir [chapitre 13](13-Debugging.md).

### 5.1 Reprise après un plantage

Quand le journal est sauvegardé ligne par ligne, un programme qui a planté peut reconstruire ses instances : `restore()` rejoue le journal depuis le marquage initial, et les transitions dont la fonction tournait (`Scheduled` sans `Completed`) sont soit rappelées (`Recovery::Restart` : la fonction doit être idempotente), soit terminées avec `Failed` (`Recovery::Fail`).

```c++
std::vector<LogEntry> entries;
fromJsonLines(journal_text, net.colors, entries);
auto order = runtime.restore("order_42", entries, Runtime::Recovery::Restart,
                             {}, { { "Done", Success } });
Color result = runtime.run(*order);
```

Un programme qui a sauvegardé son journal et a planté pendant que `grasp` tournait (la fonction a levé une exception) donne, relancé avec ce code (§9.8 le fait dans un programme complet) :

```text
crash!
{"instance":"order_42","transition":"detect","event":"Scheduled","date":0.0}
{"instance":"order_42","transition":"detect","event":"Started","date":0.0}
{"instance":"order_42","transition":"detect","event":"Completed","result":"Success","date":0.0}
{"instance":"order_42","transition":"grasp","event":"Scheduled","date":0.0}
{"instance":"order_42","transition":"grasp","event":"Started","date":0.0}
grasp called again
placed
order_42: Success
```

## 6. Cibles

| Cible | Comment | Exemple |
|-------|---------|---------|
| Bureau | `ThreadPoolExecutor`, `run()` | [PickAndPlace](../../examples/Runtime/PickAndPlace/README.md) |
| Services | un pool ; les réponses des services sont injectées ; temporisations | [MicroServices](../../examples/Runtime/MicroServices/README.md) |
| ROS 2 | un timer du nœud appelle `step()` ; les fonctions envoient des actions ROS, leurs résultats reviennent par `inject()` | [Ros2Node](../../examples/Runtime/Ros2Node/README.md) |
| Micro-contrôleur | construire le réseau dans `setup()` (pas de système de fichiers), `InlineExecutor`, `Options::log = false`, `step()` dans `loop()` avec le temps de `millis()` | [TrafficLight](../../examples/Runtime/TrafficLight/README.md) |

Le cœur et le runtime se compilent avec `-fno-exceptions -fno-rtti`, comme sur une carte (`make -C examples` le vérifie). Après `instantiate()`, le runtime n'alloue de la mémoire que pour les requêtes de `inject()` et pour les jetons en route le long des arcs avec une durée. Un vrai AVR sans bibliothèque standard reste hors de portée : pour lui, générez le code d'un GRAFCET ([chapitre 12](12-Code-Generation.md)).

## 7. Le runtime et la simulation de l'éditeur

L'éditeur simule un réseau pour vous le faire comprendre ; le runtime l'exécute pour piloter un programme. Ils partagent la règle de tir mais diffèrent volontairement :

| | Simulation de l'éditeur | Runtime |
|-|--------------------------|---------|
| Couleurs des jetons | utilisées : les arcs prennent leur couleur ; les résultats sont tirés parmi les couleurs demandées après la transition | utilisées : les arcs prennent leur couleur, les fonctions donnent la leur |
| Une transition franit | réseau de Petri : quand vous cliquez dessus ; réseau temporisé : dès qu'elle le peut | dès qu'elle le peut, et que sa garde l'autorise |
| Sa durée | les durées des arcs | le temps de sa fonction, plus les durées des arcs (réseaux temporisés) |
| Transitions sources | réseau de Petri : au clic ; réseau temporisé : encore quand les jetons de son dernier tir sont arrivés | seulement sur `inject()` |
| Conflits | *Simulation > Conflicts* | `Options::policy`, priorités |
| Fin | jamais (vous l'arrêtez) | objectif, interblocage, échec ou annulation |

## 8. 🤖 Du dessin à un robot en marche

Tout le chemin avec le pick and place du §1.2.

### 8.1 Dessiner le réseau

Ouvrez [`PickAndPlace.json`](../../examples/Runtime/PickAndPlace/PickAndPlace.json), ou dessinez-le ([chapitre 2](02-Getting-Started.md)) : un clic ajoute une place, un clic droit une transition, un glisser depuis un nœud trace un arc, un double clic nomme un nœud. Puis les couleurs : clic droit sur l'arc `Held → place` > *Color of the tokens* > `Success`, sur `Held → retry` > `Failed`.

### 8.2 Le vérifier

*Analysis > Properties*, ou `tpne-check` dans un terminal ou une CI :

![Propriétés du pick and place](pics/11-PickAndPlace-Properties.png)

```text
$ tpne-check --allow-deadlock examples/Runtime/PickAndPlace/PickAndPlace.json
examples/Runtime/PickAndPlace/PickAndPlace.json:
Net: Pick and place (Petri net, 4 places, 4 transitions)
Reachable markings: 5
Deadlock: yes, marking (P0=0, P1=0, P2=0, P3=1) after firing T0 (detect), T1 (grasp), T2 (place)
Dead transitions: none
Bounded: yes (1-bounded)
Live: no, not live: T0 (detect), T1 (grasp), T2 (place), T3 (retry)
P-invariants:
  P0 (Ready) + P1 (Seen) + P2 (Held) + P3 (Done) = 1
T-invariants:
  T1 (grasp) + T3 (retry)
Note: The colors of the tokens are considered: a transition gives one of the colors asked by the arcs after it (Success if none is), a function of the runtime returning another color is not considered.
```

Le seul interblocage est `Done` : la fin du travail, voulue, d'où `--allow-deadlock` (sans cela le code de sortie vaut 1, ce qui fait échouer une CI). L'invariant P dit qu'il y a toujours exactement un objet quelque part ; l'invariant T que `grasp` puis `retry` redonne le même marquage : la boucle de nouvelle tentative. L'analyse suit les couleurs comme le runtime : `grasp` donne un jeton `Success` ou `Failed` à `Held` (les deux couleurs demandées après), donc `Held` est deux marquages, et seul `place` prend le premier, seul `retry` le second : 5 marquages.

### 8.3 Générer le programme

*File > Export > C++ functions for the runtime*, `pick_and_place.cpp` : le réseau construit dans le code (une carte n'a pas de système de fichiers), une fonction par transition, et un `main()` :

```c++
//! \brief grasp: takes Seen, gives Held.
//! Its result chooses what comes next: Failed (retry), Success (place).
Color grasp()
{
    // TODO: what grasp does.
    return Success;
}
```

Le fichier entier est montré au [chapitre 12](12-Code-Generation.md).

### 8.4 Écrire les fonctions

Remplissez-les avec le code du robot. Ici, des fausses : la pince glisse une fois.

```c++
Color grasp()
{
    static int attempts = 0;
    std::this_thread::sleep_for(std::chrono::milliseconds(300)); // the gripper
    bool const held = (++attempts >= 2);                          // slips once
    std::cout << "gripper: " << (held ? "holding" : "slipped") << std::endl;
    return held ? Success : Failed;
}
```

`detect` et `place` affichent et renvoient `Success` ; `retry` reste tel que généré. Compilez et exécutez :

```sh
g++ -std=c++20 -I<TimedPetriNetEditor>/src pick_and_place.cpp \
    -L<TimedPetriNetEditor>/build -lTimedPetriRuntime -lTimedPetriNet -pthread -o pick
./pick
```

```text
camera: object seen
gripper: slipped
gripper: holding
arm: placed
Pick and place: Success
```

L'instance se termine avec `Success` : son objectif, un jeton dans `Done`, est atteint.

### 8.5 Le voir tourner

Compilez avec `-DTPNE_DEBUG -lzmq` : le programme publie son journal, et l'éditeur connecté (*Debug > Connect*) déplace les jetons pendant que le programme tourne. C'est le [chapitre 13](13-Debugging.md).

## 9. Les programmes d'exemple

Chacun est à côté de son réseau, se compile avec `make -C examples` et s'exécute avec `make -C examples run` (depuis la racine du projet, après `make`).

### 9.1 ☕ Coffee : le résultat choisit la branche

[`examples/Petri-Nets/Coffee/Coffee.cpp`](../../examples/Petri-Nets/Coffee/Coffee.cpp) exécute [`Coffee.json`](../../examples/Petri-Nets/Coffee/Coffee.json). Le contrôleur de pièces `Insert Coin` renvoie `true` (`Success`) pour une bonne pièce, `false` (`Failed`) pour une fausse ; les arcs depuis `Coin Inserted` demandent `Success` pour `Accept` et `Failed` pour `Reject Coin`. Une garde laisse `Insert Coin` franir seulement tant qu'il reste des clients en attente.

```c++
runtime.on("Insert Coin", [&coins, &coin]() {
    bool const good = coins[coin++];
    std::cout << "coin " << coin << ": " << (good ? "good" : "fake") << std::endl;
    return good; // true: Success, false: Failed
});
runtime.guard("Insert Coin", [&coins, &coin](Context const&) { return coin < coins.size(); });
```

```text
  heater: water is hot
coin 1: good
  coin accepted
  coffee 1 served
  heater: water is hot
coin 2: fake
  coin given back
coin 3: good
  coin accepted
  coffee 2 served
  heater: water is hot
coin 4: good
  coin accepted
  coffee 3 served
  heater: water is hot
3 coffees for 4 coins
```

### 9.2 ✈️ Landing gear : le réseau comme couche de sécurité

[`examples/Petri-Nets/LandingGear/LandingGear.cpp`](../../examples/Petri-Nets/LandingGear/LandingGear.cpp) exécute [`LandingGear.json`](../../examples/Petri-Nets/LandingGear/LandingGear.json). Le pilote donne des ordres : chacun ouvre la garde de sa transition. Le réseau ne laisse franir que les ordres autorisés : `Extend` lit `Low speed`, donc un ordre de sortir le train à grande vitesse attend. `Start warning` n'a pas de garde : elle franit toute seule quand l'avion vole vite avec le train sorti.

```c++
for (char const* name : { "Faster", "Slower", "Retract", "Extend", "Ack" })
{
    runtime.guard(name, [&order, name](Context const&) { return order[name]; });
    runtime.on(name, [&order, name]() { order[name] = false; ... });
}
```

```text
t = 1  pilot: Faster
        Faster done
        WARNING: gear down at high speed!
t = 2  pilot: Retract
        Retract done
t = 3  pilot: Ack
        Ack done
t = 4  pilot: Extend  (refused by the net, the order waits)
t = 5  pilot: Slower
        Slower done
        Extend done
end: [Low speed] [Gear down] [Warning off]
```

Entre deux ordres, rien ne peut franir mais l'instance ne se termine pas : les transitions attendent leurs gardes (§3.1).

### 9.3 🎨 Colors : vos propres couleurs

[`examples/Extended-Petri-Nets/Colors/Colors.cpp`](../../examples/Extended-Petri-Nets/Colors/Colors.cpp) exécute [`Colors.json`](../../examples/Extended-Petri-Nets/Colors/Colors.json), dont les couleurs `Good` et `Bad` sont listées dans le fichier. `Inspect` renvoie l'une d'elles ; `Ship` prend les jetons `Good`, `Scrap` les `Bad`.

```c++
Color const good = *net.colors.find("Good");
Color const bad = *net.colors.find("Bad");
runtime.on("Inspect", [&part, good, bad]() {
    bool const ok = (++part != 2); // the second part has a scratch
    return ok ? good : bad;
});
```

```text
inspect part 1: Good
inspect part 2: Bad
inspect part 3: Good
  ship it
  ship it
  scrap it
lot_1: Deadlocked, shipped 2, scrapped 1
```

Sans objectif, `Deadlocked` est la fin normale : toutes les pièces sont traitées.

### 9.4 📦 Bounded buffer : extensions, temps et événements

[`examples/Extended-Petri-Nets/BoundedBuffer/BoundedBuffer.cpp`](../../examples/Extended-Petri-Nets/BoundedBuffer/BoundedBuffer.cpp) exécute [`BoundedBuffer.json`](../../examples/Extended-Petri-Nets/BoundedBuffer/BoundedBuffer.json) sur une horloge simulée (`step()` avec des dates). Les gardes donnent le rythme des machines : une pièce à chaque unité de temps, un paquet de 2 pièces toutes les 3. `Stop` et `Ask flush` sont des sources : le programme les injecte. `Restart` attend l'opérateur (une garde).

```c++
runtime.on("Produce", [&now, &next_part]() { next_part = now + 1.0; });
runtime.guard("Produce", [&next_part](Context const& c) { return c.now >= next_part; });
...
if (date == 8) runtime.inject(*line, "Stop");
if (date == 12) restart = true;
if (date == 15) runtime.inject(*line, "Ask flush");
runtime.step(*line, now);
```

```text
 date  Buffer  Packed  Stopped  event
    0       2       0        0
    1       3       0        0
    2       4       0        0
    3       3       1        0
    4       4       1        0
    5       4       1        0
    6       3       2        0
    7       4       2        0
    8       4       2        1  Stop: the producer waits
    9       2       3        1
   10       2       3        1
   11       2       3        1
   12       1       4        0  Restart
   13       2       4        0
   14       3       4        0
   15       0       5        0  Ask flush: Buffer emptied
   16       1       5        0
   17       2       5        0
   18       1       6        0
   19       2       6        0
   20       3       6        0
```

La capacité maintient `Buffer` à 4 (dates 2 à 8 : le producteur attend le conditionneur) ; l'arc inhibiteur depuis `Stopped` arrête le producteur (dates 9 à 11) ; l'arc de reset de `Flush` vide le tampon d'un coup (date 15).

### 9.5 🚑 Emergency calls : le type d'appel est une couleur

[`examples/Petri-Nets/EmergencyCalls/EmergencyCalls.cpp`](../../examples/Petri-Nets/EmergencyCalls/EmergencyCalls.cpp) exécute [`EmergencyCalls.json`](../../examples/Petri-Nets/EmergencyCalls/EmergencyCalls.json). Quand un opérateur de niveau 1 répond, la fonction renvoie le type d'appel, une couleur du réseau (`Advice`, `Urgent`, `Critical`) ; les trois arcs depuis `Call screening` prennent le jeton de leur couleur.

```c++
std::vector<std::string> const calls = { "Critical", "Advice", "Urgent", "Urgent",
                                         "Advice", "Critical", "Urgent" };
runtime.on("Level 1 answers", [&net, &calls, &call]() {
    return *net.colors.find(calls[call++]);
});
```

Le journal donne les transitions dans l'ordre où elles se sont terminées (le réseau n'est pas temporisé : les durées de ses arcs ne sont pas attendues) :

```text
  1  Level 1 answers      -> Critical
  2  Critical
  3  Level 2 answers
  4  Level 1 leaves
  5  End of call
  6  Level 1 answers      -> Advice
  7  Level 1 answers      -> Urgent
  8  Urgent
  9  Level 1 answers      -> Urgent
 10  Urgent
 11  Level 1 answers      -> Advice
 12  Transfer to level 2
 13  Level 1 answers      -> Critical
 14  Critical
 15  Level 2 answers
 16  Level 1 leaves
 17  End of call
 18  Level 1 answers      -> Urgent
 19  Urgent
 20  Transfer to level 2
 21  Transfer to level 2
 22  Level 2 answers
 23  Level 2 answers
 24  Level 2 answers
 25  Advice
 26  Advice
 27  End of call
 28  End of call
 29  End of call
 30  End of call
 31  End of call
Deadlocked: 3 urgent and 2 critical calls handled by level 2
```

### 9.6 🚨 Priorities : qui gagne un conflit

[`examples/Extended-Petri-Nets/Priorities/Priorities.cpp`](../../examples/Extended-Petri-Nets/Priorities/Priorities.cpp) exécute [`Priorities.json`](../../examples/Extended-Petri-Nets/Priorities/Priorities.json) sur une horloge simulée. La voiture et l'ambulance veulent le carrefour à la même date ; `Options::policy` choisit, comme *Simulation > Conflicts* dans l'éditeur :

```c++
Runtime::Options options;
options.policy = Engine::Policy::Random; // Priority (default), Random, FirstDeclared
options.seed = 7u;                       // the same seed, the same draws
Runtime runtime(net, std::make_shared<InlineExecutor>(), options);
```

```text
Policy Priority:
  t =  0.0  Ambulance crosses
  t =  2.0  Car crosses
  t =  7.0  Ambulance crosses
...
Policy Random, first to cross for the seeds 1 to 10:
  Car Ambulance Ambulance Ambulance Ambulance Ambulance Ambulance Car Ambulance Ambulance
  the car first 42 times for the seeds 1 to 100
Seed 7 twice: the same crossings
```

### 9.7 🍝 Philosophers : les jetons sont les verrous

[`examples/Petri-Nets/Philosophers/Philosophers.cpp`](../../examples/Petri-Nets/Philosophers/Philosophers.cpp) exécute [`Philosophers.json`](../../examples/Petri-Nets/Philosophers/Philosophers.json) sur un pool de trois threads. Un repas est la fonction de `PhiN eats` : il dure 100 ms, sur son propre thread. Le programme n'a pas de mutex : un repas ne commence que lorsque le runtime a pris les jetons de sa coutellerie. Le programme compte les philosophes qui mangent en même temps :

```text
   0 ms  Phi0 starts meal 1 (1 eating)
 102 ms  Phi1 starts meal 1 (1 eating)
...
 817 ms  Phi2 starts meal 3 (1 eating)
 919 ms  Canceled: 3, 3 and 3 meals, at most 1 philosopher eating at once
```

Jamais plus d'un, comme le disaient les invariants P de l'analyse : le réseau a prouvé la propriété avant que le programme ne tourne.

### 9.8 🏭 Producer and consumers : reprise après plantage

[`examples/Petri-Nets/ProducerConsumer/ProducerConsumer.cpp`](../../examples/Petri-Nets/ProducerConsumer/ProducerConsumer.cpp) exécute [`ProducerConsumer.json`](../../examples/Petri-Nets/ProducerConsumer/ProducerConsumer.json) et est le §5.1 dans un vrai programme. Le journal va dans un fichier ligne par ligne ; le programme meurt pendant que deux consommateurs travaillent ; le second run reconstruit l'instance depuis le fichier et rappelle les deux `Consume` :

```c++
options.journal = [&](LogEntry const& entry) {
    journal << toJsonLine(entry, net.colors) << std::endl; // flushed: it survives a crash
};
...
fromJsonLines(text.str(), net.colors, entries);
auto line = runtime->restore("line", entries, Runtime::Recovery::Restart);
```

```text
  crash! (while the parts 3 to 4 are used)
Second run
  journal: 47 lines, 5 parts made, 2 used
  restored: Buffer 5, Accepted 0, Ready 0; Consume running at the crash: 2, called again
...
Done: 6 parts made, 10 used (expected 10)
```

Aucune pièce perdue, aucune utilisée deux fois.

### 9.9 Autres programmes

[`examples/Runtime/`](../../examples/Runtime/README.md) contient les programmes pour chaque cible : pick and place sur un pool de threads, micro-services avec temporisation et process mining, un feu tricolore écrit comme sketch Arduino, un nœud ROS 2, le débogueur et le benchmark.

## 10. Comparé à Symmetri

| Symmetri | Ce runtime |
|----------|------------|
| Jetons colorés par le résultat | conservé : l'idée centrale |
| Un seul thread de boucle et des reducers | conservé |
| Callbacks de n'importe quel type, par effacement de type et ADL | conservé (`fire`, `cancel`, `pause`, `resume`, `isSynchronous`) |
| Réseaux imbriqués | conservé (`SubNet`) |
| Places et transitions en chaînes partout | noms à la construction, indices à l'exécution |
| Pas de temps sur les arcs ou transitions | durées des arcs et délais des transitions, sans thread qui dort |
| Pas de garde | gardes (les réceptivités d'un GRAFCET) |
| Pas d'analyse | l'éditeur et `tpne-check` : interblocages, bornes, vivacité, invariants, (max,+) |
| Un marquage par objet `PetriNet` | un `Net` partagé par plusieurs `Instance` |
| `fire(app)` bloque l'appelant | `run()` bloque, `step()` non : pour la boucle ImGui, ROS ou Arduino |
| Un cycle sans fonction franit indéfiniment | arrêté après `max_firings_per_date` tirs, l'instance `Failed` |
| Journal | les mêmes trois événements, sauvés en lignes JSON, CSV, XES, publiés à l'éditeur, rejoués après plantage |

Les deux runtimes ont été mesurés sur la même machine (AMD Ryzen 7 9700X, Linux, g++ 16.2, `-O3`, Symmetri au commit `22e47a4`) :

| Réseau | Tirs | TimedPetriNetEditor | Symmetri |
|-----|:-------:|--------------------:|---------:|
| défaut de Symmetri (2 transitions, 6 jetons) | 6 | 1,0 µs | 0,6 µs |
| `auto.pnml` (1921 transitions, 29152 arcs) | 12 / 19 | **0,43 ms** | 2,8 ms |
| `n1.pnml`, `sat.pnml` (un cycle sans durée) | 10000 | arrêté : 2,1 ms, 3,3 ms | ne se termine jamais |

Symmetri est plus rapide sur de petits réseaux (son journal garde des indices, celui-ci des noms) ; ce runtime est 4 à 6 fois plus rapide sur de grands réseaux, car il ne vérifie que les transitions autour des places qui ont changé. La méthode et tous les chiffres : [le benchmark](../../examples/Runtime/Benchmark/README.md). Pour comparer sur les mêmes réseaux, l'éditeur lit et écrit le PNML de Symmetri (`.symmetri.pnml`, [chapitre 15](15-File-Formats.md)).

## 11. Pièges

- **Une fonction n'est jamais appelée** : son nom ne correspond pas à la légende d'une transition (`on()` renvoie `false`) ; `runtime.unregisteredTransitions()` liste les transitions sans fonction, qui franissent tout de suite.
- **L'instance se termine avec `Deadlocked` trop tôt** : cherchez une couleur demandée par un arc qu'aucune fonction ne donne (le jeton attend indéfiniment), ou un jeton dans une place qu'aucune transition ne lit. Les sources et les gardes fermées ne terminent jamais une instance.
- **L'instance `Failed` avec « Transitions keep firing »** : un cycle de transitions sans fonction ni durée (`max_firings_per_date`). Donnez une fonction, une garde ou une durée à l'une d'elles.
- **Fonctions sur un pool partagent des données** : elles tournent sur plusieurs threads ; protégez ce qu'elles partagent. Les gardes tournent dans le thread de la boucle.
- **Analyse et couleurs** : `tpne-check` ignore les couleurs ; un réseau sûr pour le runtime peut sembler dangereux pour lui, et l'inverse.

## 12. Pour aller plus loin

- [12. Génération de code](12-Code-Generation.md) : le squelette du programme, et le code C++ des GRAFCET.
- [13. Débogage](13-Debugging.md) : voir le programme tourner dans l'éditeur.
- [16. Bibliothèque C++](16-Library.md) : l'API de `libTimedPetriRuntime`.
- [Symmetri](https://github.com/thorstink/Symmetri) et sa documentation.
- Sean Parent, *Inheritance Is The Base Class of Evil* (GoingNative 2013), pour l'effacement de type de `Callback`.

---

Suite : [12. Génération de code](12-Code-Generation.md).
