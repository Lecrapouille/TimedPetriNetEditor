# ➕ 8. L'algèbre (max,+)


[![fr](https://lecrapouille.github.io/icons/fr.png)](../fr/08-Max-Plus-Algebra.md) [![en](https://lecrapouille.github.io/icons/en.png)](../en/08-Max-Plus-Algebra.md)

**Ce que vous apprendrez**

- l'**algèbre (max,+)** : un monde où « plus » est `max` et « fois » est `+` ;
- ses matrices, et des produits calculés à la main ;
- comment un graphe d'événements temporisé devient un **système linéaire** `X(k) = D X(k) ⊕ A X(k-1) ⊕ B U(k)`, `Y(k) = C X(k)` ;
- l'**étoile de Kleene** et la forme explicite du système ;
- le **temps de cycle** comme **valeur propre**, le régime périodique, le **circuit critique** ;
- **l'algorithme de Howard**, pas à pas sur un exemple ;
- ce que calcule l'éditeur, et comment continuer en Julia.

**Prérequis** : [7. Graphes d'événements temporisés](07-Timed-Event-Graphs.md) (équations dateur, forme canonique, entrées et sorties).

---

## 1. Bref historique

Au début des années 1980, à l'INRIA, G. Cohen, D. Dubois, J.-P. Quadrat et M. Viot ont remarqué que les dates d'un graphe d'événements temporisé obéissent à des équations **linéaires** si l'on remplace le `+` habituel par `max` et le `×` habituel par `+` (*A linear system theoretic view of discrete event processes and its use for performance evaluation in manufacturing*, IEEE TAC, 1985). La théorie des systèmes linéaires s'appliquait alors aux usines, réseaux ferroviaires et systèmes informatiques : valeurs propres, fonctions de transfert, rétroaction. L'ouvrage *Synchronization and Linearity* (F. Baccelli, G. Cohen, G. J. Olsder, J.-P. Quadrat, Wiley, 1992) rassemble la théorie ; il est [gratuit en ligne](https://www.rocq.inria.fr/metalau/cohen/SED/book-online.html).

La boîte à outils (max,+) de [ScicosLab](http://www.scicoslab.org/) (le fork INRIA de Scilab) calculait ces objets, avec **l'algorithme de Howard** pour la valeur propre (J. Cochet-Terrasson, G. Cohen, S. Gaubert, M. McGettrick, J.-P. Quadrat, *Numerical computation of spectral elements in max-plus algebra*, 1998). ScicosLab n'est plus développé ; la boîte à outils vit en Julia sous [MaxPlus.jl](https://github.com/Lecrapouille/MaxPlus.jl), et cet éditeur en est la moitié graphique : il dessine le réseau et écrit les matrices, et il exécute le même algorithme de Howard (le code C de la boîte à outils, `src/MaxPlus/Howard.c`).

## 2. L'algèbre

### 2.1 Deux opérations

Sur les réels complétés par `-∞` :

| (max,+) | Signification | Exemple |
|---------|-------|---------|
| `a ⊕ b` | `max(a, b)` | `3 ⊕ 5 = 5` |
| `a ⊗ b` (ou `a b`) | `a + b` | `3 ⊗ 5 = 8` |
| `ε` (zéro) | `-∞` | `ε ⊕ a = a`, `ε ⊗ a = ε` |
| `e` (un) | `0` | `e ⊗ a = a` |
| `a^n` | `n × a` | `3^2 = 6` |

Les règles de l'algèbre scolaire restent valables : `⊕` et `⊗` sont associatifs et commutatifs, `⊗` est distributif par rapport à `⊕` : `a ⊗ (b ⊕ c) = (a ⊗ b) ⊕ (a ⊗ c)`, c'est-à-dire `a + max(b, c) = max(a + b, a + c)`. Deux règles sont nouvelles :

- `⊕` est **idempotent** : `a ⊕ a = a` (`max(a, a) = a`) ;
- donc `⊕` n'a **pas d'inverse** : il n'existe pas de `x` avec `5 ⊕ x = ε`. On ne peut pas « soustraire » en (max,+) ; les équations se résolvent autrement (résiduation, étoile de Kleene du §4).

Une telle structure est un **semi-anneau idempotent**, ou **dioïde**. L'algèbre (min,+) est son jumeau : `⊕ = min`, `ε = +∞` ; c'est l'algèbre des compteurs (chapitre 7, §3.3) et des plus courts chemins.

### 2.2 L'équation dateur est linéaire

L'équation dateur du chapitre 7, `x_i(k) = max_j (x_j(k - m_ij) + t_ij)`, devient :

```text
x_i(k) = ⊕_j  t_ij ⊗ x_j(k - m_ij)
```

une somme de produits par des constantes : une équation **linéaire**. Sur Howard2 l'éditeur écrit (*Analysis > Equations*, la forme dateur en notation (max,+)) :

```text
T0(k) = 1 T3(k) ⨁ 5 T1(k)
T1(k) = 3 T2(k)
T2(k) = 5 T0(k - 2)
T3(k) = 1 T2(k)
```

## 3. Matrices

### 3.1 Produits

Les matrices suivent les règles habituelles avec les nouvelles opérations :

```text
(A ⊕ B)_ij = max(A_ij, B_ij)        (A ⊗ B)_ij = max over l of (A_il + B_lj)
```

L'identité `I` a `e = 0` sur sa diagonale et `ε` ailleurs. Une matrice de l'éditeur écrit `ε` comme `.` en texte ou `ε` dans les tableaux.

À la main, avec la matrice `A` de [JPQ](../../examples/Timed-Event-Graphs/JPQ/README.md) et le vecteur `x = (0, 1)` :

```text
    | 3  7 |       | 0 |     | max(3 + 0, 7 + 1) |   | 8 |
A = | 2  4 |,  x = | 1 |,    | max(2 + 0, 4 + 1) | = | 5 |   = A ⊗ x
```

### 3.2 Une matrice est un graphe

Une matrice carrée `A` est un graphe : un arc `j → i` de poids `A_ij` pour chaque entrée qui n'est pas `ε` (l'ordre « vers, de » fait que `A ⊗ x` donne, en `x_i`, les contributions des prédécesseurs de `i`). Alors :

- `(A^n)_ij` est le **chemin le plus lourd** de `n` arcs de `j` à `i` ;
- `A^+ = A ⊕ A^2 ⊕ A^3 ⊕ ...` est le chemin le plus lourd de n'importe quelle longueur ;
- un circuit du graphe a un **poids moyen** : son poids divisé par sa longueur.

Ce graphe est le **graphe de précédence** : pour un graphe d'événements temporisé c'est le dessin compact du chapitre 7, lu à l'envers.

### 3.3 Les matrices d'adjacence de l'éditeur

*Analysis > Adjacency* donne un graphe d'événements temporisé sous deux matrices : les **durées** et les **jetons** des arcs entre transitions, l'entrée `(i, j)` étant l'arc de `T_j` vers `T_i`. Pour Howard2 :

```text
             Tokens                 Durations
       T0 T1 T2 T3            T0 T1 T2 T3
  T0 |  .  .  2  . |     T0 |  .  .  5  . |
  T1 |  0  .  .  . |     T1 |  5  .  .  . |
  T2 |  .  0  .  0 |     T2 |  .  3  .  1 |
  T3 |  0  .  .  . |     T3 |  1  .  .  . |
```

`(T0, T2)` : la place de `T2` vers `T0`... lisez plutôt « `T0` reçoit de `T2` » ? Non : l'éditeur écrit, sous l'onglet, *Entry (i, j): the arc from transition j to transition i*. Donc `(T2, T0) = 2 jetons, 5` : de `T0` à `T2`, la place `P0`.

![Adjacency matrices of Howard2](pics/08-Howard2-Adjacency.png)

## 4. Le système linéaire d'un graphe d'événements temporisé

### 4.1 La forme implicite

Prenez la **forme canonique** du réseau (chapitre 7, §4) : chaque décalage de jeton est alors 0 ou 1. Classez les transitions en entrées `U` (sans place d'entrée), sorties `Y` (sans place de sortie) et états `X` (les autres). Les équations dateur deviennent :

```text
X(k) = D X(k) ⊕ A X(k-1) ⊕ B U(k)
Y(k) = C X(k)
```

| Matrice | Contient les arcs ... |
|--------|--------------------|
| `D` | entre états, via une place **sans** jeton (même cycle `k`) |
| `A` | entre états, via une place **avec un** jeton (cycle précédent `k - 1`) |
| `B` | des entrées vers les états |
| `C` | des états vers les sorties |

Le système est **implicite** : `X(k)` est des deux côtés.

*Analysis > Linear system* affiche les quatre matrices (l'éditeur calcule la forme canonique pour vous), chacune avec un bouton *Copy as Julia*. Pour [JPQ](../../examples/Timed-Event-Graphs/JPQ/README.md) (états `x1 = Node 0`, `x2 = Node 1`, entrée `u(k)`, sortie `y(k)`) :

```text
    | .  . |       | 3  7 |       | . |
D = | .  . |,  A = | 2  4 |,  B = | 1 |,  C = | 3  . |
```

![The linear system of JPQ](pics/08-JPQ-Linear-System.png)

`D` est vide : toute place entre deux états contient un jeton. Vérifiez le système contre les dates au plus tôt du chapitre 7 (`Node 0: 0 8 12 17`, `Node 1: 1 5 10 14`, l'entrée toujours prête : `u(k) = 0`) :

```text
X(2) = A X(1) ⊕ B u(2) = | max(3 + 0, 7 + 1)    |   | 8 |
                         | max(2 + 0, 4 + 1, 1) | = | 5 |

X(3) = A X(2) ⊕ B u(3) = | max(3 + 8, 7 + 5)    |   | 12 |
                         | max(2 + 8, 4 + 5, 1) | = | 10 |

Y(k) = C X(k) = 3 + x1(k):  3, 11, 15 ...
```

(`X(1) = (0, 1)` vient des jetons initiaux, disponibles au temps 0.)

### 4.2 L'étoile de Kleene et la forme explicite

Pour Howard2, la forme canonique ajoute la transition `T4` (la place `P0` à deux jetons devient deux places à un jeton, chapitre 7 §4), et *Analysis > Linear system* donne deux matrices 5 × 5 (lignes et colonnes `T0 ... T4`, pas d'entrée, pas de sortie) :

```text
    | .  5  .  1  . |        | .  .  .  .  . |
    | .  .  3  .  . |        | .  .  .  .  . |
D = | .  .  .  .  . |,   A = | .  .  .  .  5 |
    | .  .  1  .  . |        | .  .  .  .  . |
    | .  .  .  .  . |        | 0  .  .  .  . |
```

Pour enlever `X(k)` du côté droit, remplacez-le encore et encore :

```text
X(k) = D X(k) ⊕ R      with R = A X(k-1) ⊕ B U(k)
     = D (D X(k) ⊕ R) ⊕ R = D² X(k) ⊕ D R ⊕ R = ...
     = (I ⊕ D ⊕ D² ⊕ D³ ⊕ ...) R = D* R
```

`D* = I ⊕ D ⊕ D² ⊕ ...` est l'**étoile de Kleene** de `D` : les chemins les plus lourds de n'importe quelle longueur via des places sans jeton. Elle est finie quand le graphe de `D` n'a pas de circuit, c'est-à-dire quand **chaque circuit du réseau contient un jeton**, la condition de vivacité du chapitre 7. Alors `D^n = ε` à partir de `n` = le nombre d'états, et :

```text
X(k) = D* A X(k-1) ⊕ D* B U(k)        (explicit form)
```

L'éditeur affiche `D` et `A` ; il ne calcule pas `D*`. À la main pour Howard2 : les chemins de `D` sont `T1 → T0` (5), `T3 → T0` (1), `T2 → T1` (3), `T2 → T3` (1) ; de longueur 2, `T2 → T1 → T0` (8) et `T2 → T3 → T0` (2), donc `(D²)_02 = max(8, 2) = 8` ; pas de chemin de longueur 3 : `D³ = ε`.

```text
              | 0  5  8  1  . |                 | .  .  .  .  13 |
              | .  0  3  .  . |                 | .  .  .  .   8 |
D* = I⊕D⊕D² = | .  .  0  .  . |,   M = D* A =   | .  .  .  .   5 |
              | .  .  1  0  . |                 | .  .  .  .   6 |
              | .  .  .  .  0 |                 | 0  .  .  .   . |
```

Donc `T0(k) = 13 + T4(k-1)` et `T4(k) = T0(k-1)` : `T0(k) = 13 + T0(k-2)`. Les dates au plus tôt du chapitre 7 (`T0: 8 8 21 21 34 34`) croissent de 13 toutes les deux cycles.

## 5. Le temps de cycle est une valeur propre

### 5.1 Valeur propre et vecteur propre

Sans entrées, le système est `X(k) = M X(k-1)`. Un nombre `λ` et un vecteur `v` (pas tous `ε`) tels que

```text
M ⊗ v = λ ⊗ v        (that is  max_j (M_ij + v_j) = λ + v_i  for each i)
```

sont une **valeur propre** et un **vecteur propre** de `M`. Si le réseau démarre à `X(0) = v`, alors `X(k) = k λ + v` : chaque transition tire toutes les `λ` unités de temps. `λ` est le **temps de cycle** (son inverse est le **débit**).

Le théorème fondamental de l'algèbre (max,+) dit, pour une matrice **irréductible** `M` (son graphe est fortement connexe) :

1. `M` a **une** valeur propre, le **plus grand poids moyen de ses circuits** : `λ = max over the circuits c of  weight(c) / length(c)` ;
2. quel que soit le départ, les dates deviennent **périodiques** : il existe `K` et une **périodicité** `c` tels que `X(k + c) = c λ + X(k)` pour `k ≥ K`.

Pour un graphe d'événements temporisé, un circuit de `M` est un circuit du réseau, son poids est la somme des durées, sa longueur le nombre de jetons qu'il contient. Donc :

```text
λ = max over the circuits of the net of  (sum of the durations) / (sum of the tokens)
```

Les circuits qui atteignent ce maximum sont **critiques** : ils imposent le rythme de tout le système ; ce sont ses **goulots d'étranglement**.

### 5.2 À la main

**Howard2** a deux circuits :

| Circuit | Durées | Jetons | Moyenne |
|---------|-----------|:------:|------|
| `T0 → T2 → T1 → T0` | 5 + 3 + 5 = 13 | 2 | **6.5** |
| `T0 → T2 → T3 → T0` | 5 + 1 + 1 = 7 | 2 | 3.5 |

`λ = 6.5` et la périodicité est 2 : `T0: 8 8 21 21 34 34 47 47`, donc `T0(k + 2) = 13 + T0(k)` à partir de `k = 1`.

**JPQ** (`A` ci-dessus) a trois circuits : `x1 → x1` (3, 1 jeton), `x2 → x2` (4, 1 jeton), `x1 → x2 → x1` (2 + 7 = 9, 2 jetons : 4.5). `λ = 4.5`. Les dates sur 10 cycles : `Node 0: 0 8 12 17 21 26 30 35 39 44` : à partir de `k = 2`, les pas alternent 4 et 5, et `Node0(k + 2) = 9 + Node0(k)` : périodicité 2, `λ = 9 / 2`.

**Le flowshop** du chapitre 7 : la boucle de la machine `M1` dure 2 + 3 + 1 = 6 avec un jeton, celle de `M2` 4.5, celles des convoyeurs 3, 5 et 2.5, et les circuits mixtes moins : `λ = 6`.

### 5.3 Et si le réseau n'est pas fortement connexe ?

Alors chaque composante fortement connexe a sa propre valeur propre, et une transition va au rythme de la composante la plus lente **en amont** (une machine rapide alimentée par une lente attend). [SemiHoward](../../examples/Timed-Event-Graphs/SemiHoward/README.md) a une composante `{T0, T1}` de moyenne 1 et une composante `{T2}` de moyenne 2 qui l'alimente : les dates au plus tôt croissent de 2 pour chaque transition (`T0: 2 4 6 8`, `T1: 1 3 5 7`, `T2: 0 2 4 6`). L'éditeur trouve les deux composantes (`Found 2 connected components of the optimal policy`) et leurs circuits, mais son tableau de temps de cycle donne à chaque transition la moyenne de la composante qu'elle atteint, 1 pour `T0` et `T1` : faites confiance aux dates au plus tôt dans ce cas.

## 6. L'algorithme de Howard

Calculer le poids moyen de chaque circuit est impossible sur un grand réseau (il y en a exponentiellement). L'**itération sur les politiques** de Howard (R. Howard, 1960, pour les processus de décision markoviens ; adaptée au (max,+) par Cochet-Terrasson, Cohen, Gaubert, McGettrick et Quadrat, 1998) trouve `λ` en quelques itérations, en pratique presque linéairement dans la taille du réseau.

### 6.1 L'idée

Écrivez, pour chaque place, l'arc `i → j` entre ses deux transitions, sa durée `w` et ses jetons `τ`. Une **politique** choisit, pour chaque transition, **un** de ses arcs. Le graphe d'une politique a un arc par nœud : suivre les arcs finit toujours sur un circuit, dont la moyenne est facile à calculer. L'algorithme améliore la politique jusqu'à ce qu'aucun choix ne puisse rendre un circuit plus lourd :

1. **Démarrer** avec une politique quelconque.
2. **L'évaluer** : pour chaque circuit de la politique, sa moyenne `λ = Σw / Στ` ; puis le **biais** `v` de chaque transition, une date relative telle que, le long de l'arc choisi `i → j` : `v_i = w − λ τ + v_j` (avec `v = 0` en un nœud de chaque circuit).
3. **L'améliorer** : pour chaque transition, regarder tous ses arcs `i → j` ; préférer un arc qui mène à un circuit de plus grand `λ`, sinon celui de plus grand `w − λ τ + v_j`. Changer seulement si strictement meilleur.
4. Si rien n'a changé, **s'arrêter** : `λ` est le temps de cycle, les circuits de la politique sont les circuits critiques, `v` est le biais. Sinon retourner à 2.

### 6.2 À la main sur Howard2

Les arcs, en `from → to (w, τ)` : `T0 → T2 (5, 2)`, `T1 → T0 (5, 0)`, `T2 → T1 (3, 0)`, `T2 → T3 (1, 0)`, `T3 → T0 (1, 0)`. Seul `T2` a un choix.

| Itération | Politique de `T2` | Circuit | λ | Biais `v_T0, v_T1, v_T2, v_T3` | Amélioration à `T2` |
|-----------|----------------|---------|---|-------------------------------|---------------------|
| 1 | `T2 → T3` | `T0 T2 T3`: 7 / 2 | 3.5 | 0, 5, 2, 1 | `T2 → T1`: 3 + v_T1 = 8 > 1 + v_T3 = 2 : changer |
| 2 | `T2 → T1` | `T0 T2 T1`: 13 / 2 | 6.5 | 0, 5, 8, 1 | `T2 → T3`: 1 + 1 = 2 < 3 + 5 = 8 : garder |

Deux itérations : `λ = 6.5`, le circuit critique est `T0 → T2 → T1 → T0`, et le biais est `(0, 5, 8, 1)`. Vérifiez le biais de l'itération 2 : `v_T1 = 5 − 0 + v_T0 = 5`, `v_T2 = 3 + v_T1 = 8`, `v_T3 = 1 + v_T0 = 1`, et autour du circuit `v_T0 = 5 − 6.5 × 2 + v_T2 = 0`.

### 6.3 Dans l'éditeur

*Analysis > Critical circuit* exécute l'algorithme de Howard et donne les mêmes nombres :

```text
Strongly connected components: 1
Cycle time λ: 6.50 time units.
```

L'onglet *Circuit arcs* liste les places du circuit critique (`T0 P0 T2`, `T1 P1 T0`, `T2 P2 T1`), dessinées en rouge sur le réseau ; l'onglet *Bias* donne `T0: 0`, `T1: 5`, `T2: 8`, `T3: 1`.

![Critical circuit of Howard2](pics/08-Howard2-Critical.png)

Sur le flowshop, il dessine la boucle de la machine `M1`, `λ = 6` :

![Critical circuit of the flowshop](pics/08-Flowshop-Critical.png)

et sur `complex.flowshop` la colonne de `Conveyor3`, `λ = 16.95` (chapitre 7, §6.6) :

![Critical circuit of the big flowshop](pics/08-Complex-Critical.png)

L'algorithme exige que chaque transition ait au moins une place d'entrée (chaque nœud doit avoir un arc à choisir). Sur un réseau avec **entrées**, comme JPQ ou EventGraph, l'onglet dit `No optimal policy found` : calculez `λ` à la main à partir de la matrice `A` (§5.2), ou fermez l'entrée par une boucle (une source qui attend son dernier jeton revient à une place avec un jeton de et vers elle).

## 7. Dates au plus tôt et diagramme de Gantt

Le diagramme de Gantt du chapitre 7 est la solution de l'équation dateur à partir du marquage initial, cycle après cycle : `X(k) = D X(k) ⊕ A X(k-1) ⊕ B U(k)` résolue dans l'ordre des places sans jeton, avec les entrées toujours prêtes. C'est l'**ordonnancement le plus rapide** du système : aucune transition ne peut tirer plus tôt sans enfreindre une place. Après un transitoire, il devient périodique avec la période `λ` (ou `c λ` tous les `c` cycles).

## 8. Le point de vue (min,+)

Le compteur `x_i(t)` (le nombre de tirs jusqu'à la date `t`) obéit à `x_i(t) = min_j (m_ij + x_j(t − t_ij))` : une équation linéaire en (min,+), avec les jetons comme coefficients et les durées comme retards. Les deux vues décrivent le même réseau ; la vue dateur donne des dates pour un nombre d'événements donné, la vue compteur compte les événements pour une date donnée. À long terme, une transition tire `t / λ` fois jusqu'à la date `t`. *Analysis > Equations > Counter (min,+)* écrit cette forme, avec l'option *(min,+) notation*.

## 9. Julia

Chaque matrice de la fenêtre *Analysis* a un bouton *Copy as Julia*. *File > Export > Julia* (`.jl`) écrit tout le système pour [MaxPlus.jl](https://github.com/Lecrapouille/MaxPlus.jl) ; pour JPQ cela se termine par :

```julia
D = sparse([], [], MP([]), 2, 2) # States without tokens
A = sparse([1, 1, 2, 2], [1, 2, 1, 2], MP([3, 7, 2, 4]), 2, 2) # States with 1 token
B = sparse([2], [1], MP([1]), 2, 1) # Inputs
C = sparse([1], [1], MP([3]), 1, 2) # Outputs
S = MPSysLin(A, B, C, D)
```

En Julia, MaxPlus.jl calcule l'étoile, les valeurs propres, les fonctions de transfert et les simulations de ces systèmes. Le [chapitre 17](17-Julia.md) pilote l'éditeur depuis Julia.

## 10. Référence

| Onglet de *Analysis* | Donne |
|-------------------|-------|
| Critical circuit | λ, le nombre de composantes fortement connexes, *Circuit arcs* (rouge sur le réseau), *Bias*, *Help* |
| Linear system | `D`, `A`, `B`, `C` de la forme canonique, *Copy as Julia* |
| Adjacency | durées et jetons, entrée `(i, j)` = arc de `T_j` à `T_i` |
| Equations | dateur (max,+), compteur (min,+) |
| Gantt | les dates au plus tôt |

La bibliothèque donne les mêmes résultats à un programme : `findCriticalCycle()`, `toSysLin()`, `toAdjacencyMatrices()`, `earliestDates()`, `showDaterEquation()` ([chapitre 16](16-Library.md)).

## 11. À essayer

| Exemple | Faire | Observer |
|---------|----|---------|
| [Howard2](../../examples/Timed-Event-Graphs/Howard2/README.md) | *Critical circuit* ; changer la durée `T2 → T3` à 10 | λ = 6.5, puis l'autre circuit : (5 + 10 + 1) / 2 = 8 |
| [Howard1](../../examples/Timed-Event-Graphs/Howard1/README.md) | *Critical circuit* | λ = 2 : la boucle de `T1` (4 pour 2 jetons) |
| [JPQ](../../examples/Timed-Event-Graphs/JPQ/README.md) | *Linear system* | `D`, `A`, `B`, `C` du §4.1 |
| [SemiHoward](../../examples/Timed-Event-Graphs/SemiHoward/README.md) | *Critical circuit*, *Gantt* | deux composantes (§5.3) |
| [SemiNetherlands](../../examples/Timed-Event-Graphs/SemiNetherlands/README.md) | *Critical circuit* | λ = 47.67 sur un réseau de 8 transitions |
| [Flowshop](../../examples/Flowshops/README.md) | ajouter un convoyeur (`np`), puis regarder *Critical circuit* | λ baisse ; le circuit rouge peut passer d'une colonne à une ligne |

## 12. Pièges

- **Graphes d'événements seulement** : un choix dans le réseau et l'algèbre ne s'applique plus : l'onglet *(max,+) algebra* dit pourquoi, et *Show the faulty arcs* dessine les arcs à modifier.
- **Un circuit sans jeton** rend `D*` infini : le réseau ne démarre jamais.
- **Entrées** : l'onglet Howard exige que chaque transition ait une place d'entrée (§6.3).
- **Pas fortement connexe** : lisez le §5.3 avant de faire confiance à un temps de cycle par transition.

## 13. Pour aller plus loin

- F. Baccelli, G. Cohen, G. J. Olsder, J.-P. Quadrat, *Synchronization and Linearity*, Wiley, 1992.
- B. Heidergott, G. J. Olsder, J. van der Woude, *Max Plus at Work*, Princeton University Press, 2006 (le réseau ferroviaire néerlandais, dont l'exemple a inspiré `SemiNetherlands.teg`).
- J. Cochet-Terrasson, G. Cohen, S. Gaubert, M. McGettrick, J.-P. Quadrat, *Numerical computation of spectral elements in max-plus algebra*, IFAC SSC, 1998.
- Les tutoriels de [MaxPlus.jl](https://github.com/Lecrapouille/MaxPlus.jl), et les cours de [J.-P. Quadrat](https://jpquadrat.github.io/).
- La [bibliographie](Bibliography.md).

---

Suivant : [9. GRAFCET](09-GRAFCET.md) — des réseaux de Petri pour la commande des machines.
