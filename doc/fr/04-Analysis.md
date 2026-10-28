# 4. 🔍 Analyse des réseaux de Petri


[![fr](https://lecrapouille.github.io/icons/fr.png)](../fr/04-Analysis.md) [![en](https://lecrapouille.github.io/icons/en.png)](../en/04-Analysis.md)

**Ce que vous apprendrez**

- le **graphe d’atteignabilité** d’un réseau, tracé à la main, puis par l’éditeur ;
- les questions qui comptent pour un système réel et leurs noms précis : **interblocage**, **transition morte**, **bornitude**, **vivacité** ;
- le **graphe de couverture** de Karp et Miller, qui reste fini quand une place croît sans limite ;
- la **matrice d’incidence**, l’**équation d’état**, et ce qu’elles prouvent sans simuler ;
- les **P-invariants** (ce qui ne change jamais) et **T-invariants** (ce qui revient), calculés à la main puis par l’éditeur ;
- comment obtenir tout cela dans l’éditeur (*Analysis > Properties*) et en terminal ou en intégration continue (`tpne-check`).

**Prérequis** : [chapitre 3](03-Petri-Nets.md) (places, transitions, marquage, règle de tir, matrices `Pre` et `Post`).

---

## 1. Pourquoi analyser ?

Simuler montre *un* comportement : celui sur lequel vous avez cliqué. Un système réel rencontre tous les autres. Deux robots qui partagent deux outils peuvent fonctionner des mois, puis bloquer l’usine le jour où ils prennent tous deux leur premier outil au même instant. L’analyse répond à des questions sur **tous** les comportements :

| Question | Nom de la propriété |
|----------|---------------------|
| Le système peut-il se bloquer, plus rien n’étant possible ? | **interblocage** (deadlock) |
| Une partie du modèle n’est-elle jamais utilisée ? | **transition morte** |
| Un tampon, une file, un compteur peut-il croître sans limite ? | **bornitude** |
| Chaque action peut-elle toujours se reproduire, quoi qu’il se soit passé avant ? | **vivacité** |
| Quelles quantités ne changent jamais (un outil, trois volets au plus) ? | **P-invariant** |
| Quelles séquences d’actions ramènent le système à son état ? | **T-invariant** |

Il y a deux familles de méthodes : **l’énumération** (construire tous les marquages atteignables, §2 à §4) et les **méthodes structurelles** (raisonner sur les matrices, sans les marquages, §5 à §7). L’éditeur utilise les deux. Le temps n’est pas pris en compte dans ce chapitre : l’analyse d’un réseau temporisé est celle de sa version non temporisée, qui peut atteindre *plus* de marquages que le temporisé (le rapport le dit).

## 2. Le graphe d’atteignabilité

Le **graphe d’atteignabilité** a un nœud par marquage atteignable et une arête `M → M'` étiquetée `t` quand tirer `t` dans `M` donne `M'`. C’est l’automate caché dans le réseau de Petri.

### 2.1 À la main : exclusion mutuelle

[MutualExclusion](../../examples/Petri-Nets/MutualExclusion/README.md) a cinq places, dans l’ordre `P0` Idle A, `P1` Critical A, `P2` Mutex, `P3` Idle B, `P4` Critical B, et `M0 = (1 0 1 1 0)`.

1. Dans `M0`, `Enter A` (`T0`) et `Enter B` (`T2`) sont autorisées. Tirer `T0` donne `(0 1 0 1 0)`, tirer `T2` donne `(1 0 0 0 1)`.
2. Dans `(0 1 0 1 0)` seule `Leave A` (`T1`) est autorisée : `Enter B` manque le jeton de Mutex. `T1` redonne `M0`.
3. Dans `(1 0 0 0 1)` seule `Leave B` (`T3`) est autorisée, et redonne `M0`.

Aucun marquage nouveau : le graphe est complet, avec 3 marquages.

```text
                 T0 (Enter A)                    T2 (Enter B)
   0 1 0 1 0  ◄──────────────  1 0 1 1 0  ──────────────►  1 0 0 0 1
              ──────────────►  (initial)  ◄──────────────
                 T1 (Leave A)                    T3 (Leave B)
```

| Depuis le marquage | Transition | Vers le marquage |
|--------------------|------------|------------------|
| `(1 0 1 1 0)` initial `M0` | `T0` (Enter A) | `(0 1 0 1 0)` |
| `(0 1 0 1 0)` | `T1` (Leave A) | `(1 0 1 1 0)` |
| `(1 0 1 1 0)` | `T2` (Enter B) | `(1 0 0 0 1)` |
| `(1 0 0 0 1)` | `T3` (Leave B) | `(1 0 1 1 0)` |

Lisez les propriétés sur le graphe : aucun marquage n’a les deux places critiques marquées, donc l’exclusion mutuelle tient ; chaque marquage a un successeur, donc pas d’interblocage ; depuis chaque marquage chaque transition peut tirer à nouveau, donc le réseau est vivant.

### 2.2 Par l’éditeur

Dans l’éditeur, *Analysis > Properties > Graph of the markings* dessine le graphe d’atteignabilité dans une fenêtre : une boîte par marquage, une flèche par tir étiquetée par sa transition, le marquage initial en gras, les interblocages en rouge. Un ressort place les boîtes (*Layout again* relance la disposition) ; glisser une boîte ou la vue, la molette zoome. Un réseau avec couleurs montre les couleurs des jetons (`3{Phi0:1 Phi1:1 Phi2:1}` : 3 jetons, un de chaque couleur). Les graphes de plus de 400 marquages ne sont pas dessinés, seulement exportés. Ici, les 4 marquages de [ColoredPhilosophers](../../examples/Extended-Petri-Nets/ColoredPhilosophers/README.md) :

![Graphe des marquages de ColoredPhilosophers](pics/04-Markings-ColoredPhilosophers.png)

*Export to dot...* dans cette fenêtre, ou `tpne-check --dot graph.dot net.json` en terminal, écrit le même graphe pour [Graphviz](https://graphviz.org) ; `dot -Tpng graph.dot -o graph.png` le dessine :

![Graphe des marquages de MutualExclusion](pics/04-Graph-MutualExclusion.png)

*Les mêmes 3 marquages, trouvés par `tpne-check` et dessinés par Graphviz. Chaque marquage liste les jetons de `P0 ... P4`.*

### 2.3 La limite : explosion d’états

Le graphe d’atteignabilité peut être énorme : `n` places indépendantes avec un jeton chacune donnent `2^n` marquages, et une place qui peut croître sans limite donne un graphe infini. L’éditeur s’arrête après un **nombre maximal de marquages** (100 000 pour `tpne-check`, `--max-states` pour le changer ; 20 000 par défaut dans l’éditeur, le champ *max markings*) et indique que le résultat est partiel. Le centre d’appels d’urgence a déjà 25 443 marquages atteignables.

## 3. Propriétés

Soit `R(M0)` l’ensemble des marquages atteignables depuis `M0`.

### 3.1 Interblocage

Un **interblocage** (deadlock) est un marquage atteignable où aucune transition n’est autorisée. L’éditeur explore le graphe en largeur : le premier interblocage trouvé est le **plus proche** de `M0`, et l’éditeur donne la **plus courte séquence de tirs** qui y mène.

[Deadlock](../../examples/Petri-Nets/Deadlock/README.md) est le classique : le robot A prend l’outil 1 puis l’outil 2, le robot B prend l’outil 2 puis l’outil 1.

![Graphe des marquages de Deadlock](pics/04-Graph-Deadlock.png)

*Le marquage en rouge, `0 1 0 0 0 0 1 0`, est l’interblocage : A tient l’outil 1, B tient l’outil 2.*

```text
$ build/tpne-check --allow-deadlock examples/Petri-Nets/Deadlock/Deadlock.json
examples/Petri-Nets/Deadlock/Deadlock.json:
Net: Deadlock (Petri net, 8 places, 6 transitions)
Reachable markings: 6
Deadlock: yes, marking (P0=0, P1=1, P2=0, P3=0, P4=0, P5=0, P6=1, P7=0) after firing T0 (A takes 1), T3 (B takes 2)
Dead transitions: none
Bounded: yes (1-bounded)
Live: no, not live: T0 (A takes 1), T1 (A takes 2), T2 (A releases), T3 (B takes 2), T4 (B takes 1), T5 (B releases)
P-invariants:
  P0 (A idle) + P1 (A has 1) + P2 (A works) = 1
  P1 (A has 1) + P2 (A works) + P3 (Tool 1) + P7 (B works) = 1
  P5 (B idle) + P6 (B has 2) + P7 (B works) = 1
  P2 (A works) + P4 (Tool 2) + P6 (B has 2) + P7 (B works) = 1
T-invariants:
  T0 (A takes 1) + T1 (A takes 2) + T2 (A releases)
  T3 (B takes 2) + T4 (B takes 1) + T5 (B releases)
```

Le remède est de modifier le réseau : les deux robots prennent les outils dans le même ordre, ou les deux en même temps (une transition avec les deux outils en places d’entrée, comme les [philosophes](../../examples/Petri-Nets/Philosophers/README.md)).

Parfois un interblocage est voulu : la fin d’un travail. [FirstNet](../../examples/Petri-Nets/FirstNet/README.md) se termine dans `Done`, le [pick and place](../../examples/Runtime/PickAndPlace/README.md) du runtime dans `Done`. `--allow-deadlock` dit à `tpne-check` de ne pas échouer sur eux.

### 3.2 Transitions mortes

Une transition est **morte** quand elle n’est autorisée dans aucun marquage atteignable : elle ne peut jamais tirer. C’est souvent une erreur de modélisation (jeton manquant, arc dans le mauvais sens). L’opposé, « tire au moins une fois dans un comportement », s’appelle **quasi-vivacité** (vivacité de niveau L1).

### 3.3 Bornitude

Une place est **k-bornée** quand elle ne contient jamais plus de `k` jetons dans un marquage atteignable ; un réseau est borné quand toutes ses places le sont. Un réseau **sûr** (safe) est 1-borné : chaque place est une condition booléenne (vraie quand marquée). L’éditeur donne la borne de chaque place (onglet *Bounds*) et la borne du réseau : `1-bounded` pour l’exemple Deadlock, `3-bounded` pour [SmarthomeSafety](../../examples/Petri-Nets/SmarthomeSafety/README.md) (la place `Remaining` contient 3 jetons). Un réseau borné a un graphe d’atteignabilité fini. Une place non bornée est un tampon sans limite : une mémoire qui débordera, une file qui ne se videra jamais.

### 3.4 Vivacité

Une transition est **vivante** (niveau L4) quand, depuis tout marquage atteignable, une séquence de tirs la rend à nouveau autorisée : quoi qu’il se soit passé, elle peut encore se produire. Un réseau est vivant quand toutes ses transitions le sont. La vivacité est plus forte que l’absence d’interblocage : un réseau peut tourner indéfiniment dans une boucle qui laisse certaines transitions de côté. L’éditeur la calcule sur le graphe d’atteignabilité complet : une transition est vivante quand elle étiquette une arête dans chaque composante fortement connexe terminale du graphe (un groupe de marquages que le système ne peut quitter). Quand le graphe est incomplet, la vivacité est « inconnue ».

| Exemple | Interblocage | Borné | Vivant |
|---------|:------------:|:-----:|:------:|
| [MutualExclusion](../../examples/Petri-Nets/MutualExclusion/README.md) | aucun | 1 | oui |
| [Philosophers](../../examples/Petri-Nets/Philosophers/README.md) | aucun | 1 | oui |
| [Deadlock](../../examples/Petri-Nets/Deadlock/README.md) | oui | 1 | non |
| [Conflict](../../examples/Petri-Nets/Conflict/README.md) | oui (après le choix) | 1 | non |
| [SmarthomeSafety](../../examples/Petri-Nets/SmarthomeSafety/README.md) | aucun | 3 | oui |
| [ProducerConsumer](../../examples/Petri-Nets/ProducerConsumer/README.md) | aucun trouvé | non (`Buffer`) | inconnue |

L’éditeur ne calcule pas la **réversibilité** (le marquage initial peut-il toujours être retrouvé ?) ni les états home ; pour un réseau vivant dont le graphe d’atteignabilité est une seule composante fortement connexe, comme MutualExclusion, cela tient.

## 4. Le graphe de couverture (Karp-Miller)

Quand une place croît sans limite, le graphe d’atteignabilité est infini. En 1969, Karp et Miller construisent un graphe fini qui répond encore à la question de bornitude : le **graphe de couverture**. L’idée : si l’exploration atteint un marquage `M'` **plus grand** qu’un marquage `M` sur son chemin (`M' ≥ M` place par place, et `M' ≠ M`), la séquence de tirs de `M` à `M'` peut être répétée autant de fois que l’on veut, et les places qui ont grandi peuvent contenir autant de jetons que l’on veut : elles sont remplacées par `ω` (oméga, « autant que l’on veut »).

[Unbounded](../../examples/Timed-Petri-Nets/Unbounded/README.md) a une transition `T0` qui rend le jeton de `P0` et en ajoute un à `P1` :

```text
M0 = (1 0)  ──T0──►  (1 1)  ≥ (1 0) and larger in P1:  P1 becomes ω
(1 ω)  ──T0──►  (1 ω)                                  (ω + 1 = ω)
```

Le graphe de couverture calculé par la bibliothèque a ces deux marquages :

```text
digraph "Unbounded" {
  M0 [label="1 0", penwidth=2];
  M1 [label="1 w"];
  M0 -> M1 [label="T0"];
  M1 -> M1 [label="T0"];
}
```

Une place est non bornée exactement quand un `ω` y apparaît. Pour le producteur et le consommateur, le graphe de couverture a 46 marquages et `ω` dans `Buffer` seulement. L’éditeur le construit tout seul quand le graphe d’atteignabilité est trop gros, pour dire si le réseau est borné :

```text
$ build/tpne-check examples/Petri-Nets/ProducerConsumer/ProducerConsumer.json
examples/Petri-Nets/ProducerConsumer/ProducerConsumer.json:
Net: Producer Consumer (Petri net, 5 places, 4 transitions)
Reachable markings: 100000 (stopped)
Deadlock: none found
Dead transitions: none
Bounded: no, unbounded: P2 (Buffer)
Live: unknown
P-invariants:
  P0 (Producer ready) + P1 (Idle) = 2
  P3 (Accepted) + P4 (Ready) = 2
T-invariants:
  T0 (Send) + T1 (Produce) + T2 (Accept) + T3 (Consume)
Note: Stopped after 100000 markings: no deadlock found is not a proof, the liveness is unknown.
```

Deux limites :

- Le graphe de couverture perd de l’information : `ω` signifie « non borné », pas « n’importe quel nombre ». Il prouve la bornitude et les transitions mortes, pas les interblocages ni la vivacité.
- Karp-Miller exige une règle de tir **monotone** : plus de jetons ne désactivent jamais une transition. Les arcs inhibiteurs, de remise à zéro et les capacités la cassent, et l’éditeur les refuse (« Karp-Miller does not apply to nets with inhibitor arcs ») ; voir le [chapitre 5](05-Extended-Petri-Nets.md#8-ce-que-coutent-les-extensions).

## 5. La matrice d’incidence et l’équation d’état

L’énumération croît avec le nombre de marquages. Les méthodes structurelles ne regardent que les matrices du réseau, quel que soit son marquage. La **matrice d’incidence** est

```text
C = Post − Pre        C(p, t) = jetons donnés à p par t − jetons pris à p par t
```

Pour MutualExclusion (places `P0` Idle A, `P1` Critical A, `P2` Mutex, `P3` Idle B, `P4` Critical B ; transitions `T0` Enter A, `T1` Leave A, `T2` Enter B, `T3` Leave B) :

```text
            T0  T1  T2  T3
       P0 | -1  +1   0   0 |
       P1 | +1  -1   0   0 |
  C =  P2 | -1  +1  -1  +1 |
       P3 |  0   0  -1  +1 |
       P4 |  0   0  +1  -1 |
```

La colonne `T0` se lit : Enter A prend un jeton à Idle A et à Mutex, en donne un à Critical A. Si une séquence de tirs `σ` tire chaque transition `t` `σ(t)` fois (le **vecteur de comptage de tirs**, ou vecteur de Parikh), chaque tir ajoute sa colonne, quel que soit l’ordre, donc

```text
M = M0 + C · σ             (l’équation d’état)
```

Vérifiez avec `σ = T0` (tirer Enter A une fois) : `M = (1,0,1,1,0) + (−1,+1,−1,0,0) = (0,1,0,1,0)`, le marquage du §2.1.

L’équation d’état donne une condition **nécessaire** : si `M` est atteignable, l’équation a une solution `σ ≥ 0` en entiers. Contraposée : **pas de solution prouve que `M` n’est pas atteignable**. Les deux processus peuvent-ils être en section critique, `M = (0,1,0,0,1)` ? Les lignes de `P0`, `P3` et `P2` demandent

```text
P0:  1 − σ(T0) + σ(T1) = 0             so  σ(T0) = σ(T1) + 1
P3:  1 − σ(T2) + σ(T3) = 0             so  σ(T2) = σ(T3) + 1
P2:  1 − σ(T0) + σ(T1) − σ(T2) + σ(T3) = 1 − 1 − 1 = −1 ≠ 0
```

Pas de solution : l’exclusion mutuelle est prouvée pour tout comportement, sans construire un seul marquage. La condition n’est pas suffisante : une solution ne prouve pas que `M` est atteignable, car l’équation ignore l’ordre des tirs. Pensez à une transition `t` avec une boucle sur une place vide `p` et une place de sortie `q` : sa colonne vaut 0 pour `p` et +1 pour `q`, donc `(0, 1) = (0, 0) + C · t` a une solution, mais `t` n’est jamais autorisée.

## 6. P-invariants

Un **P-invariant** (invariant de place) est un vecteur `y` de poids sur les places tel que

```text
y · C = 0
```

Multipliez l’équation d’état par `y` : `y · M = y · M0 + y · C · σ = y · M0`. La somme pondérée des jetons de ces places **ne change jamais**, quoi que tire. L’éditeur donne les P-invariants **semi-positifs minimaux** (aussi appelés P-semiflots) : `y ≥ 0`, avec un ensemble minimal de places (son **support**), calculés par l’algorithme de Farkas (1902), adapté aux réseaux de Petri par Martinez et Silva.

### 6.1 À la main

Dans la matrice `C` de MutualExclusion, cherchez des poids qui annulent chaque colonne.

- `y = (1, 1, 0, 0, 0)` (Idle A + Critical A) : la colonne `T0` donne `−1 + 1 = 0`, `T1` donne `+1 − 1 = 0`, `T2` et `T3` donnent 0. C’est un P-invariant : `Idle A + Critical A = 1` (le processus A est dans exactement un état).
- `y = (0, 0, 0, 1, 1)` : `Idle B + Critical B = 1`.
- `y = (0, 1, 1, 0, 1)` : la colonne `T0` donne `+1 − 1 = 0`, `T1` donne `−1 + 1 = 0`, `T2` donne `−1 + 1 = 0`, `T3` donne `+1 − 1 = 0`. Donc `Critical A + Mutex + Critical B = 1`.

Le dernier *est* l’exclusion mutuelle : comme les jetons ne sont jamais négatifs, `Critical A + Critical B ≤ 1`. L’éditeur trouve les mêmes trois :

```text
$ build/tpne-check examples/Petri-Nets/MutualExclusion/MutualExclusion.json
examples/Petri-Nets/MutualExclusion/MutualExclusion.json:
Net: Mutual exclusion (Petri net, 5 places, 4 transitions)
Reachable markings: 3
Deadlock: none
Dead transitions: none
Bounded: yes (1-bounded)
Live: yes
P-invariants:
  P0 (Idle A) + P1 (Critical A) = 1
  P3 (Idle B) + P4 (Critical B) = 1
  P1 (Critical A) + P2 (Mutex) + P4 (Critical B) = 1
T-invariants:
  T0 (Enter A) + T1 (Leave A)
  T2 (Enter B) + T3 (Leave B)
```

### 6.2 Ce que disent les P-invariants

- **Conservation** : chaque ressource des [philosophes](../../examples/Petri-Nets/Philosophers/README.md) est un invariant (`P1 (Phi1 Eating) + P3 (Phi0 Eating) + P6 (Fork) = 1` : la fourchette est tenue par Phi0, par Phi1, ou libre).
- **Règles de sécurité** : `P1 + P3 + P5 + P7 (the shutters down) + P8 (Remaining) = 3` dans [SmarthomeSafety](../../examples/Petri-Nets/SmarthomeSafety/README.md) : au plus trois volets sont baissés, une sortie reste toujours ouverte.
- **Poids** : les invariants peuvent avoir des poids. Dans [Weights](../../examples/Extended-Petri-Nets/Weights/README.md), `2 P1 (O2) + P2 (H2O) = 4` : chaque molécule d’eau contient une demi-molécule d’oxygène, les atomes sont conservés.
- **Bornitude** : une place couverte par un P-invariant à poids positif est bornée, quel que soit le marquage. `Buffer` de ProducerConsumer n’est dans aucun P-invariant : c’est la place non bornée.
- **Erreurs de modélisation** : une place qui devrait être conservée et n’est dans aucun invariant manque un arc. Une somme qui n’est pas le nombre attendu (2 au lieu de 1) est un jeton de trop dans `M0`. Dans ProducerConsumer, `Producer ready + Idle = 2` : il y a deux producteurs.

## 7. T-invariants

Un **T-invariant** est un vecteur `x ≥ 0` de comptages de tirs tel que

```text
C · x = 0
```

Par l’équation d’état, une séquence de tirs dont le vecteur de comptage est `x` redonne le marquage où elle a commencé : c’est un **cycle** du comportement. Dans MutualExclusion, `x = (1, 1, 0, 0)` (Enter A, Leave A) : la colonne `T0` plus la colonne `T1` vaut zéro. Les T-invariants sont les boucles d’un système ; une transition dans aucun T-invariant ne peut tirer infiniment souvent dans un réseau borné (elle ne peut tirer qu’un nombre fini de fois). [Conflict](../../examples/Petri-Nets/Conflict/README.md) n’en a aucun : rien ne revient après le choix.

Un T-invariant est un cycle candidat : l’ordre des tirs n’est pas donné, et certains ordres peuvent être impossibles depuis le marquage courant (dans [Deadlock](../../examples/Petri-Nets/Deadlock/README.md), `T0 + T1 + T2` est un cycle du robot A, mais pas après `B takes 2`).

**Seuls les arcs normaux** entrent dans `C` : avec des arcs inhibiteurs ou de remise à zéro l’éditeur écrit « Only the normal arcs are considered: the invariants may not hold with inhibitor or reset arcs » ([chapitre 5](05-Extended-Petri-Nets.md#8-ce-que-coutent-les-extensions)).

## 8. Dans l’éditeur

### 8.1 Analysis > Properties

*Analysis > Properties* met l’onglet **Summary** de la fenêtre *Analysis* au premier plan (la fenêtre est toujours là), pour tout type de réseau. Les propriétés ont quatre onglets : *Summary*, *Bounds*, *Invariants* et *Report* ; chacun commence par *Compute*, *max markings*, *Graph of the markings* et *Show on the net*. Pour un petit réseau (60 nœuds ou moins) le calcul se lance tout de suite, et après chaque modification : dessinez et regardez les propriétés suivre. Pour un plus gros réseau, appuyez sur **Compute** ; *max markings* fixe la limite d’exploration.

![Propriétés de l’exemple Deadlock](pics/04-Properties-Deadlock.png)

*Les propriétés des deux robots. Le résumé : 6 marquages (tous), un interblocage, aucune transition morte, 1-borné, non vivant. Avec *Show on the net*, les transitions non vivantes sont encadrées sur le réseau.*

| Onglet ou bouton | Ce qu’il montre |
|------------------|-----------------|
| Summary | marquages explorés (et si tous l’ont été), interblocage, transitions mortes, borné, vivant, en vert ou rouge ; puis le marquage d’interblocage, la plus courte séquence de tirs jusqu’à lui, et *Replay the path in the Journal* |
| Bounds | le nombre maximal de jetons de chaque place, ou non borné |
| Invariants | les P- et T-invariants ; un clic sur l’un marque ses nœuds sur le réseau |
| Report | le même texte que `tpne-check` (*Copy the report*) |
| *Show on the net* | marque les transitions mortes, les transitions non vivantes et les places non bornées |
| *Graph of the markings* | dessine le graphe d’atteignabilité dans une fenêtre (*Export to dot...* l’enregistre pour Graphviz) |

*Replay the path in the Journal* remplit le *Journal* des tirs qui mènent à l’interblocage : son curseur de relecture montre le marquage après chacun, pas à pas ([chapitre 13](13-Debugging.md)).

![Propriétés du producteur et du consommateur](pics/04-Properties-ProducerConsumer.png)

*Le producteur et le consommateur : l’exploration s’est arrêtée à 20 000 marquages, Karp-Miller dit que `Buffer` (P2) est non borné, marqué en rouge sur le réseau.*

![Propriétés de l’exclusion mutuelle](pics/04-Properties-MutualExclusion.png)

*Exclusion mutuelle : vivant, 1-borné, et les trois P-invariants du §6.1.*

### 8.2 tpne-check en terminal ou en CI

```text
tpne-check [--max-states N] [--allow-deadlock] [--dot graph.dot] net.json ...
```

| Option | Effet |
|--------|-------|
| `--max-states N` | arrêter l’exploration après `N` marquages (défaut 100000) |
| `--allow-deadlock` | un interblocage n’est pas une erreur (objectif d’un travail) |
| `--dot graph.dot` | écrire le graphe d’atteignabilité du dernier réseau pour Graphviz |

Il affiche le rapport de chaque réseau et renvoie **0** quand aucun réseau ne peut s’interbloquer, **1** quand un le peut (sauf `--allow-deadlock`), **2** pour une mauvaise utilisation ou un fichier illisible. Il lit les formats de l’éditeur (`.json`, `.pnml`, `.grml`, `.teg`). L’intégration continue de ce projet l’exécute sur les exemples, donc un changement qui introduit un interblocage fait échouer la build :

```sh
build/tpne-check examples/Petri-Nets/Philosophers/Philosophers.json examples/Petri-Nets/ProducerConsumer/ProducerConsumer.json
build/tpne-check --allow-deadlock examples/Runtime/PickAndPlace/PickAndPlace.json
```

Faites pareil dans votre projet : dessinez le réseau de votre contrôleur, commitez-le, vérifiez-le à chaque commit. Les mêmes fonctions sont dans la bibliothèque C++ (`properties()`, `pInvariants()`, `tInvariants()`, `reachabilityGraph()`, `coverabilityGraph()`, `report()` de `Analysis/Analysis.hpp`, [chapitre 16](16-Library.md)).

### 8.3 Ce que l’analyse ne prend pas en compte

Le rapport ajoute une note quand il ignore quelque chose :

- **durées** d’un réseau de Petri temporisé : « the timed net may reach fewer markings » ; un interblocage trouvé peut être impossible avec les vraies durées ;
- **règles GRAFCET** (réceptivités, tir simultané) : l’analyse d’un GRAFCET est celle de son réseau de Petri ;
- **couleurs** des jetons : elles sont prises en compte, comme le runtime les tire (« The colors of the tokens are considered »). Un marquage est les jetons de chaque couleur dans chaque place, et un tir donne une branche pour chaque couleur que son résultat peut avoir : les couleurs demandées par les arcs après la transition (`Success` si aucune). Une fonction du runtime qui renvoie une autre couleur n’est pas prise en compte. Si le réseau a trop de marquages, le graphe de couverture des bornes ne considère pas les couleurs (une note le dit).

Cela compte : l’analyse sans couleurs trouve des marquages que le runtime n’atteint jamais. Dans [ColoredPhilosophers](../../examples/Extended-Petri-Nets/ColoredPhilosophers/README.md), elle trouve 10 marquages, où `Phi0 eats` prend le jeton de `Phi1` ; avec les couleurs il n’y en a que 4 (au repos, ou un philosophe mange).

## 9. Essayez

| Exemple | Faire | Observer |
|---------|-------|----------|
| [MutualExclusion](../../examples/Petri-Nets/MutualExclusion/README.md) | ouvrir Properties, puis ajouter un jeton à Mutex | l’invariant devient `= 2` : l’exclusion est perdue |
| [Deadlock](../../examples/Petri-Nets/Deadlock/README.md) | *Replay the path in the Journal* ; puis faire prendre l’outil 1 au robot B en premier | l’interblocage disparaît |
| [Philosophers](../../examples/Petri-Nets/Philosophers/README.md) | cliquer sur chaque invariant | les nœuds de chaque ressource |
| [SmarthomeSafety](../../examples/Petri-Nets/SmarthomeSafety/README.md) | passer `Remaining` à 4 | la règle de sécurité disparaît, l’invariant dit `= 4` |
| [ProducerConsumer](../../examples/Petri-Nets/ProducerConsumer/README.md) | clic droit sur `Buffer` > *Max tokens* | le graphe devient fini et le réseau borné (Karp-Miller n’est plus nécessaire) |
| [EmergencyCalls](../../examples/Petri-Nets/EmergencyCalls/README.md) | *Compute* | 25 443 marquages ; la fin, tous les appels traités, est un interblocage |

## 10. Pièges

- **« Aucun interblocage trouvé » n’est pas une preuve** quand l’exploration s’est arrêtée : lisez le résumé (*all* ou *stopped*).
- **Non borné à cause d’une source** : une transition sans place d’entrée peut tirer indéfiniment, toute place en aval est non bornée sauf si quelque chose la limite (capacité, arc retour).
- **Analyser un réseau temporisé** : le réseau non temporisé peut s’interbloquer là où le temporisé ne le fait pas ; vérifiez le chemin d’interblocage contre les durées.
- **Invariants et extensions** : avec des arcs inhibiteurs ou de remise à zéro, un invariant des arcs normaux peut être faux.

## 11. Pour aller plus loin

- T. Murata, *Petri nets: properties, analysis and applications*, 1989.
- R. M. Karp, R. E. Miller, *Parallel program schemata*, 1969 (l’arbre de couverture).
- J. Martinez, M. Silva, *A simple and fast algorithm to obtain all invariants of a generalized Petri net*, 1982 (Farkas pour les réseaux de Petri).
- La [bibliographie](Bibliography.md).

---

Suivant : [5. Réseaux de Petri étendus](05-Extended-Petri-Nets.md) — poids, capacités, arcs inhibiteurs et de remise à zéro, priorités, couleurs.
