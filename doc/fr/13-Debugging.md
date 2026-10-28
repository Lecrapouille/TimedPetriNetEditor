# 13. 🐞 Débogage : l'éditeur comme débogueur visuel


[![fr](https://lecrapouille.github.io/icons/fr.png)](../fr/13-Debugging.md) [![en](https://lecrapouille.github.io/icons/en.png)](../en/13-Debugging.md)

**Ce que vous allez apprendre**

- connecter l'éditeur à un programme en cours d'exécution et voir ses jetons se déplacer sur le réseau que vous avez dessiné ;
- les messages qu'envoie un programme, et comment les envoyer depuis votre propre programme, depuis le code généré d'un GRAFCET, ou depuis un Arduino ;
- la fenêtre *Journal* : lire, rejouer, enregistrer et ouvrir des journaux ;
- quoi faire d'un journal : process mining, reprise après plantage.

**Prérequis** : [11. Le runtime](11-Runtime.md), et [12. Génération de code](12-Code-Generation.md) pour les §6 et §7.

---

## 1. L'idée

Un programme piloté par un réseau de Petri a sa logique dans un dessin. Quand quelque chose ne va pas, le meilleur endroit pour regarder est ce même dessin : quels jetons sont où, quelle fonction tourne, laquelle a échoué. L'éditeur peut l'afficher, en direct, pour un programme qui tourne n'importe où :

```text
  votre programme (runtime, ou GRAFCET généré)              l'éditeur
  ┌───────────────────────────────┐                    ┌──────────────────────┐
  │ runtime ─► lignes de journal ─┼── ZeroMQ PUB ────► │ SUB : fenêtre Journal, │
  │            manifest, marquage │   tcp://*:5556     │ jetons sur le réseau,  │
  └───────────────────────────────┘                    │ fenêtre Program        │
                                                       └──────────────────────┘
```

Le programme **publie** ; l'éditeur **s'abonne**. C'est l'inverse de la télécommande du [chapitre 18](18-Remote-Control.md), où un autre programme pilote l'éditeur. Le programme n'attend pas l'éditeur : il tourne pareil qu'un éditeur écoute ou non, et plusieurs éditeurs peuvent écouter le même programme.

## 2. Essayer

L'exemple `Debugger` exécute en boucle le pick and place du chapitre 11, avec des fonctions lentes (pour voir les jetons bouger) et une pince qui échoue une fois sur trois :

```sh
make -C examples                       # après make à la racine
./build/examples/Debugger &            # publie pendant 30 s (ou : Debugger 120)
./build/TimedPetriNetEditor -c tcp://localhost:5556 examples/Runtime/PickAndPlace/PickAndPlace.json
```

Dans l'éditeur, *Debug > Connect* avec le point de terminaison `tcp://localhost:5556` fait la même chose que `-c`.

![L'éditeur qui suit un programme en marche](pics/13-Debugger.png)

*L'éditeur connecté au programme : `place` est marquée `running` (sa fonction tourne maintenant), `retry` est marquée `no function` (elle franit tout de suite) ; le Journal liste les événements de la commande `order_3` ; la fenêtre Program nomme le réseau du programme et ses transitions sans fonction.*

Ce que vous voyez :

- **Sur le réseau** : les jetons de l'instance au dernier événement ; une transition dont la fonction tourne a un halo marqué `running` ; une transition sans fonction en a un marqué `no function`.
- **Journal** (en bas) : chaque événement de chaque instance, au fil de l'eau. *Follow* garde le dernier en vue ; *Replay* affiche le marquage d'une instance à une date (§4).
- **Program** (à droite) : le point de terminaison, le réseau du programme, ses transitions sans fonction, et les entrées et sorties d'un GRAFCET (§6). Si le programme n'a pas le même nombre de transitions que le réseau ouvert, un avertissement demande : *is it the same net?*

*Debug > Disconnect* (ou *Disconnect* dans la fenêtre Program) arrête le suivi.

## 3. Les messages

Chaque message est un objet JSON, en texte. Il y a quatre sortes.

| Message | Exemple | L'éditeur |
|---------|---------|-----------|
| une ligne du journal | `{"instance":"order_1","transition":"grasp","event":"Completed","result":"Failed","date":1.614213573}` | l'ajoute au Journal, déplace les jetons |
| manifest | `{"type":"manifest","net":"Pick and place","places":[...],"transitions":[...],"unregistered":["retry"]}` | marque les transitions sans fonction |
| marquage | `{"type":"marking","instance":"German traffic lights","date":3.000,"tokens":[0,1,0,0]}` | démarre le rejeu de l'instance à partir de ces jetons ; le runtime ajoute `"colors":[{"Success":1},...]`, la couleur des jetons de chaque place (sans cela : `Success`) |
| entrées et sorties | `{"type":"io","inputs":{},"outputs":{"Green LED":false,"Red LED":false,"Yellow LED":true}}` | les affiche dans la fenêtre Program |

`event` vaut `Scheduled` (la transition a pris ses jetons d'entrée), `Started` (sa fonction tourne) ou `Completed` (elle a donné ses jetons de sortie, colorés par `result`). Une transition sans fonction n'a pas de `Started`.

Voici ce qu'un abonné Python reçoit en 12 lignes depuis `Debugger` :

```python
import zmq
socket = zmq.Context().socket(zmq.SUB)
socket.connect("tcp://localhost:5556")
socket.setsockopt_string(zmq.SUBSCRIBE, "")
for _ in range(12):
    print(socket.recv_string())
```

```text
{"instance":"order_1","transition":"detect","event":"Completed","result":"Success","date":0.7065038330000001}
{"instance":"order_1","transition":"grasp","event":"Scheduled","date":0.7065038330000001}
{"instance":"order_1","transition":"grasp","event":"Started","date":0.7065038330000001}
{"type":"manifest","net":"Pick and place","places":["Ready","Seen","Held","Done"],"transitions":["detect","grasp","place","retry"],"unregistered":["retry"]}
{"instance":"order_1","transition":"grasp","event":"Completed","result":"Failed","date":1.614213573}
{"instance":"order_1","transition":"retry","event":"Scheduled","date":1.614213573}
{"instance":"order_1","transition":"retry","event":"Completed","result":"Success","date":1.614213573}
{"instance":"order_1","transition":"grasp","event":"Scheduled","date":1.614213573}
{"instance":"order_1","transition":"grasp","event":"Started","date":1.614213573}
{"instance":"order_1","transition":"grasp","event":"Completed","result":"Success","date":2.522026006}
{"instance":"order_1","transition":"place","event":"Scheduled","date":2.522026006}
{"instance":"order_1","transition":"place","event":"Started","date":2.522026006}
```

L'abonné s'est connecté un instant avant le programme et a quand même manqué ses premiers messages : avec ZeroMQ, un abonné qui se connecte a besoin d'un peu de temps avant de recevoir (le « slow joiner »). C'est pourquoi le programme renvoie le **manifest** chaque seconde, et un **marquage** au début de chaque instance : un éditeur connecté tard connaît quand même le réseau et les jetons.

## 4. La fenêtre Journal

Le *Journal* (à côté de la *Console*) contient les événements d'une source : la dernière simulation de l'éditeur, un fichier journal ouvert, le chemin vers un interblocage trouvé par *Analysis > Properties* ([chapitre 4](04-Analysis.md)), ou un programme connecté.

![Le journal d'une simulation](pics/13-Journal.png)

*Le journal de la simulation de [`Timeout`](../../examples/Timed-Petri-Nets/Timeout/Timeout.json) : `Send` à 0, puis `Timeout` à 3 (son délai), la réponse arrivant à 5.*

| Partie | Rôle |
|--------|------|
| *Clear* | vider |
| *Open...* | lire un journal du runtime (`.jsonl`, `.json`, `.log` : lignes JSON) |
| *Save...* | l'écrire, le format selon l'extension : `.jsonl` (lignes JSON), `.csv`, `.xes` |
| *Follow* | garder le dernier événement en vue |
| Filtre | ne garder que les lignes contenant un texte |
| colonnes | `Date`, `Instance`, `Transition`, `k` (numéro de tir de cette transition : k = 2 est son deuxième tir), `Event`, `Result` (en rouge quand ce n'est pas `Success`) |
| *Replay* | afficher sur le réseau le marquage de l'instance choisie à une date : le curseur choisit la date, `<` et `>` vont à l'événement précédent et suivant, les flèches au premier et au dernier, *Play* laisse le temps avancer à la vitesse indiquée (`x1.0`). Les transitions dont la fonction tourne à cette date sont marquées `running` |
| un clic sur une ligne | va à sa date ; hors rejeu, marque sa transition |

Le rejeu reconstruit le marquage avec le runtime lui-même (`Runtime::restore()`), à partir des jetons initiaux du réseau : ce que vous voyez est ce que le programme avait.

*Debug > Open a journal...*, *Save the journal...* font comme les boutons ; *Debug > Clear the marks of the nodes* enlève les halos laissés par le journal, un programme ou l'analyse.

## 5. Publier depuis votre programme

Un programme utilisant le runtime publie avec `JournalPublisher` (`Runtime/JournalPublisher.hpp`). Seuls les programmes qui incluent cet en-tête lient libzmq : le runtime lui-même n'en dépend pas.

```c++
#include "Runtime/JournalPublisher.hpp"   // link with -lzmq

JournalPublisher publisher(net);           // binds tcp://*:5556
if (!publisher.error().empty()) { /* the port is taken ... */ }
Runtime::Options options;
options.journal = [&publisher](LogEntry const& e) { publisher.publish(e); };
Runtime runtime(net, executor, options);
runtime.on("grasp", grasp);
publisher.manifest(runtime);               // its transitions without function
auto order = runtime.instantiate("order_42");
publisher.marking(*order);                 // for an editor connected later
```

| `JournalPublisher` | Action |
|--------------------|--------|
| `JournalPublisher(net, endpoint)` | ouvre une socket PUB (`tcp://*:5556` par défaut) ; le réseau nomme les couleurs |
| `publish(entry)` | envoie une ligne du journal (n'importe quel thread) |
| `manifest(runtime)` | envoie le manifest, et encore chaque seconde avec les messages suivants |
| `marking(instance)` | envoie les jetons de l'instance à sa date |
| `send(text)` | envoie un message quelconque (un GRAFCET envoie ainsi ses entrées et sorties) |

Le programme généré par *File > Export > C++ functions for the runtime* fait tout cela lorsqu'il est compilé avec `-DTPNE_DEBUG -lzmq` ([chapitre 12](12-Code-Generation.md)).

Sans ZeroMQ, la même information peut aller dans un fichier : `toManifest(runtime)` écrit dans un fichier s'ouvre avec *Debug > Open a manifest...* (il marque les transitions sans fonction), et le journal enregistré en `.jsonl` s'ouvre avec le Journal.

## 6. Suivre le code GRAFCET généré

Le code C++ d'un GRAFCET ([chapitre 12](12-Code-Generation.md)) compilé avec `-DGRAFCET_DEBUG` publie à chaque cycle les tirs, les étapes actives (un marquage par graphe, chaque graphe étant une instance nommée d'après lui), et les entrées et sorties quand elles changent ; le manifest chaque seconde. Un test sur ordinateur avec le feu tricolore allemand :

```c++
#include "GermanTrafficLights.hpp"
#include <chrono>
#include <thread>

int main()
{
    german_traffic_lights::Grafcet grafcet;
    for (int cycle = 0; cycle < 600; ++cycle) // 60 s, a cycle every 100 ms
    {
        grafcet.cycle(0.1f);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
```

```sh
g++ -std=c++11 -DGRAFCET_DEBUG lights_debug.cpp -lzmq -o lights_debug
./lights_debug &
./build/TimedPetriNetEditor -c tcp://localhost:5556 examples/GRAFCET/GermanTrafficLights/GermanTrafficLights.json
```

Il publie :

```text
{"type":"manifest","net":"German traffic lights","places":["P0","P1","P2","P3"],"transitions":["T0","T1","T2","T3"],"unregistered":[]}
{"type":"marking","instance":"German traffic lights","date":1.100,"tokens":[1,0,0,0]}
{"type":"io","inputs":{},"outputs":{"Green LED":true,"Red LED":false,"Yellow LED":false}}
...
{"instance":"German traffic lights","transition":"T0","event":"Scheduled","date":3.000}
{"instance":"German traffic lights","transition":"T0","event":"Completed","result":"Success","date":3.000}
{"type":"marking","instance":"German traffic lights","date":3.000,"tokens":[0,1,0,0]}
{"type":"io","inputs":{},"outputs":{"Green LED":false,"Red LED":false,"Yellow LED":true}}
```

![L'éditeur qui suit le code généré](pics/13-GrafcetDebug.png)

*L'éditeur qui suit le code généré : l'étape 2 (rouge) est active dans le programme, la fenêtre Program affiche ses sorties (`out Red LED = true`), le Journal ses tirs. La fenêtre Outputs appartient à la simulation de l'éditeur, qui est arrêtée : elle reste éteinte.*

Les transitions d'un GRAFCET sont nommées par leurs clés (`T0`), car leurs légendes sont des réceptivités.

## 7. Depuis un Arduino

Une carte n'a pas de réseau. Le code généré, compilé pour Arduino avec `GRAFCET_DEBUG` défini, écrit les mêmes messages sur `Serial`, un objet JSON par ligne. `tools/serial-bridge.py` lit la ligne série et publie les lignes sur ZeroMQ :

```sh
pip install pyserial pyzmq
python3 tools/serial-bridge.py /dev/ttyACM0 [115200] [tcp://*:5556]
./build/TimedPetriNetEditor -c tcp://localhost:5556 my_grafcet.json
```

Le sketch démarre la liaison série à la même vitesse (`Serial.begin(115200);` dans `setup()`). Les lignes qui ne sont pas du JSON (vos propres `Serial.print`) sont affichées par le pont, pas envoyées à l'éditeur. Ailleurs (bus CAN, radio), définissez `GRAFCET_DEBUG_SEND(text)` pour envoyer chaque ligne vous-même, et publiez-les sur ZeroMQ de l'autre côté.

## 8. Utiliser les journaux

### 8.1 Tableurs et process mining

Le journal d'une instance dit quand chaque fonction a démarré et terminé, et comment. Enregistré en CSV, il s'ouvre dans un tableur :

```text
instance,transition,event,result,date
fast,request,Scheduled,,0
fast,request,Started,,0
fast,request,Completed,Success,0.0101056
fast,paid,Scheduled,,0.100624
```

Enregistré en **XES** (IEEE 1849), il s'ouvre dans les outils de **process mining** : [ProM](https://promtools.org), [PM4Py](https://pm4py.fit.fraunhofer.de), Disco. Chaque instance est une trace, chaque événement un événement avec `concept:name` (la transition), `lifecycle:transition` (`schedule`, `start`, `complete`) et `org:resource` (le résultat). Ces outils dessinent le processus qui s'est réellement produit à partir de milliers d'instances, mesurent les durées réelles des fonctions, et trouvent les chemins que vous n'attendiez pas.

```c++
#include "Runtime/Journal.hpp"
saveJournal(instance->log(), net.colors, "orders.xes");   // .jsonl, .csv, .xes
```

Les dates du runtime sont des secondes depuis le début de l'instance ; donnez à `saveJournal()` une origine (secondes depuis 1970) pour obtenir de vraies dates calendaires dans le fichier XES, sinon les dates sont en 1970. L'exemple [`MicroServices`](../../examples/Runtime/MicroServices/README.md) enregistre son journal dans les trois formats.

### 8.2 Après un plantage

Un journal enregistré ligne par ligne (`Options::journal` écrivant chaque ligne dans un fichier) suffit à reconstruire les instances d'un programme arrêté : `Runtime::restore()` le rejoue, puis rappelle ou échoue les fonctions qui tournaient ([chapitre 11](11-Runtime.md#51-restart-after-a-crash)). Le même fichier ouvert dans le Journal montre, avec *Replay*, où chaque instance en était quand le programme s'est arrêté.

## 9. Pièges

- **Rien n'arrive** : le programme ouvre `tcp://*:5556`, l'éditeur se connecte à `tcp://localhost:5556` (ou à l'adresse de la machine du programme). Un seul programme peut ouvrir un port : `publisher.error()` le signale.
- **Les jetons ne correspondent pas au réseau** : l'éditeur affiche le programme sur le réseau ouvert ; ouvrez le même réseau (la fenêtre Program avertit quand les nombres de transitions diffèrent).
- **Les premiers événements manquent** : le slow joiner de ZeroMQ (§3) ; appelez `publisher.marking()` au début de chaque instance.
- **Trop de messages** : un GRAFCET qui cycle chaque milliseconde ne publie que ce qui change, mais le runtime publie chaque événement : sous forte charge, gardez le journal dans un fichier et ouvrez-le plus tard.

## 10. Pour aller plus loin

- [11. Le runtime](11-Runtime.md) et [12. Génération de code](12-Code-Generation.md).
- [14. Référence de l'éditeur](14-Editor-Reference.md) : les fenêtres Journal et Program, l'option `-c`.
- [18. Télécommande](18-Remote-Control.md) : l'autre sens.
- W. van der Aalst, *Process Mining: Data Science in Action*, Springer, pour quoi faire avec des journaux XES.

---

Suite : les références, en commençant par [14. Référence de l'éditeur](14-Editor-Reference.md).
