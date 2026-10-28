# 5. 🧪 Réseaux de Petri étendus


[![fr](https://lecrapouille.github.io/icons/fr.png)](../fr/05-Extended-Petri-Nets.md) [![en](https://lecrapouille.github.io/icons/en.png)](../en/05-Extended-Petri-Nets.md)

**Ce que vous apprendrez**

- six extensions des réseaux de Petri du [chapitre 3](03-Petri-Nets.md) : **poids**, **capacités**, **arcs inhibiteurs**, **arcs de remise à zéro**, **priorités** et **jetons colorés** ;
- pour chacune : l’intuition, la modification de la règle de tir, un petit exemple, comment la dessiner dans l’éditeur et comment elle est enregistrée ;
- ce que coûte chaque extension : les analyses du [chapitre 4](04-Analysis.md) qui cessent de fonctionner, et pourquoi ;
- quels types de réseaux acceptent quelle extension.

**Prérequis** : [chapitre 3](03-Petri-Nets.md) (règle de tir, `Pre` et `Post`) et [chapitre 4](04-Analysis.md) (atteignabilité, Karp-Miller, invariants).

---

## Pourquoi étendre les réseaux de Petri ?

Les réseaux de Petri ordinaires (arcs de poids 1, places sans limite) suffisent en théorie pour beaucoup de systèmes, mais les modèles s’allongent : une machine qui prend une douzaine de pièces exige une douzaine d’arcs, un tampon de 4 places exige une place complémentaire, et certains comportements ne se décrivent pas du tout. « Tirer seulement quand la pièce est **vide** » est impossible : une transition ordinaire ne peut attendre la **présence** de jetons, jamais leur **absence**.

Les extensions raccourcissent et enrichissent les modèles. Le prix : plus un modèle peut dire, moins un algorithme peut en prouver des choses. Les extensions de ce chapitre sont portées par les arcs et les places, pas par le type du réseau : un réseau reste un *Petri net* ou un *timed Petri net*, et l’éditeur calcule quelles extensions il utilise.

| Extension | Sur | Règle | Exemple |
|-----------|-----|-------|---------|
| Poids | arc | prend ou donne `w` jetons d’un coup | [Weights](../../examples/Extended-Petri-Nets/Weights/README.md) |
| Capacité | place | jamais plus de `k` jetons | [Capacity](../../examples/Extended-Petri-Nets/Capacity/README.md) |
| Arc inhibiteur | arc depuis une place | tire seulement tant que la place a moins de `w` jetons | [Inhibitor](../../examples/Extended-Petri-Nets/Inhibitor/README.md) |
| Arc de remise à zéro | arc depuis une place | vide la place quand la transition tire | [Reset](../../examples/Extended-Petri-Nets/Reset/README.md) |
| Priorité | transition | la plus haute priorité gagne un conflit | [Priorities](../../examples/Extended-Petri-Nets/Priorities/README.md) |
| Couleur | arc | prend ou donne des jetons d’une couleur donnée | [Colors](../../examples/Extended-Petri-Nets/Colors/README.md) |

## 1. Poids

**Intuition.** Une réaction chimique `2 H2 + O2 → 2 H2O` consomme deux molécules d’hydrogène d’un coup. Une machine d’emballage prend 12 bouteilles pour faire une caisse. Un **poids** `w` sur un arc indique combien de jetons y passent.

**Règle.** C’est la forme générale déjà donnée au [chapitre 3](03-Petri-Nets.md#4-la-definition-mathematique) : `Pre(p, t)` et `Post(p, t)` sont les poids. `t` est autorisée quand `M(p) ≥ Pre(p, t)` pour toute place d’entrée, et la tirer donne `M − Pre(·, t) + Post(·, t)`. Un réseau ordinaire est un réseau dont tous les poids valent 1 ; les réseaux à poids sont parfois appelés *réseaux de Petri généralisés*.

**Exemple.** [Weights](../../examples/Extended-Petri-Nets/Weights/README.md) : `Burn` prend 2 jetons de `H2` et 1 de `O2`, en donne 2 de `H2O` ; `Electrolysis` fait l’inverse.

![Weights](../../examples/Extended-Petri-Nets/Weights/Weights.png)

```text
M0 = (4, 2, 0)  enabled: Burn
fire Burn -> (2, 1, 2)  enabled: Burn Electrolysis
fire Burn -> (0, 0, 4)  enabled: Electrolysis
Burn not enabled
fire Electrolysis -> (2, 1, 2)  enabled: Burn Electrolysis
```

Après deux tirs `H2` et `O2` sont vides et `Burn` n’est plus autorisée. Essayez avec 5 jetons dans `H2` : après deux tirs un jeton reste, seul, et `Burn` ne peut pas tirer, car il en faut 2.

**Analyse.** Les poids conservent toutes les analyses du chapitre 4. Les P-invariants prennent des poids : l’éditeur trouve `P0 (H2) + P2 (H2O) = 4` et `2 P1 (O2) + P2 (H2O) = 4`, la conservation des atomes d’hydrogène et d’oxygène.

**Dans l’éditeur.** Clic droit sur un arc > *Weight...* (ou *Duration and weight...* sur un réseau temporisé), ou double clic sur l’arc. Le poids est dessiné au milieu de l’arc quand il n’est pas 1. JSON : `"weight": 2` dans l’arc.

## 2. Capacités

**Intuition.** Un parking a 3 places ; un tampon entre deux machines contient 4 pièces. Une **capacité** `k` sur une place lui interdit de contenir plus de `k` jetons.

**Règle.** Une transition n’est autorisée que si, après son tir, aucune place de sortie ne dépasse sa capacité (en comptant les jetons qu’elle prend dans la même place, et, sur un réseau temporisé, les jetons déjà en route vers elle). Les places non bornées n’ont pas de capacité.

**Exemple.** [Capacity](../../examples/Extended-Petri-Nets/Capacity/README.md) : `Arrive` n’a pas de place d’entrée, donc elle est toujours autorisée… jusqu’à ce que le parking soit plein.

![Capacity](../../examples/Extended-Petri-Nets/Capacity/Capacity.png)

```text
M0 = (0, 0)  enabled: Arrive
fire Arrive -> (1, 0)  enabled: Arrive Leave
fire Arrive -> (2, 0)  enabled: Arrive Leave
fire Arrive -> (3, 0)  enabled: Leave
Arrive not enabled
fire Leave -> (2, 1)  enabled: Arrive Leave
fire Arrive -> (3, 1)  enabled: Leave
```

**L’astuce classique.** Une capacité peut être remplacée par une **place complémentaire** `p'` : donnez à `p'` les jetons `k − M0(p)`, et à chaque transition qui ajoute `w` jetons à `p` un arc qui prend `w` jetons à `p'` (et l’inverse pour celles qui prennent dans `p`). `p + p'` devient alors un P-invariant égal à `k`. Le réseau redevient ordinaire, et toutes les analyses reviennent. Les places `Remaining` de [SmarthomeSafety](../../examples/Petri-Nets/SmarthomeSafety/README.md) et `Mutex` de [MutualExclusion](../../examples/Petri-Nets/MutualExclusion/README.md) sont de telles places.

**Dans l’éditeur.** Clic droit sur une place > cocher *Max tokens* et donner la capacité. C’est dessiné `≤ 3` au-dessus de la place, et l’éditeur refuse d’ajouter des jetons au-delà. JSON : `"capacity": 3` dans la place. Une étape GRAFCET a la capacité 1 par nature.

## 3. Arcs inhibiteurs

**Intuition.** « Éteindre la lumière seulement quand la pièce est vide », « ne pas produire tant que la machine est arrêtée ». Un **arc inhibiteur** d’une place vers une transition teste **l’absence** de jetons. Il se dessine avec un petit cercle à la place de la pointe de flèche.

**Règle.** Un arc inhibiteur de poids `w` de `p` vers `t` ajoute la condition `M(p) < w` à l’autorisation de `t` (avec `w = 1` : `p` est vide). Il ne prend aucun jeton quand `t` tire.

**Exemple.** [Inhibitor](../../examples/Extended-Petri-Nets/Inhibitor/README.md) : `Switch off` ne tire que tant que `Inside` est vide.

![Arc inhibiteur](../../examples/Extended-Petri-Nets/Inhibitor/Inhibitor.png)

```text
M0 = (3, 0, 1, 0)  enabled: Enter Switch off
fire Enter -> (2, 1, 1, 0)  enabled: Enter Exit
Switch off not enabled
fire Exit -> (3, 0, 1, 0)  enabled: Enter Switch off
fire Switch off -> (3, 0, 0, 1)  enabled: Switch on
```

`Enter` lit aussi `Light on` avec un arc dans chaque sens : on entre seulement quand la lumière est allumée.

**Ce que ça coûte.** Le test à zéro est exactement ce qui manque aux réseaux ordinaires : avec des arcs inhibiteurs, les réseaux de Petri simulent une machine à compteurs (Minsky), et deviennent aussi puissants qu’un programme informatique (Turing-complets). Le prix est que l’atteignabilité, la bornitude et la vivacité deviennent **indécidables** en général : aucun algorithme ne peut y répondre pour tout réseau. Concrètement dans l’éditeur : le graphe d’atteignabilité se construit encore (il suit toutes les règles), mais Karp-Miller est refusé (plus de jetons peut désactiver une transition), et les invariants sont calculés sur les arcs normaux seulement, avec un avertissement :

![Propriétés de l’exemple inhibiteur](pics/05-Properties-Inhibitor.png)

*Le réseau est petit et borné : ses 5 marquages sont tous explorés, et les propriétés sont exactes. La note dit que les invariants ignorent l’arc inhibiteur.*

**Dans l’éditeur.** Clic droit sur un arc depuis une place > *Kind* > *inhibitor*. JSON : `"kind": "inhibitor"` (et `"weight"` pour `w`).

## 4. Arcs de remise à zéro

**Intuition.** Vider la poubelle, purger un tampon, remettre un compteur à zéro : prendre **tous** les jetons d’une place, quel qu’en soit le nombre. Un **arc de remise à zéro** se dessine avec une double pointe de flèche.

**Règle.** Un arc de remise à zéro de `p` vers `t` ne change pas l’autorisation de `t` (il n’a besoin d’aucun jeton) ; quand `t` tire, `p` devient vide.

**Exemple.** [Reset](../../examples/Extended-Petri-Nets/Reset/README.md) :

![Arc de remise à zéro](../../examples/Extended-Petri-Nets/Reset/Reset.png)

```text
M0 = (1, 0, 1)  enabled: Throw Empty
fire Throw -> (1, 1, 1)  enabled: Throw Empty
fire Throw -> (1, 2, 1)  enabled: Throw Empty
fire Throw -> (1, 3, 1)  enabled: Throw Empty
fire Empty -> (1, 0, 1)  enabled: Throw Empty
fire Empty -> (1, 0, 1)  enabled: Throw Empty
```

**Ce que ça coûte.** Les réseaux à remise à zéro s’analysent moins bien que les ordinaires : l’atteignabilité et la bornitude deviennent indécidables (Dufourd, Finkel et Schnoebelen, 1998). L’éditeur refuse Karp-Miller et avertit sur les invariants, comme pour les arcs inhibiteurs. Comme `Throw` peut tirer indéfiniment, le graphe d’atteignabilité de cet exemple est infini, et l’éditeur indique qu’il s’est arrêté :

```text
Reachable markings: 100000 (stopped)
...
Note: Karp-Miller does not apply to nets with reset arcs: the bounds and the dead transitions are the ones of the explored markings.
Note: Only the normal arcs are considered: the invariants may not hold with inhibitor or reset arcs.
```

**Dans l’éditeur.** Clic droit sur un arc depuis une place > *Kind* > *reset*. JSON : `"kind": "reset"`.

## 5. Priorités

**Intuition.** Une ambulance et une voiture attendent le même passage : l’ambulance passe en premier. Une **priorité** sur une transition tranche les conflits où elle se trouve.

**Règle.** Parmi les transitions qui peuvent tirer en même temps, celle de priorité la plus haute tire en premier ; à priorité égale, la première déclarée (identifiant le plus petit). Les priorités changent *laquelle* transition tire, jamais *si* une l’est.

**Où ça s’applique.** Les priorités comptent seulement quand le programme choisit :

- dans un **réseau de Petri temporisé** simulé par l’éditeur, avec *Simulation > Conflicts > Highest priority* (la politique par défaut est *Random*) ;
- dans le **runtime** ([chapitre 11](11-Runtime.md)), dont la politique par défaut est la priorité.

Dans un réseau de Petri simulé à la main, vous choisissez en cliquant, et la priorité n’est pas utilisée.

**Exemple.** [Priorities](../../examples/Extended-Petri-Nets/Priorities/README.md) : `Ambulance crosses` a la priorité 1, `Car crosses` la priorité 0.

![Priorités en marche](pics/05-Priorities-Running.png)

*Après 3 unités de temps de simulation : l’ambulance est passée en premier, son jeton revient en trajet. Les jetons sur les arcs sont en route ([chapitre 6](06-Timed-Petri-Nets.md)).*

**Ce que ça coûte.** L’analyse ignore les priorités : elle explore chaque transition autorisée, donc elle peut trouver des marquages, et des interblocages, que les priorités rendent impossibles. L’inverse est sûr : les priorités ne choisissent qu’entre transitions autorisées, donc un réseau prouvé sans interblocage sans elles le reste avec.

**Dans l’éditeur.** L’onglet *Transitions* de l’inspecteur a une colonne priorité ; *Simulation > Conflicts* choisit la politique, et la seed de l’aléatoire (*Replay the last simulation*). JSON : `"priority": 1` dans la transition.

## 6. Jetons colorés

### 6.1 Les réseaux de Petri colorés en général

Dans les **réseaux de Petri colorés** de Kurt Jensen (1981, l’outil CPN Tools), un jeton porte une valeur d’un type (sa couleur : un entier, un enregistrement, un client avec son nom), et les arcs portent des expressions qui disent quelles valeurs sont prises et produites. Toute une famille de sous-réseaux similaires se replie en un seul : une place `Philosopher` avec trois jetons colorés au lieu de trois places. Les réseaux colorés sont aussi expressifs que des programmes, et leur analyse déplie en général vers des réseaux ordinaires.

### 6.2 Les couleurs de cet éditeur : des résultats qui choisissent la suite

Cet éditeur met en œuvre une idée plus simple, empruntée à [Symmetri](https://github.com/thorstink/Symmetri) : une couleur est un **nom**, et c’est surtout le **résultat** de l’action d’une transition.

- Un jeton a une couleur parmi `Success`, `Failed`, `Canceled` (intégrées) et les couleurs du réseau (sa liste `colors`).
- Un arc **depuis** une place peut demander une couleur : la transition ne prend que les jetons de cette couleur. Sans couleur, elle prend n’importe quel jeton.
- Un arc **vers** une place peut donner une couleur. Sans couleur, il donne le **résultat** de la fonction de la transition (`Success`, `Failed`, ou une couleur à vous) quand le réseau est exécuté par le runtime.

**Pas encore un réseau de Petri coloré général.** Une couleur n’est qu’un nom : un jeton ne porte pas d’objet typé (identifiant, valeur, ensemble de champs), et les arcs n’ont pas d’expressions ni de liaisons sur les valeurs des jetons. Les réseaux de Petri colorés généraux au sens du §6.1 sont prévus pour une version ultérieure.

Ainsi une transition dont la fonction peut réussir ou échouer met un jeton dont la couleur est son résultat, et les arcs suivants choisissent : l’arc `Success` continue, l’arc `Failed` retente. Le résultat d’une fonction **choisit la suite**.

**Exemple.** [Colors](../../examples/Extended-Petri-Nets/Colors/README.md) : `Inspect` renvoie `Good` ou `Bad` ; `Ship` ne prend que les jetons `Good`, `Scrap` seulement les `Bad`.

![Colors](../../examples/Extended-Petri-Nets/Colors/Colors.png)

Exécution par le runtime (`build/examples/Colors`, [chapitre 11](11-Runtime.md)) :

```text
inspect part 1: Good
inspect part 2: Bad
inspect part 3: Good
  ship it
  ship it
  scrap it
lot_1: Deadlocked, shipped 2, scrapped 1
```

Le même mécanisme choisit `Accept` ou `Reject Coin` dans [Coffee](../../examples/Petri-Nets/Coffee/README.md) et le type d’appel dans [EmergencyCalls](../../examples/Petri-Nets/EmergencyCalls/README.md).

**Dans la simulation.** La simulation de l’éditeur suit les couleurs avec la même règle que le runtime : un arc qui demande une couleur ne prend que les jetons de cette couleur. Il n’y a pas de fonction pour donner un résultat, donc le résultat d’un tir est tiré au hasard parmi les couleurs demandées par les arcs après la transition (dans Colors : `Good` ou `Bad`), `Success` quand aucun arc n’en demande. Le canevas dessine chaque jeton dans une place avec sa couleur (jetons `Success` comme d’habitude, `Failed` en rouge, chaque couleur à vous avec sa teinte, celle de son nom sur les arcs), et ne marque comme autorisées que les transitions que les couleurs permettent. L’[analyse](04-Analysis.md) suit les couleurs avec la même règle, et explore chaque résultat possible.

**Dans l’éditeur.** La fenêtre *Colors* liste les couleurs de l’utilisateur : *Add* en crée une. Clic droit sur un arc > *Color of the tokens*, ou la colonne Color de *Arcs*, en choisit une ; *any* la retire. Une nouvelle couleur peut aussi être saisie dans le menu de l’arc. JSON : `"color": "Good"` dans l’arc, et `"colors": ["Good", "Bad"]` dans le réseau pour les couleurs utilisateur. Les jetons initiaux d’une place sont `Success` ; `"colors": { "Phi0": 1, "Phi1": 1 }` dans une place colore une partie d’entre eux (les autres restent `Success`). Dans l’éditeur : clic droit sur une place > *Tokens by color*, ou *colors* dans le panneau *Places*, où *Color of the tokens* choisit la couleur ajoutée ou retirée par les flèches et les touches **+** et **-**. L’infobulle d’une place liste ses jetons par couleur. Clic droit sur une transition > *Result in the simulation* remplace le résultat aléatoire par une couleur (pour simuler un échec, par exemple). Exemple : [ColoredPhilosophers](../../examples/Extended-Petri-Nets/ColoredPhilosophers/README.md).

## 7. Tout ensemble : le tampon borné

[BoundedBuffer](../../examples/Extended-Petri-Nets/BoundedBuffer/README.md) utilise quatre extensions dans une petite ligne de production : `Buffer` contient au plus 4 pièces (capacité), `Pack` en prend 2 d’un coup (poids), `Produce` est bloquée tant que `Stopped` a un jeton (arc inhibiteur), `Flush` vide le tampon (arc de remise à zéro). `Stop`, `Restart` et `Ask flush` permettent d’essayer chacune.

![Tampon borné](../../examples/Extended-Petri-Nets/BoundedBuffer/BoundedBuffer.png)

Exécution par le runtime sur une horloge simulée (une pièce par unité de temps, un paquet de 2 toutes les 3), les quatre règles apparaissent dans le tableau (`build/examples/BoundedBuffer`) :

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

Aux dates 2, 4, 5 et 7 le tampon est plein : la capacité bloque le producteur. De 8 à 11 l’arc inhibiteur l’arrête : seul l’emballeur travaille. À 15 l’arc de remise à zéro vide le tampon.

## 8. Ce que coûtent les extensions

| Extension | Graphe d’atteignabilité | Karp-Miller (bornitude des réseaux infinis) | Invariants | Décidabilité |
|-----------|:---:|:---:|:---:|---|
| Poids | oui | oui | oui, avec poids | comme réseaux ordinaires |
| Capacités | oui | refusé | la capacité n’est pas dans `C` | comme réseaux ordinaires (places complémentaires) |
| Arcs inhibiteurs | oui | refusé | arcs normaux seulement, avertissement | Turing-complet : indécidable en général |
| Arcs de remise à zéro | oui | refusé | arcs normaux seulement, avertissement | atteignabilité et bornitude indécidables |
| Priorités | ignorées (toutes transitions autorisées) | ignorées | ignorées | — |
| Couleurs | ignorées (n’importe quelle couleur) | ignorées | ignorées | — |

Karp-Miller exige une règle **monotone** : si une transition est autorisée dans `M`, elle doit l’être dans tout marquage plus grand. Les arcs inhibiteurs (plus de jetons désactive), les arcs de remise à zéro et les capacités cassent cela, donc l’éditeur le refuse pour eux (« Karp-Miller does not apply to nets with places with a capacity »). Un réseau borné est encore entièrement analysé par son graphe d’atteignabilité, quelles que soient ses extensions ; seuls les infinis échappent à l’analyse.

## 9. Quels réseaux acceptent quelle extension

| Type de réseau | Poids | Capacités | Inhibiteur | Reset | Couleurs | Priorités |
|----------------|:---:|:---:|:---:|:---:|:---:|:---:|
| Petri net | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ (runtime) |
| Timed Petri net | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Timed event graph | ❌ | ❌ | ❌ | ❌ | ❌ | — |
| GRAFCET | ❌ | ❌ (1 par nature) | ❌ | ❌ | ❌ | — |

Un graphe d’événements doit n’avoir que des arcs ordinaires, pour que ses dates obéissent aux équations (max,+) du [chapitre 8](08-Max-Plus-Algebra.md) ; un GRAFCET suit les règles de l’IEC 60848 ([chapitre 9](09-GRAFCET.md)). Changer le type d’un réseau qui utilise une extension refusée (*Edit > Type of net*) échoue avec un message la nommant (par exemple « A GRAFCET cannot have inhibitor arcs ») et marque les arcs en cause. Les menus des arcs n’affichent que les extensions que le type accepte.

## 10. Essayez

| Exemple | Faire | Observer |
|---------|-------|----------|
| [Weights](../../examples/Extended-Petri-Nets/Weights/README.md) | tirer `Burn` deux fois, puis `Electrolysis` | les invariants pondérés dans *Analysis > Properties* |
| [Capacity](../../examples/Extended-Petri-Nets/Capacity/README.md) | tirer `Arrive` jusqu’à ce qu’elle grise | une place pleine bloque ses transitions entrantes |
| [Inhibitor](../../examples/Extended-Petri-Nets/Inhibitor/README.md) | laisser entrer des gens, essayer `Switch off` | un test à zéro |
| [Reset](../../examples/Extended-Petri-Nets/Reset/README.md) | jeter 3 choses, puis `Empty` | tous les jetons partent d’un coup |
| [Priorities](../../examples/Extended-Petri-Nets/Priorities/README.md) | exécuter avec *Random*, puis *Highest priority* | qui passe en premier |
| [Colors](../../examples/Extended-Petri-Nets/Colors/README.md) | lancer `build/examples/Colors` | le résultat d’une fonction choisit la branche |
| [BoundedBuffer](../../examples/Extended-Petri-Nets/BoundedBuffer/README.md) | tirer `Stop`, puis `Ask flush` | quatre extensions dans un même réseau |

## 11. Pièges

- **Un arc inhibiteur de poids 2** ne teste pas le vide : il autorise jusqu’à 1 jeton. Vérifiez le poids.
- **Un arc de remise à zéro n’autorise pas** : la transition a besoin de ses autres places d’entrée ; un arc de remise à zéro seul sur une place vide convient.
- **Couleurs dans l’éditeur** : si la simulation ne choisit pas la branche attendue, rappelez-vous qu’elle ignore les couleurs ; exécutez le réseau avec le runtime.
- **Priorités dans un réseau de Petri simulé à la main** : elles n’ont aucun effet, vous choisissez.
- **Préférez les places complémentaires aux capacités** quand vous avez besoin de l’analyse complète d’un réseau qui semble non borné.

## 12. Pour aller plus loin

- K. Jensen, L. M. Kristensen, *Coloured Petri Nets: Modelling and Validation of Concurrent Systems*, Springer, 2009.
- C. Dufourd, A. Finkel, P. Schnoebelen, *Reset nets between decidability and undecidability*, ICALP 1998.
- M. Minsky, *Computation: Finite and Infinite Machines*, 1967 (machines à compteurs, la raison pour laquelle les arcs inhibiteurs donnent la puissance de Turing).
- [Symmetri](https://github.com/thorstink/Symmetri), le runtime qui a inspiré les couleurs de résultats.

---

Suivant : [6. Réseaux de Petri temporisés](06-Timed-Petri-Nets.md) — ajouter le temps : durées, délais, temporisations.
