# 🔁 7. Graphes d'événements temporisés


[![fr](https://lecrapouille.github.io/icons/fr.png)](../fr/07-Timed-Event-Graphs.md) [![en](https://lecrapouille.github.io/icons/en.png)](../en/07-Timed-Event-Graphs.md)

**Ce que vous apprendrez**

- ce qu'est un **graphe d'événements**, et pourquoi il n'a pas de conflit ;
- ses propriétés : les jetons d'un circuit ne changent jamais, vivacité et bornitude se lisent sur les circuits ;
- le **dessin compact** de l'éditeur (places cachées sur les arcs) ;
- les équations **dateurs** et **compteurs**, calculées à la main, puis par l'éditeur ;
- la **forme canonique**, les **entrées** et **sorties** d'un système ;
- une usine (un **flowshop**) : machines, convoyeurs, la ressource qui la limite, et son **diagramme de Gantt** ;
- le format de fichier `.teg`.

**Prérequis** : [6. Réseaux de Petri temporisés](06-Timed-Petri-Nets.md).

---

## 1. Graphes d'événements

Un réseau de Petri est un **graphe d'événements** (aussi appelé **graphe marqué**) quand **chaque place a exactement une transition d'entrée et exactement une transition de sortie**. Les transitions peuvent avoir plusieurs arcs : elles attendent toujours plusieurs choses à la fois (synchronisation) et en démarrent plusieurs à la fois (parallélisme). Ce qui est interdit, c'est une place avec deux sorties (un choix) ou deux entrées (une fusion).

```text
   allowed (synchronization)          forbidden (choice)
   ( ) ──┐                                      ┌──► [T1]
         ├──► [T]                      ( ) ─────┤
   ( ) ──┘                                      └──► [T2]
```

La première conséquence est la plus importante : **il n'y a jamais de conflit**. Un jeton dans une place ne peut aller que vers une transition ; rien n'est aléatoire, rien ne dépend d'une politique. L'ordre des événements est fixé par la structure et les jetons ; avec des durées, leurs **dates** le sont aussi, et suivent des équations simples (§3).

Les graphes d'événements modélisent des systèmes qui répètent le même travail : lignes de production, assemblage, horaires ferroviaires, pipelines d'un processeur, boucles d'un programme temps réel. Ils ont été étudiés par F. Commoner, A. Holt, S. Even et A. Pnueli (*Marked directed graphs*, 1971).

### 1.1 Ce que disent les circuits

Dans un graphe d'événements, une transition sur un circuit prend un jeton de la place avant elle sur le circuit et en donne un à la place après elle sur le même circuit. Donc **le nombre de jetons de chaque circuit ne change jamais** : chaque circuit est un P-invariant ([chapitre 4](04-Analysis.md)). Trois résultats en découlent (Commoner et al., 1971) :

| Propriété | Se lit sur les circuits |
|----------|-----------------------|
| **vivant** (chaque transition peut toujours retirer) | chaque circuit contient au moins un jeton |
| **borné** | un graphe d'événements fortement connexe est borné : une place ne contient jamais plus de jetons que ceux des circuits qui la traversent |
| **sûr** (au plus un jeton par place), quand vivant | chaque place est sur un circuit qui contient exactement un jeton |

Un circuit sans jeton est une impasse : ses transitions s'attendent indéfiniment. L'éditeur le dit dans le diagramme de Gantt : `A circuit has no token: the event graph never fires`.

## 2. Le dessin compact

Comme une place d'un graphe d'événements a une transition avant et une après, on peut la dessiner comme un **arc entre deux transitions**, qui porte les jetons (un disque avec leur nombre, près de son extrémité) et la durée de la place cachée. C'est la manière des livres (max,+), et la manière dont l'éditeur dessine un **graphe d'événements temporisé**.

Le même réseau, [Howard2](../../examples/Timed-Event-Graphs/Howard2/README.md), en réseau de Petri temporisé :

![Howard2 as a Petri net](pics/EventGraph01.png)

et [Howard2](../../examples/Timed-Event-Graphs/Howard2/README.md) dessiné de façon compacte : l'arc `T0 → T2` est la place `P0`, son disque `2` ce sont les deux jetons de `P0`, et `5` sa durée :

![Howard2, compact](pics/07-Howard2-Compact.png)

Le disque près de l'extrémité d'un arc contient le nombre de jetons qui attendent dans la place cachée pour la prochaine transition : un seul disque quel que soit leur nombre, pour que les arcs restent lisibles. Pendant la simulation, les jetons en transit sont un disque vert avec leur nombre qui se déplace le long de l'arc pendant sa durée, puis rejoint le disque des jetons en attente.

Dans l'éditeur, le type *Timed event graph* (barre d'outils ou *Edit > Type of net*) est un réseau de Petri temporisé dessiné de façon compacte : un clic dans le vide ajoute une transition, un glisser entre deux transitions ajoute la place cachée, double clic sur un arc pour donner sa durée et ses jetons. Passer un réseau de Petri temporisé en *Timed event graph* vérifie que c'est un graphe d'événements et indique quelles places ne le sont pas.

## 3. Les dates suivent des équations

### 3.1 La forme dateur

Numérotez les tirs de chaque transition : `x_i(k)` est la **date** du **k-ième tir** de la transition `i` (un **dateur**). Regardez une place entre les transitions `j` et `i`, avec `m` jetons et la durée `t`. Le k-ième jeton pris par `i` dans cette place est :

- l'un des `m` jetons initiaux, pour `k ≤ m` (disponibles au temps 0) ;
- sinon le jeton donné par le `(k - m)`-ième tir de `j`, arrivé à `x_j(k - m) + t`.

Avec le tir au plus tôt du chapitre 6, `i` tire dès que toutes ses places d'entrée ont leur k-ième jeton : la date est le **plus grand** de ces arrivées.

```text
   x_i(k) = max over the input places of i ( x_j(k - m) + t )
```

C'est l'**équation dateur** : elle ne contient que `max` et `+`.

### 3.2 À la main sur Howard2

Les places de [Howard2](../../examples/Timed-Event-Graphs/Howard2/README.md) :

| Place | De → vers | Jetons `m` | Durée `t` |
|-------|-----------|:----------:|:------------:|
| P0 | T0 → T2 | 2 | 5 |
| P1 | T1 → T0 | 0 | 5 |
| P2 | T2 → T1 | 0 | 3 |
| P3 | T3 → T0 | 0 | 1 |
| P4 | T2 → T3 | 0 | 1 |

donnent les équations (*Analysis > Equations > Dater (max,+)* écrit la même chose) :

```text
T0(k) = max(1 + T3(k), 5 + T1(k))
T1(k) = max(3 + T2(k))
T2(k) = max(5 + T0(k - 2))
T3(k) = max(1 + T2(k))
```

Résolvez-les cycle par cycle. Pour `k = 1` et `k = 2`, `T2` prend les jetons initiaux de `P0`, disponibles à 0 : `T2(1) = T2(2) = 0`. Puis :

| k | T2(k) | T1(k) = 3 + T2(k) | T3(k) = 1 + T2(k) | T0(k) = max(1 + T3(k), 5 + T1(k)) |
|---|-------|-------------------|-------------------|-----------------------------------|
| 1 | 0 | 3 | 1 | max(2, 8) = 8 |
| 2 | 0 | 3 | 1 | max(2, 8) = 8 |
| 3 | 5 + T0(1) = 13 | 16 | 14 | max(15, 21) = 21 |
| 4 | 5 + T0(2) = 13 | 16 | 14 | max(15, 21) = 21 |

L'éditeur calcule ces **dates au plus tôt** pour le diagramme de Gantt ; pour 4 cycles il donne exactement :

```text
T0: 8 8 21 21
T1: 3 3 16 16
T2: 0 0 13 13
T3: 1 1 14 14
```

Deux tirs toutes les 13 unités de temps : 6,5 par tir. Le chapitre 8 montre comment trouver ce nombre sans calculer les dates.

![Dater equations of Howard2](pics/07-Howard2-Equations.png)

### 3.3 La forme compteur

Inversez la question : `x_i(t)` est maintenant le **nombre de tirs** de la transition `i` jusqu'à la date `t` (un **compteur**). Une place entre `j` et `i` avec `m` jetons et la durée `t_ij` permet à `i` de tirer au plus `m` fois de plus que `j` n'a tiré il y a `t_ij` unités de temps :

```text
   x_i(t) = min over the input places of i ( m + x_j(t - t_ij) )
```

*Analysis > Equations > Counter (min,+)* donne pour Howard2 :

```text
T0(t) = min(T3(t - 1), T1(t - 5))
T1(t) = min(T2(t - 3))
T2(t) = min(2 + T0(t - 5))
T3(t) = min(T2(t - 1))
```

Les rôles des jetons et des durées s'échangent : dans la forme dateur les jetons décalent l'indice `k` et les durées s'ajoutent ; dans la forme compteur les durées décalent le temps `t` et les jetons s'ajoutent. La forme dateur est en général préférée :

- un système réel a peu de jetons (2 trains, 3 machines) mais de longues durées (2 heures pour un système échantillonné à 1 Hz, c'est 7200 pas de délai) ; chaque pas de délai coûte une variable de mémoire dans un calcul ;
- avec la forme canonique (§4) chaque décalage de jeton est 0 ou 1.

## 4. La forme canonique

Un graphe d'événements est **canonique** quand :

- chaque place contient **au plus un jeton** ;
- les places sortant d'une transition d'entrée, et les places entrant dans une transition de sortie, ne contiennent **aucun jeton**.

Tout graphe d'événements a une forme canonique équivalente : une place avec `m` jetons devient `m` places en série avec un jeton chacune, séparées par de nouvelles transitions ; une place avec un jeton à côté d'une entrée ou d'une sortie reçoit une place vide avant ou après. Sur Howard2, la place `P0` (2 jetons, durée 5) devient :

```text
   before:  T0 ──(●●, 5)──► T2
   after:   T0 ──(●, 0)──► T4 ──(●, 5)──► T2
```

*Edit > To canonical form* le fait (ici il ajoute `T4`). En forme canonique, le décalage `k - m` de l'équation dateur est `k` (pas de jeton) ou `k - 1` (un jeton) : les équations deviennent le système linéaire du chapitre 8.

## 5. Entrées et sorties

Une transition sans place d'entrée est une **entrée** `u` du système (l'arrivée de matière première, d'un train, d'un client). Une transition sans place de sortie est une **sortie** `y` (un produit, un départ). Les autres forment l'**état** `x`. Aux dates au plus tôt, une entrée est toujours prête : elle tire à 0 pour tout `k`.

![JPQ as a Petri net](pics/07-JPQ-Petri.png)

[JPQ](../../examples/Timed-Event-Graphs/JPQ/README.md) a l'entrée `u(k)` et la sortie `y(k)`. Ses équations dateur (*Analysis > Equations*) :

```text
T0(k) = max(7 + T1(k - 1), 3 + T0(k - 1))
T1(k) = max(2 + T0(k - 1), 4 + T1(k - 1), 1 + T3(k))
T2(k) = max(3 + T0(k))
```

avec `T0 = Node 0`, `T1 = Node 1`, `T2 = y(k)`, `T3 = u(k)`. La sortie suit l'état : `y(k) = 3 + Node0(k)`. Les dates au plus tôt sur 4 cycles :

```text
Node 0: 0 8 12 17
Node 1: 1 5 10 14
y(k):   3 11 15 20
u(k):   0 0 0 0
```

Le chapitre 8 écrit ces équations avec des matrices.

## 6. Tutoriel : à quelle vitesse tourne mon usine ?

### 6.1 Le flowshop

Un **flowshop** est une usine qui produit des pièces. Chaque pièce visite les machines dans le même ordre. Un **job shop** laisse chaque pièce avoir son propre parcours ; il faut des choix, ce n'est donc pas un graphe d'événements. Le flowshop l'est.

Sur le graphe d'événements temporisé que construit l'éditeur, les ressources sont dessinées dans deux directions :

- une **machine** est un cycle **horizontal** (une ligne) ;
- un **convoyeur** est un cycle **vertical** (une colonne).

Les nombres dans le fichier indiquent combien de chaque ressource l'usine possède. Les exemples ci-dessous partent avec **une machine** de chaque type et **un convoyeur** de chaque type : un jeton sur chaque circuit.

[`examples/Flowshops/simple.flowshop`](../../examples/Flowshops/README.md) a 2 machines et 3 convoyeurs :

```text
machine\conveyor,C1,C2,C3
M1,2.0,3.0,1.0
M2,1.0,2.0,1.5
```

Le format est un tableau : les convoyeurs sur la première ligne, les machines sur la première colonne, et dans chaque case le temps de traitement du convoyeur sur la machine (`M1` travaille 2 sur `C1`, 3 sur `C2`, 1 sur `C3`). Une case vide signifie que le convoyeur ne visite pas cette machine (c'est le ε du chapitre 8). Une ligne qui commence par `#` est un commentaire. Une unité de chaque ressource est la valeur par défaut ; `C1*2` serait deux convoyeurs `C1`. Le format complet est au [chapitre 15, §4.3](15-File-Formats.md#43-flowshop), avec l'ancienne syntaxe des fichiers `.flowshop`, toujours lue.

*File > New > Flowshop...* remplit le même tableau dans une fenêtre et construit le réseau.

### 6.2 Le graphe d'événements temporisé d'un flowshop

Ouvrez-le : l'éditeur construit le graphe d'événements temporisé.

```sh
./build/TimedPetriNetEditor examples/Flowshops/simple.flowshop
```

![The flowshop](pics/07-Flowshop.png)

| Transition | Signification |
|------------|---------|
| `M1_C2` | une **opération** : la machine `M1` commence à travailler sur le convoyeur `C2` ; ses arcs de sortie portent le temps de traitement 3 |
| `M1`, `M2` | la machine est à nouveau libre : son jeton va vers sa première opération (`nm` jetons, ici 1) |
| `C1`, `C2`, `C3` | le convoyeur est à nouveau libre : son jeton va vers sa première machine (`np` jetons, ici 1) |
| `out_M1`, `out_C2` ... | la machine a fait sa dernière opération du cycle, le convoyeur a quitté sa dernière machine |

Chaque ligne est la boucle d'une machine : `M1 → M1_C1 → M1_C2 → M1_C3 → out_M1 → M1`, un jeton. La machine travaille sur un convoyeur à la fois, dans l'ordre des colonnes. Chaque colonne est la boucle d'un convoyeur : `C1 → M1_C1 → M2_C1 → out_C1 → C1`, un jeton. Le convoyeur est sur une machine à la fois, dans l'ordre des lignes. Une opération attend sa machine et son convoyeur : elle a deux places d'entrée, une synchronisation.

### 6.3 Temps de cycle et ressource critique

*Analysis > Critical circuit* (le chapitre 8 l'explique) répond :

```text
Cycle time λ: 6.00 time units.
```

et dessine en rouge le **circuit critique** : `M1 → M1_C1 → M1_C2 → M1_C3 → out_M1 → M1`. Le circuit rouge est horizontal : la machine `M1` est la ressource qui limite l'usine.

![The row of M1 is critical](pics/07-Flowshop-Critical.png)

À long terme l'usine termine un tour des trois convoyeurs toutes les 6 unités de temps. Pourquoi 6 ? Ce circuit est la boucle de `M1` : elle travaille 2 + 3 + 1 = 6 par tour, avec son unique jeton. `M2` travaille 1 + 2 + 1,5 = 4,5. La colonne du convoyeur `C2` fait 3 + 2 = 5, `C1` fait 2 + 1 = 3, `C3` fait 1 + 1,5 = 2,5. `M1` est la ressource la plus lente : pour aller plus vite, achetez une `M1` plus rapide, ou une seconde (`nm: 2 1`), pas une `M2` plus rapide.

### 6.4 L'ordonnancement : le diagramme de Gantt

*Analysis > Gantt* : une ligne par machine, une barre par opération, une couleur par convoyeur (la légende au-dessus du diagramme les nomme ; la ligne au-dessus rappelle quelles lignes sont dessinées). La barre `C2 k=2` est la 2e fois (cycle `k = 2`) que la machine travaille sur le convoyeur `C2` ; elle commence à la date au plus tôt de l'équation dateur (§3) et dure le temps de traitement. Le curseur *Cycles* affiche plus de cycles ; la souris sur une barre donne son début et sa fin.

![Gantt chart](pics/07-Flowshop-Gantt.png)

**Pourquoi n'y a-t-il que les lignes `M1` et `M2`, et pas une ligne par transition ?** Le diagramme de Gantt ne dessine que les **opérations** : les transitions suivies d'une durée (la plus longue durée de leurs arcs de sortie n'est pas 0). Alors :

- une transition nommée `<machine>_<conveyor>` (les opérations construites à partir d'un flowshop) est dessinée sur la ligne de `<machine>`, avec la couleur de `<conveyor>` : `M1_C1`, `M1_C2`, `M1_C3` vont toutes sur la ligne `M1` ;
- toute autre transition avec une durée a sa propre ligne, nommée d'après elle (dans un réseau qui n'est pas un flowshop, chaque transition temporisée est une ligne) ;
- les transitions sans durée ne prennent pas de temps, donc rien à dessiner : ici `M1`, `M2` (une machine est libre à nouveau), `C1`, `C2`, `C3` (un convoyeur est libre à nouveau) et les transitions `out_...` (fin d'un cycle). Elles comptent quand même dans les dates : elles passent les jetons des machines et des convoyeurs d'un cycle au suivant.

Les dates au plus tôt des 3 premiers cycles (l'éditeur, 4 cycles affichés) :

```text
M1_C1: 0 6 12 18       M2_C1: 2 8.5 14.5 20.5
M1_C2: 2 8 14 20       M2_C2: 5 11 17 23
M1_C3: 5 11 17 23      M2_C3: 7 13 19 25
```

`M1` ne s'arrête jamais : `M1_C1` à 0, `M1_C2` à 2 (après 2), `M1_C3` à 5 (après 3), `M1_C1` à nouveau à 6 (après 1) : un cycle de 6. `M2` attend `M1` : elle démarre `C1` à 2, quand `M1` l'a terminé ; dès le second cycle `M2_C1` commence à 8,5 et non à 8, parce que `M2` est occupée avec `C3` jusqu'à 7 + 1,5 = 8,5.

### 6.5 Les mêmes dates dans la simulation

Appuyez sur **Space** : la simulation du chapitre 6 exécute le même réseau. Le *Journal* liste les tirs : `M1_C1` à 0, `M1_C2` et `M2_C1` à 2 ... les dates du diagramme de Gantt, parce qu'un graphe d'événements temporisé n'a pas de choix et la simulation tire au plus tôt.

![Journal of the flowshop](pics/07-Flowshop-Journal.png)

### 6.6 D'abord un convoyeur, puis une machine

[`complex.flowshop`](../../examples/Flowshops/README.md) est le même type d'usine avec 8 machines et 6 convoyeurs, toujours une ressource de chaque. Certains convoyeurs sautent une machine (`-inf`).

*Critical circuit* donne `λ = 16.95` et peint une colonne en rouge, la boucle de `Conveyor3` :

![Conveyor3 is the critical resource](pics/07-Flowshop-Critical-Conveyor.png)

Le circuit est vertical. `Conveyor3` visite chaque machine, et avec son unique jeton la colonne dure

```text
0.95 + 2 + 2.2 + 2 + 3 + 4.3 + 1 + 1.5 = 16.95
```

Aucune ligne de machine n'est aussi longue (la plus longue est `Machine3`, 3.7 + 2.2 + 6.4 = 12.3). La ressource rare est un convoyeur, pas une machine.

Un convoyeur coûte moins qu'une machine, donc le prochain achat est un convoyeur. [`complex-conveyors.flowshop`](../../examples/Flowshops/complex-conveyors.flowshop) est la même table avec un second `Conveyor3` (`Conveyor3*2` dans sa première ligne). Le temps de cycle tombe à 15.12. Les circuits rouges ne sont plus cette colonne : ce sont les colonnes de `Conveyor2` et `Conveyor5`, et la ligne de `Machine3`.

![After one more Conveyor3](pics/07-Flowshop-More-Conveyors.png)

Continuez à ajouter des convoyeurs. [`complex-machine.flowshop`](../../examples/Flowshops/complex-machine.flowshop) en a deux de chaque (`Conveyor1*2` ... `Conveyor6*2`) et toujours une machine de chaque. Le temps de cycle est 12.30, et le seul circuit rouge est la ligne de `Machine3` (3.7 + 2.2 + 6.4 = 12.3, un jeton). Le circuit est horizontal : la machine limite maintenant l'usine. Un convoyeur de plus n'aiderait pas ; une seconde `Machine3` le ferait.

![Machine3 is the critical resource](pics/07-Flowshop-Critical-Machine.png)

L'éditeur ne choisit pas ces nombres. Il n'y a pas de réglage automatique d'un flowshop idéal : vous modifiez `nm` et `np`, rouvrez le fichier, et lisez le circuit critique.

## 7. Le format de fichier `.teg`

Les graphes d'événements temporisés peuvent aussi être enregistrés dans un petit format texte, hérité des programmes de l'[algorithme de Howard](http://www.cmap.polytechnique.fr/~gaubert/HOWARD2.html) (*File > Export > Timed event graph*, extension `.teg`) :

```text
TimedEventGraph <number of transitions> <number of arcs>
<from transition> <to transition>: <duration> <tokens>
...
```

Une ligne par place cachée, de la transition avant à la transition après. [SemiHoward.teg](../../examples/Timed-Event-Graphs/SemiHoward/README.md) :

```text
TimedEventGraph 3 5

0 1:   0   1
1 0:   1   0
2 0:   0   0
2 1:   1   0
2 2:   2   1
```

Le format n'a ni noms ni positions : l'éditeur place les transitions sur un cercle. [SemiNetherlands.teg](../../examples/Timed-Event-Graphs/SemiNetherlands/README.md) est plus grand (8 transitions, 12 places).

## 8. Référence : la fenêtre Analysis des graphes d'événements

La fenêtre a une rangée d'onglets : *Summary*, *Bounds*, *Invariants* et *Report* pour tout réseau (la théorie des réseaux de Petri du [chapitre 4](04-Analysis.md)), puis, pour un graphe d'événements temporisé, les onglets ci-dessous ; ils apparaissent tout seuls. Quand le réseau temporisé n'est pas un graphe d'événements, un onglet *(max,+)* dit pourquoi (une place avec deux arcs d'entrée, un poids ...) et *Show the faulty arcs* dessine les arcs à modifier.

| Onglet | Affiche | Chapitre |
|-----|-------|---------|
| Critical circuit | λ, les arcs du circuit critique (rouge sur le réseau), le biais | [8](08-Max-Plus-Algebra.md) |
| Gantt | dates au plus tôt des premiers cycles, par machine | §6.4 |
| Linear system | les matrices `D`, `A`, `B`, `C` | [8](08-Max-Plus-Algebra.md) |
| Equations | formes dateur (max,+) et compteur (min,+), avec identifiants ou légendes | §3 |
| Adjacency | durées et jetons sous forme de matrices | [8](08-Max-Plus-Algebra.md) |
| Firing dates | `y(k)` de chaque transition pendant la simulation | [6](06-Timed-Petri-Nets.md) |
| Properties | interblocages, bornes, vivacité, invariants | [4](04-Analysis.md) |

## 9. À essayer

| Exemple | Faire | Observer |
|---------|----|---------|
| [Howard2](../../examples/Timed-Event-Graphs/Howard2/README.md) | *Analysis > Gantt*, *Equations* | les dates du §3.2 |
| [Howard1](../../examples/Timed-Event-Graphs/Howard1/README.md) | *Analysis > Equations* | chaque décalage de jeton est `k - 2` |
| [JPQ](../../examples/Timed-Event-Graphs/JPQ/README.md) | *Equations* ; passer le type en *Timed event graph* | entrées et sorties ; le dessin compact |
| [EventGraph](../../examples/Timed-Event-Graphs/EventGraph/README.md) | *Equations* | l'exemple du livre de Baccelli, Cohen, Olsder, Quadrat |
| [Flowshop](../../examples/Flowshops/README.md) | *Critical circuit* sur `complex.flowshop`, puis `complex-conveyors.flowshop`, puis `complex-machine.flowshop` | le circuit rouge passe d'une colonne (un convoyeur) à une ligne (une machine) |
| [SemiHoward](../../examples/Timed-Event-Graphs/SemiHoward/README.md) | ouvrir le `.teg` | un réseau sans noms |

## 10. Pièges

- **Un choix casse tout** : une place avec deux transitions de sortie et les onglets (max,+) disparaissent (la fenêtre dit pourquoi). Utilisez un réseau de Petri temporisé (chapitre 6) et simulez.
- **Un circuit sans jeton ne tire jamais** : le diagramme de Gantt le dit.
- **Les dates au plus tôt supposent les entrées toujours prêtes** (elles tirent à 0) : les vraies entrées arrivent plus tard ; les dates sont alors plus grandes.

## 11. Pour aller plus loin

- F. Commoner, A. W. Holt, S. Even, A. Pnueli, *Marked directed graphs*, Journal of Computer and System Sciences, 1971.
- F. Baccelli, G. Cohen, G. J. Olsder, J.-P. Quadrat, *Synchronization and Linearity*, Wiley, 1992, chapitre 2 ([gratuit en ligne](https://www.rocq.inria.fr/metalau/cohen/SED/book-online.html)).
- La [bibliographie](Bibliography.md).

---

Suivant : [8. L'algèbre (max,+)](08-Max-Plus-Algebra.md) — les équations de ce chapitre deviennent linéaires, et le temps de cycle une valeur propre.
