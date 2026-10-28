# 📡 18. Télécommande


[![fr](https://lecrapouille.github.io/icons/fr.png)](../fr/18-Remote-Control.md) [![en](https://lecrapouille.github.io/icons/en.png)](../en/18-Remote-Control.md)

Ce que vous trouverez ici : comment un autre programme (un test, un robot, un script) pilote un éditeur en cours d'exécution : charger un réseau, le démarrer, faire tirer des transitions, lire les jetons ; et comment l'éditeur suit un programme qui exécute un réseau (§4).

---

## 1. Fonctionnement

L'éditeur écoute sur `tcp://*:5555` avec [ZeroMQ](https://zeromq.org/) (socket REP). Un client envoie une commande JSON et reçoit une réponse JSON.

```text
  votre programme  ── {"command": "fire", "params": {...}} ──►  éditeur
                   ◄── {"status": "ok", "message": "..."} ───
```

La télécommande démarre avec l'éditeur (la console affiche `Remote control listening on tcp://*:5555`) ; *View > Remote control* l'arrête ou la relance. Compiler sans elle avec `make TPNE_ZEROMQ=0`.

## 2. Tutoriel : piloter le producteur-consommateur

Ouvrez `data/examples/ProducerConsumer.json` dans l'éditeur, puis exécutez ce script Python (`pip install pyzmq`) :

```python
import json, time, zmq
s = zmq.Context().socket(zmq.REQ)
s.connect("tcp://localhost:5555")
def send(cmd):
    print(">>", json.dumps(cmd))
    s.send_string(json.dumps(cmd))
    print("<<", s.recv_string())

send({"command": "state"})
send({"command": "start"})
time.sleep(0.5)
send({"command": "fire", "params": {"transitions": [0]}})
time.sleep(0.5)
send({"command": "state"})
send({"command": "stop"})
```

Il affiche (les listes de transitions sont tronquées ici) :

```text
>> {"command": "state"}
<< {"places":[{"caption":"Producer ready","id":0,"key":"P0","tokens":1},{"caption":"Idle","id":1,"key":"P1","tokens":1},{"caption":"Buffer","id":2,"key":"P2","tokens":3},{"caption":"Accepted","id":3,"key":"P3","tokens":0},{"caption":"Ready","id":4,"key":"P4","tokens":2}],"running":false,"status":"ok","transitions":[...]}
>> {"command": "start"}
<< {"message":"Simulation started","status":"ok"}
>> {"command": "fire", "params": {"transitions": [0]}}
<< {"message":"Transitions fired","status":"ok"}
>> {"command": "state"}
<< {"places":[{"caption":"Producer ready","id":0,"key":"P0","tokens":0},{"caption":"Idle","id":1,"key":"P1","tokens":2},{"caption":"Buffer","id":2,"key":"P2","tokens":4},{"caption":"Accepted","id":3,"key":"P3","tokens":0},{"caption":"Ready","id":4,"key":"P4","tokens":2}],"running":true,"status":"ok","transitions":[...]}
>> {"command": "stop"}
<< {"message":"Simulation stopped","status":"ok"}
```

`Send` (`T0`) a tiré : `Producer ready` a perdu son jeton, `Idle` et `Buffer` en ont gagné un.

## 3. Référence : commandes

Une commande est `{"command": "<name>", "params": {...}}`. Une réponse a `"status": "ok"` ou `"error"` et un `"message"`.

| Commande | Params | Effet |
|---------|--------|------|
| `load` | `{"net": {...}}` (le JSON du [chapitre 15](15-File-Formats.md)) ou `{"path": "file.json"}` | remplace le réseau actif ; refusé pendant une simulation |
| `start` | | démarre la simulation ; refusé pour les réseaux temporisés |
| `stop` | | arrête la simulation |
| `fire` | `{"transitions": [0, 2]}` ou `{"bitfield": "101"}` (un caractère par transition) | fait tirer ces transitions (réseaux de Petri) ; refusé si la simulation n'est pas en cours |
| `state` | | `running`, les places (`id`, `key`, `caption`, `tokens`), les transitions (`id`, `key`, `caption`, `receptivity`) |
| `set_tokens` | `{"place": 1, "tokens": 3}` | fixe les jetons d'une place |
| `clear` | | vide le réseau actif ; refusé pendant une simulation |

Utilisez des réseaux de Petri ou des GRAFCET : les réseaux temporisés déplacent leurs jetons tout seuls.

La CI exécute `tests/remote/test_remote.py` (pyzmq) : il pilote l'éditeur avec ces commandes et vérifie les réponses.

## 4. Suivre un programme en cours d'exécution

Dans l'autre sens : un programme qui exécute un réseau dit à l'éditeur ce qu'il fait, et l'éditeur l'affiche (débogueur visuel). Le programme publie sur un socket ZeroMQ PUB (`tcp://*:5556` par défaut) ; l'éditeur s'abonne avec *Debug > Connect* (ou `TimedPetriNetEditor -c tcp://localhost:5556 net.json`). Le *Journal* suit le programme (son *Replay* montre le marquage à la dernière date), la fenêtre *Program* montre ses entrées et sorties et les transitions sans fonction, marquées sur le réseau.

Qui publie :

- un programme utilisant le runtime : `Runtime/JournalPublisher.hpp` (`examples/runtime/Debugger.cpp`, et les programmes de *File > Export > C++ functions for the runtime* compilés avec `-DTPNE_DEBUG`) ;
- le code GRAFCET de *File > Export > C++ GRAFCET* compilé avec `-DGRAFCET_DEBUG` (sur un Arduino, via `tools/serial-bridge.py`).

```c++
#include "Runtime/JournalPublisher.hpp"   // link with -lzmq

JournalPublisher publisher(net);           // tcp://*:5556
Runtime::Options options;
options.journal = [&publisher](LogEntry const& e) { publisher.publish(e); };
Runtime runtime(net, executor, options);
runtime.on("grasp", grasp);
publisher.manifest(runtime);               // its transitions without function
auto order = runtime.instantiate("order_42");
publisher.marking(*order);                 // for an editor connected later
```

Chaque message est un objet JSON :

| Message | Exemple | L'éditeur |
|---------|---------|------------|
| une ligne du journal | `{"instance":"order_42","transition":"grasp","event":"Completed","result":"Failed","date":1.5}` | l'ajoute au *Journal* |
| manifest | `{"type":"manifest","net":"Pick and place","places":[...],"transitions":[...],"unregistered":["retry"]}` | marque les transitions sans fonction (renvoyé chaque seconde) |
| marking | `{"type":"marking","instance":"order_42","date":1.5,"tokens":[1,0,0,0],"colors":[{"Success":1},{},{},{}]}` | démarre le replay de l'instance à partir de ces jetons, avec leurs couleurs (sans `colors` : `Success`) |
| inputs and outputs | `{"type":"io","inputs":{"Start":1},"outputs":{"Motor":true}}` | les affiche dans la fenêtre *Program* (GRAFCET) |

`event` vaut `Scheduled` (la transition a pris ses jetons), `Started` (sa fonction s'exécute) ou `Completed` (elle a donné ses jetons, colorés par `result`). Un GRAFCET publie une instance par graphe (son nom) et les clés des transitions (`T3`). *Debug > Open a manifest* marque les transitions sans fonction à partir d'un fichier écrit par `toManifest(runtime)`.

---

Suivant : [10. Utiliser la bibliothèque en C++](16-Library.md).
