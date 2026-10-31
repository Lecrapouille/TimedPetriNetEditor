# ⏱️ 6. Réseaux de Petri temporisés


[![fr](https://lecrapouille.github.io/icons/fr.png)](../fr/06-Timed-Petri-Nets.md) [![en](https://lecrapouille.github.io/icons/en.png)](../en/06-Timed-Petri-Nets.md)

**Ce que vous apprendrez**

- pourquoi on ajoute le temps à un réseau de Petri, et les trois endroits où la littérature le place ;
- le choix de cet éditeur : une **durée** sur les arcs sortant d'une transition, et un **délai** sur les transitions ;
- exactement comment un réseau temporisé s'exécute dans l'éditeur : tir au plus tôt, jetons en transit, événements, conflits dans le temps, circuits sans durée ;
- quatre schémas : la durée d'une tâche, le **timeout**, les **sources** et **puits** (entrées et sorties d'un système), la file qui grossit ;
- ce que l'éditeur affiche : l'animation, la vitesse, le *Journal*, le tracé des dates de tir.

**Prérequis** : [3. Réseaux de Petri](03-Petri-Nets.md) (marquage, règle de tir, conflit) et les priorités du [5. Réseaux de Petri étendus](05-Extended-Petri-Nets.md).

---

## 1. Pourquoi le temps ?

Un réseau de Petri tel que vu au chapitre 3 dit **quoi** peut arriver et dans quel ordre, jamais **quand**. Le tir d'une transition est instantané et personne ne sait combien de temps un jeton reste dans une place. Cela suffit pour trouver un interblocage, mais pas pour répondre aux questions d'un ingénieur :

- *Combien de produits par heure fabrique mon usine ?* (le **débit**)
- *Combien de temps un client attend-il ?* (le **temps de réponse**)
- *Que se passe-t-il si le serveur ne répond pas en 3 secondes ?* (un **timeout**)
- *Quelle machine acheter pour aller plus vite ?* (le **goulot d'étranglement**)

Pour y répondre, on donne au réseau une horloge : un événement arrive à une date, et certaines activités durent un certain temps.

## 2. Où placer le temps ? Bref historique

L'idée est venue dans les années 1970, en trois endroits différents du réseau :

| Temps sur ... | Signification | Introduit par |
|-------------|---------|---------------|
| les **transitions** : une durée de tir | la transition prend ses jetons d'entrée, travaille pendant `d`, puis donne ses jetons de sortie | C. Ramchandani, MIT, 1974 (*timed Petri nets*) |
| les **transitions** : un intervalle `[a, b]` | une fois sensibilisée, la transition tire entre `a` et `b` unités de temps plus tard | P. Merlin, 1974 (*time Petri nets*) |
| les **places** : un temps de séjour | un jeton qui arrive dans la place n'est disponible qu'`d` unités de temps plus tard | J. Sifakis, 1977 |

Plus tard, les durées sont devenues aléatoires (réseaux de Petri stochastiques, M. Molloy 1982 ; réseaux de Petri stochastiques généralisés, M. Ajmone Marsan, G. Balbo, G. Conte 1984, les réseaux de [GreatSPN](https://github.com/greatspn/SOURCES)). L'algèbre (max,+) des chapitres 7 et 8 utilise les temps de séjour sur les places.

Ces modèles ne sont pas équivalents en général, mais ils sont proches : une transition avec une durée de tir `d` revient à une transition instantanée *début*, une place *occupée* avec un temps de séjour `d`, et une transition instantanée *fin*.

```text
   firing duration on T             the same with a holding time on a place
   ( ) ──► [ T, d ] ──► ( )         ( ) ──► [start] ──► (busy, d) ──► [end] ──► ( )
```

## 3. Le choix de cet éditeur

L'éditeur place le temps en deux endroits.

### 3.1 La durée d'un arc

Un arc allant **d'une transition vers une place** a une **durée** `d` (un réel, 0 par défaut). Quand la transition tire, elle prend ses jetons d'entrée immédiatement, puis ses jetons de sortie **voyagent le long de l'arc** pendant `d` : ils atteignent la place `d` unités de temps après le tir. Entre-temps ils ne sont dans aucune place : ils sont *en transit* (un **vol**, dans le code). Les arcs entrant dans une transition n'ont pas de durée.

```text
   (●) ──► ▭ ──── 3 ────► ( )      T0 fires at t = 0: P0 is empty at once,
   P0      T0              P1       P1 gets its token at t = 3
```

C'est le temps de séjour de Sifakis, écrit sur l'arc qui alimente la place plutôt que dans la place : cela permet à deux arcs alimentant la même place d'avoir deux durées différentes. Quand tous les arcs de sortie d'une transition ont la même durée, c'est aussi la durée de tir de Ramchandani.

### 3.2 Le délai d'une transition

Une transition a un **délai** (0 par défaut) : elle ne tire qu'après avoir pu **tirer sans interruption** pendant ce délai. Si elle est désactivée avant la fin (une autre transition a pris son jeton), son délai repart de zéro la prochaine fois qu'elle est sensibilisée. C'est l'intervalle de Merlin `[d, d]` avec la politique de *mémoire d'activation*. Le délai est l'outil du **timeout** (§6.2).

### 3.3 Tir au plus tôt

Dans l'éditeur, un réseau temporisé tourne tout seul : **une transition tire dès qu'elle le peut** (sensibilisée, et son délai écoulé). Rien n'attend volontairement. C'est le comportement *au plus tôt*, celui d'un système qui ne perd jamais de temps ; le chapitre 7 montre qu'il donne les plus petites dates possibles.

## 4. Comment l'éditeur exécute un réseau temporisé

Le moteur de l'éditeur (`Engine`, dans `src/PetriNet/Engine.hpp`) est une **simulation à événements discrets** :

1. Le moteur conserve le marquage, les jetons en transit (chacun avec sa date d'arrivée) et, pour chaque transition avec un délai, la date depuis laquelle elle peut tirer.
2. À une date, il tire, **une à la fois**, les transitions qui peuvent tirer, jusqu'à ce qu'aucune ne le puisse. Chaque tir prend les jetons d'entrée et met les jetons de sortie en transit. Un jeton dont la durée est 0 arrive immédiatement et peut faire tirer une autre transition à la même date.
3. Puis l'horloge **saute au prochain événement** : la prochaine arrivée de jetons ou la prochaine fin de délai. Rien ne se passe entre les deux, donc les dates sont exactes : elles ne dépendent ni du taux d'images de l'écran ni de la vitesse de l'animation.
4. La simulation continue tant qu'il n'y a rien en transit et que rien ne peut tirer : le réseau est en **interblocage** (ou jusqu'à ce que vous l'arrêtiez).

Trois règles complètent la sémantique :

- **Conflits.** Quand plusieurs transitions peuvent tirer à une date et partagent un jeton, l'ordre compte. *Simulation > Conflicts* choisit la politique : *Random* (par défaut : un choix aléatoire, à partir d'une **graine** écrite dans la console, `Simulation has started (seed 4343662601744704737)!`, pour rejouer la même exécution), *Highest priority* (la priorité des transitions, puis la première déclarée) ou *First declared* (l'identifiant le plus petit).
- **Sources.** Une transition sans place d'entrée est toujours sensibilisée. Dans un réseau temporisé elle tire une fois, puis **tire à nouveau quand les jetons de son dernier tir sont arrivés** : sa période est la plus longue durée de ses arcs de sortie. Dans un réseau de Petri sans temps, elle tire quand vous cliquez dessus.
- **Circuits sans durée.** Un circuit de transitions dont tous les arcs ont une durée 0 pourrait tirer indéfiniment à la même date (un comportement *Zeno*). Le moteur s'arrête après 10 000 tirs à une même date et avertit : `Transitions keep firing at the date ...: a cycle without duration. The simulation stops.`

## 5. Votre premier réseau temporisé : le grille-pain

[`examples/Timed-Petri-Nets/Durations`](../../examples/Timed-Petri-Nets/Durations/README.md) grille une tranche de pain (3 unités de temps), la mange (2), et rachète du pain (1) :

```text
   (●) Bread ──► [Toast] ── 3 ──► ( ) Toasted ──► [Eat] ── 2 ──► ( ) Eaten
      ▲                                                                │
      └──────────────── 1 ──────── [Buy bread] ◄───────────────────────┘
```

Calculez les dates à la main. À `t = 0` seul `Toast` est sensibilisé : il tire, le jeton voyage 3. À `t = 3` il atteint `Toasted` : `Eat` tire, le jeton voyage 2. À `t = 5` `Buy bread` tire ; à `t = 6` le pain est de retour et `Toast` tire à nouveau. Le cycle dure 3 + 2 + 1 = 6. Le moteur donne les mêmes dates :

```text
t =  0.00  Toast              k = 1
t =  3.00  Eat                k = 1
t =  5.00  Buy bread          k = 1
t =  6.00  Toast              k = 2
t =  9.00  Eat                k = 2
t = 11.00  Buy bread          k = 2
t = 12.00  Toast              k = 3
```

`k` compte les tirs de chaque transition : `Toast` tire pour la k-ième fois à `6 (k - 1)`.

```sh
./build/linux-gcc-debug/bin/TimedPetriNetEditor examples/Timed-Petri-Nets/Durations/Durations.json
```

Appuyez sur **Space** : le réseau tourne tout seul ; le jeton se déplace le long de l'arc avec son nombre dans un disque vert. Le coin supérieur gauche affiche le temps simulé.

![The toaster at t = 1.5](pics/06-Durations-Running.png)

*À `t = 1.5`, le jeton est à mi-chemin entre `Toast` et `Toasted` : aucune place ne le détient.*

## 6. Quatre schémas

### 6.1 Une tâche qui dure : la durée

Mettez la durée d'une tâche sur l'arc qui quitte la transition qui la démarre. Une machine qui travaille 5 sur une pièce, c'est `[start] ── 5 ──► (done)`. Si la machine ne peut traiter qu'une pièce à la fois, donnez-lui une place *libre* avec un jeton, pris par `start` et rendu à la fin du travail : c'est la boucle des machines d'une usine (chapitre 7).

### 6.2 Le timeout

[`examples/Timed-Petri-Nets/Timeout`](../../examples/Timed-Petri-Nets/Timeout/README.md) envoie une requête et attend la réponse, mais pas indéfiniment :

```text
                    ┌── 5 ──► ( ) Reply ──┐
   (●) Request ──► [Send]                 ├──► [Receive] ──► ( ) Done
                    └── 0 ──► ( ) Waiting ┘
                                   │
                                   └──► [Timeout, delay 3] ──► ( ) Error
```

`Receive` et `Timeout` partagent le jeton de `Waiting` : ils sont en conflit. `Timeout` a un délai de 3 : il peut tirer 3 unités de temps après que `Waiting` a reçu son jeton, `Receive` dès que la réponse est là. Le premier l'emporte.

| Durée de `Send → Reply` | Ce qui se passe |
|----------------------------|--------------|
| 5 (comme dans le fichier) | `t = 0` Send, `t = 3` **Timeout** : la requête va vers `Error` ; la réponse arrive à 5 et reste seule dans `Reply` |
| 2 (double clic sur l'arc) | `t = 0` Send, `t = 2` **Receive** : la requête est `Done`, le timeout ne tire jamais |

![The timeout at t = 4](pics/06-Timeout-Error.png)

*À `t = 4` : le timeout a tiré (un jeton dans `Error`), la réponse est encore en transit.*

C'est le schéma de tout protocole, et du runtime du chapitre 11 (`examples/Runtime/MicroServices`).

### 6.3 Sources et puits : les entrées et sorties d'un système

Une transition sans place d'entrée est une **source** : elle apporte des jetons de l'extérieur (clients, pièces, messages). Une transition sans place de sortie est un **puits** : elle les envoie dehors. Dans le langage de la théorie du contrôle, les sources sont les **entrées** `u` du système et les puits ses **sorties** `y` (le chapitre 8 écrit les équations entre elles).

[`examples/Timed-Petri-Nets/InputsOutputs`](../../examples/Timed-Petri-Nets/InputsOutputs/README.md) a deux sources, `T0` (période 3, la durée de son arc) et `T3` (période 2), et deux puits, `T2` et `T5` :

![Inputs and outputs at t = 7](pics/06-InputsOutputs-Running.png)

*Les sources sont dessinées en orange pendant que leurs jetons sont en transit.*

### 6.4 La file qui grossit

Regardez la place `P2` du même réseau. `T3` y met un jeton toutes les 2 unités de temps. `T4` en prend un seulement quand `P5` a un jeton, et `P5` est alimentée par `T1`, qui ne peut pas aller plus vite que sa source `T0` : une fois toutes les 3 unités de temps. Le moteur donne (marquage `P0 ... P5` après chaque tir, jetons en transit non comptés) :

```text
t =  0.00  T0  k = 1   marking: 1 0 0 1 2 0
t =  0.00  T1  k = 1   marking: 0 0 0 1 1 0
t =  0.00  T3  k = 1   marking: 0 0 0 1 1 0
t =  0.00  T5  k = 1   marking: 0 0 0 0 1 0
t =  2.00  T3  k = 2   marking: 0 0 1 0 1 1
t =  2.00  T4  k = 1   marking: 0 0 0 0 1 0
...
t = 10.00  T3  k = 6   marking: 0 0 2 0 0 1
t = 10.00  T4  k = 4   marking: 0 0 1 0 0 0
t = 12.00  T0  k = 5   marking: 1 1 2 1 1 0
...
t = 12.00  T5  k = 5   marking: 0 0 2 0 0 0
```

À `t = 12`, `T3` a tiré 7 fois et `T4` 4 fois : `P2` contient 2 jetons et en gagne un toutes les 6 unités de temps. L'entrée `u1` est plus rapide que le système : la file grossit sans fin. Un réseau temporisé borné sans le temps peut donc devenir non borné avec le temps, et l'inverse : le temps élimine certains marquages (le chapitre 4 ignore le temps).

Le plus petit exemple de croissance est [`Unbounded`](../../examples/Timed-Petri-Nets/Unbounded/README.md) : `T0` rend son jeton à `P0` et en donne un de plus à `P1`, à chaque unité de temps :

```text
t =  0.00  T0  k = 1   marking: 0 0
t =  1.00  T0  k = 2   marking: 0 1
t =  2.00  T0  k = 3   marking: 0 2
t =  3.00  T0  k = 4   marking: 0 3
t =  4.00  T0  k = 5   marking: 0 4
```

## 7. Conflits dans le temps

[`examples/Timed-Petri-Nets/TimedConflict`](../../examples/Timed-Petri-Nets/TimedConflict/README.md) est le réseau de la première version de cette documentation :

![Timed conflict](../../examples/Timed-Petri-Nets/TimedConflict/TimedConflict.png)

`P1` a deux jetons et deux transitions de sortie, `T1` (5 pour atteindre `P2`) et `T2` (4). À `t = 0`, les deux peuvent tirer. Avec *First declared*, le moteur tire `T1` deux fois (son identifiant est le plus petit) ; avec *Random* et la graine 7, il tire `T2` deux fois ; et tout l'avenir change :

| | First declared | Random, seed 7 |
|-|----------------|----------------|
| à `t = 0` | `T0`, `T1`, `T1`, `T4` | `T4`, `T2`, `T0`, `T2` |
| premier tir de `T3` | `t = 5` | `t = 4` |
| tirs de `T3` avant `t = 12` | 4 (le dernier à 12) | 4 (le dernier à 12) |

La politique fait partie du modèle : quand l'ordre compte, décidez-le avec des priorités plutôt qu'en espérant la chance. [`Priorities`](../../examples/Extended-Petri-Nets/Priorities/README.md) (chapitre 5) est un carrefour où une ambulance (priorité 1) et une voiture (priorité 0) attendent. Avec *Simulation > Conflicts > Highest priority* :

```text
t =  0.00  Ambulance crosses  k = 1
t =  2.00  Car crosses        k = 1
t =  2.00  Ambulance back     k = 1
t =  4.00  Car back           k = 1
t =  7.00  Ambulance crosses  k = 2
t =  9.00  Car crosses        k = 2
```

Quand les deux attendent, l'ambulance passe en premier, à chaque fois.

## 8. Dans l'éditeur

| Pour ... | Faire |
|--------|----|
| temporiser un réseau | le type dans la barre d'outils, ou *Edit > Type of net > Timed Petri net* |
| donner une durée | double clic sur l'arc sortant d'une transition, ou le panneau *Arcs*, ou clic droit sur l'arc |
| donner un délai ou une priorité | le panneau *Transitions* de l'inspecteur |
| choisir comment les conflits sont résolus | *Simulation > Conflicts* (et sa graine pour *Random*), avant de démarrer |
| lancer, mettre en pause, changer la vitesse | **Space**, **P**, la vitesse `x0.1` à `x50` dans la barre d'outils : à `x1` une unité de temps vaut une seconde |
| ajouter ou retirer des jetons pendant l'exécution | touches **+** et **-** sur une place |
| lire ce qui s'est passé | le *Journal* (à côté de la console) |

Le **Journal** liste les tirs de la simulation : la date, le réseau, la transition, son numéro `k`, l'événement (`Scheduled` quand les jetons sont pris, `Completed` quand ils sont donnés) et le résultat. *Save* l'écrit en lignes JSON, CSV ou XES pour un tableur ou un outil de process mining ; *Replay* affiche le marquage à n'importe quelle date ([chapitre 13](13-Debugging.md)).

![The Journal of the toaster](pics/06-Journal.png)

Quand le réseau temporisé est aussi un graphe d'événements (chapitre 7), *Analysis > Firing dates* trace la date `y(k)` du k-ième tir de chaque transition. Avec *Subtract drift λ·k*, une transition qui tire régulièrement toutes les `λ` donne une courbe plate : ce qui reste, c'est le démarrage du système.

![Firing dates of Howard2](pics/06-Firing-Dates.png)

Le [runtime](11-Runtime.md) exécute les réseaux temporisés de la même façon dans vos programmes : ses arcs avec une durée donnent leurs jetons plus tard, ses délais attendent, ses priorités ordonnent les conflits (avec une différence : une source ne tire que lorsque le programme l'injecte).

## 9. À essayer

| Exemple | Faire | Observer |
|---------|----|---------|
| [Durations](../../examples/Timed-Petri-Nets/Durations/README.md) | Space, puis `x5` | un cycle de 6 ; *Analysis > Critical circuit* trouve λ = 6 |
| [Timeout](../../examples/Timed-Petri-Nets/Timeout/README.md) | Space ; puis mettre `Send → Reply` à 2 | Error, puis Done |
| [TimedConflict](../../examples/Timed-Petri-Nets/TimedConflict/README.md) | l'exécuter deux fois avec *Random*, puis avec *First declared* | des exécutions différentes, puis toujours la même |
| [InputsOutputs](../../examples/Timed-Petri-Nets/InputsOutputs/README.md) | Space, attendre | `P2` se remplit |
| [Unbounded](../../examples/Timed-Petri-Nets/Unbounded/README.md) | Space ; *Analysis > Properties* | `P1` croît ; le réseau est non borné |
| [Priorities](../../examples/Extended-Petri-Nets/Priorities/README.md) | *Conflicts > Highest priority*, Space | l'ambulance en premier |
| [Flowshop](../../examples/Flowshops/README.md) | Space | une vraie usine, le sujet du chapitre 7 |

## 10. Pièges

- **Une durée sur un arc entrant dans une transition est ignorée** : mettez-la sur l'arc qui quitte la transition.
- **Les durées exigent le type *Timed Petri net*** : dans un *Petri net*, les durées restent dans le fichier mais les jetons arrivent immédiatement et vous tirez par clics.
- **Une source avec une durée 0 tire indéfiniment** à la même date : le moteur l'arrête (l'avertissement Zeno). Donnez une durée à son arc.
- **L'analyse du chapitre 4 ignore le temps** : un interblocage trouvé là peut ne jamais arriver dans le temps, et un réseau borné peut déborder dans le temps (§6.4).
- **Random signifie aléatoire** : notez la graine de la console pour rejouer une exécution.

## 11. Pour aller plus loin

- J. Wang, *Timed Petri Nets: Theory and Application*, Kluwer, 1998.
- B. Bérard, F. Cassez, S. Haddad, D. Lime, O. H. Roux, *Comparison of the expressiveness of timed automata and time Petri nets*, FORMATS 2005.
- M. Ajmone Marsan, G. Balbo, G. Conte, S. Donatelli, G. Franceschinis, *Modelling with Generalized Stochastic Petri Nets*, Wiley, 1995 (gratuit en ligne).
- La [bibliographie](Bibliography.md).

---

Suivant : [7. Graphes d'événements temporisés](07-Timed-Event-Graphs.md) — quand aucun jeton n'a jamais le choix, les dates suivent des équations exactes.
