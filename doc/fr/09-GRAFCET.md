# 9. 🔘 GRAFCET


[![fr](https://lecrapouille.github.io/icons/fr.png)](../fr/09-GRAFCET.md) [![en](https://lecrapouille.github.io/icons/en.png)](../en/09-GRAFCET.md)

**Ce que vous apprendrez**

- d'où vient le GRAFCET, et en quoi il diffère d'un réseau de Petri et du langage SFC des automates ;
- ses éléments : étapes, transitions, réceptivités, actions, liaisons ;
- les cinq règles d'évolution de la norme IEC 60848, et la recherche de la situation stable ;
- les structures : séquence, choix, parallélisme, boucles ;
- comment écrire les réceptivités (logique, fronts, temps, comparaisons) et les actions (les dix qualificateurs, actions conditionnelles, ordres de forçage) ;
- comment l'éditeur exécute un GRAFCET : le pupitre de commande, les sorties, le chronogramme, les temporisations ;
- dessiner votre propre GRAFCET de zéro.

**Prérequis** : [3. Réseaux de Petri](03-Petri-Nets.md) (places, transitions, jetons, règle de tir). Le temps du [chapitre 6](06-Timed-Petri-Nets.md) n'est pas nécessaire : le GRAFCET a ses propres opérateurs de temps.

---

## 1. D'où vient le GRAFCET

Dans les années 1970, l'automatisation des machines se décrivait par des schémas à relais et de longs textes qu'on ne pouvait pas vérifier. En 1977, un groupe de travail de l'AFCET (association française d'informatique et de commande) et de l'ADEPA (agence pour l'automatisation de l'industrie française) a proposé le **GRAFCET** (*GRAphe Fonctionnel de Commande Étapes-Transitions*). L'idée était de garder la clarté graphique des réseaux de Petri, mais de faire de chaque place un état de la machine et de lire les capteurs sur les transitions.

Le GRAFCET est devenu la norme française NF C03-190, puis la norme internationale **IEC 60848** (1988, dernière édition 2013). Il est enseigné dans chaque lycée technique français : quand un ingénieur français dessine « une séquence », c'est un GRAFCET.

Ne le confondez pas avec le **SFC** (*Sequential Function Chart*) de la norme IEC 61131-3, qui est l'un des cinq langages de programmation des automates programmables (API). Le SFC s'est inspiré du GRAFCET, mais :

| | GRAFCET (IEC 60848) | SFC (IEC 61131-3) |
|-|---------------------|-------------------|
| Nature | un langage de **spécification** : ce que la machine doit faire | un langage de **programmation** d'un automate |
| Évolution | toutes les transitions qui peuvent franchir le font ; les graphes évoluent jusqu'à stabilité avant le calcul des actions | une évolution par balayage de l'automate ; les détails dépendent du constructeur |
| Actions | qualificateurs, actions conditionnelles, ordres de forçage entre graphes | qualificateurs, blocs d'action écrits dans d'autres langages (ST, ladder ...) |

Cet éditeur suit **IEC 60848** : vous spécifiez le comportement, l'exécutez sur une machine virtuelle (le pupitre de commande), puis en générez le code ([chapitre 12](12-Code-Generation.md)).

## 2. Un GRAFCET est un réseau de Petri interprété

Un GRAFCET est un réseau de Petri avec trois changements :

1. une place est une **étape**, et elle contient au plus un jeton : l'étape est **active** ou **inactive** (le réseau est *sûr*, au sens du [chapitre 4](04-Analysis.md)) ;
2. une transition a une **réceptivité** : une condition booléenne sur les entrées de la machine (boutons, capteurs), sur le temps et sur les autres étapes. Le réseau est *interprété* : le monde extérieur décide quand une transition franchit ;
3. une étape fait des **actions** tant qu'elle est active : elle commande les sorties de la machine (moteurs, vannes, lampes).

```text
   ╔════╗
   ║ 0  ║ Wait                 <- initial step (double square): active at the start
   ╚═╤══╝
     ┼  Dcy                    <- transition and its receptivity: the button Dcy
   ┌─┴──┐ ┌───┬───────┐
   │ 1  ├─┤ N │ Motor │        <- step 1 and its action: Motor runs while step 1 is active
   └─┬──┘ └───┴───────┘
     ┼  30s/X1                 <- 30 s after step 1 became active
     ⋮
```

| Élément | Dessin | Signification |
|---------|-------|---------|
| **Étape** | un carré avec son numéro | un état de la machine. L'étape `n` s'écrit `Xn` dans les réceptivités : `X3` est vraie tant que l'étape 3 est active |
| **Étape initiale** | un double carré | active au départ (elle a un jeton dans le fichier) |
| **Transition** | une barre sur la liaison | le passage des étapes au-dessus vers celles en dessous |
| **Réceptivité** | le texte à droite de la barre | la condition de la transition |
| **Action** | une boîte à droite de son étape | ce que fait la machine tant que l'étape est active (ou au changement, avec un qualificateur) |
| **Liaison** | une ligne verticale | elle va vers le bas ; une liaison remontante a une flèche |
| **Entrée** | un nom dans une réceptivité | ce que le GRAFCET lit : `Dcy`, `s1`, `Temperature` |
| **Sortie** | le nom d'une action | ce que le GRAFCET commande : `Motor` |

Dans l'éditeur, le grand numéro dans une étape est son identifiant (le `n` de `Xn`) ; le petit texte en dessous est sa légende. Une transition affiche son numéro entre parenthèses à gauche, `(0)`, quand *View > Transition identifiers* est coché.

## 3. Les cinq règles d'évolution

L'IEC 60848 donne cinq règles. Les voici, chacune avec un petit exemple, et comment l'éditeur les applique (`GrafcetEngine`, dans [`src/PetriNet/GrafcetEngine.cpp`](../src/PetriNet/GrafcetEngine.cpp)).

**Règle 1 — Situation initiale.** Au départ, les étapes initiales sont actives, et elles seules. Dans le fichier, une étape initiale a un jeton ; dans l'éditeur, **+** sur une étape la rend initiale (double carré).

**Règle 2 — Une transition peut être franchie** quand toutes les étapes juste au-dessus sont actives (la transition est **validée**) *et* sa réceptivité est vraie. Une transition qui peut être franchie **doit** l'être : il n'y a pas de choix, contrairement à un réseau de Petri.

```text
   [1] active, receptivity a = 1      ->  the transition fires
   [1] active, receptivity a = 0      ->  it waits
   [1] inactive, receptivity a = 1    ->  nothing: the transition is not enabled
```

Dans l'éditeur, une réceptivité vraie est dessinée en orange, même quand sa transition n'est pas validée : on voit tout de suite ce qui attend quoi.

**Règle 3 — Franchir une transition** désactive toutes les étapes juste au-dessus et active toutes celles juste en dessous, en même temps.

**Règle 4 — Les transitions qui peuvent être franchies ensemble le sont.** Si deux transitions peuvent être franchies au même instant, les deux le sont. Il n'y a ni ordre ni conflit à résoudre.

**Règle 5 — Une étape désactivée et activée en même temps reste active.** Cela arrive quand une transition revient à l'étape qu'elle quitte, ou quand deux transitions franchissent ensemble autour de la même étape. L'éditeur désactive d'abord toutes les étapes au-dessus des transitions qui franchissent, puis active celles en dessous : l'étape finit active.

### 3.1 La situation stable

Un cycle du moteur (`step(dt)`) fait ceci :

1. laisser passer `dt` secondes pour les temporisations (`3s/X2`, les délais) ;
2. appliquer les **ordres de forçage** des étapes actives (§7) ;
3. **évoluer** : trouver toutes les transitions qui peuvent être franchies, les franchir ensemble (règles 2 à 5), et encore, jusqu'à ce qu'aucune ne le puisse. Les graphes sont alors **stables** ;
4. calculer les **actions** à partir de la situation stable.

Les étapes traversées à l'étape 3 sont **transitoires** : elles étaient actives un temps nul, donc leurs actions continues ne s'exécutent pas. C'est ce que demande la norme : un moteur ne doit pas démarrer une microseconde parce que le GRAFCET est passé par une étape. Lisez le séchoir du [chapitre 10](10-GEMMA.md) : avec le bouton `Start` maintenu, le graphe `Control` passe de l'étape 3 à l'étape 4 et, *dans le même cycle*, le graphe `Production` voit `Control:X4` et va à l'étape 1. Les deux se produisent en un cycle parce que la recherche de la situation stable fait évoluer tous les graphes ensemble, encore et encore.

Un **front** (`↑a`, §5) est vrai pendant une seule évolution (un *instant*) : appuyer sur un bouton vu avec `↑Ack` franchit une transition une fois, même si le graphe revient à une étape qui attend `↑Ack` pendant le même cycle.

Si les graphes ne deviennent jamais stables (après 256 évolutions), le moteur arrête la recherche et la console avertit : *"The GRAFCET does not reach a stable situation after 256 evolutions: is there a loop of transitions always true?"*. Voir les pièges (§12).

## 4. Les structures

Un GRAFCET se construit à partir de quelques structures. Chacune a un exemple dans [`examples/GRAFCET`](../../examples/GRAFCET).

### 4.1 Séquence et boucle

Des étapes les unes après les autres, la dernière transition revenant à la première (une liaison remontante, avec une flèche) : le [feu tricolore allemand](../../examples/GRAFCET/GermanTrafficLights/README.md) est une séquence de quatre étapes, vert, jaune, rouge, rouge et jaune.

### 4.2 Choix : divergence OU et convergence OU

Une étape suivie de plusieurs transitions est une **divergence OU** (dessinée avec une seule ligne horizontale) : le GRAFCET prend la branche dont la réceptivité est vraie. Les branches se rejoignent sur une étape précédée de plusieurs transitions : une **convergence OU**.

![Exclusive choice](pics/09-ExclusiveChoice-running.png)

*[ExclusiveChoice](../../examples/GRAFCET/ExclusiveChoice/README.md) : `A . B` va à l'étape 1, `A . /B` à l'étape 2. Les réceptivités `/A` sont vraies (orange) mais leurs transitions ne sont pas validées : les étapes 1 et 2 sont inactives.*

Par la règle 4, si deux réceptivités d'une divergence OU sont vraies ensemble, **les deux** transitions franchissent, et les deux branches deviennent actives : ce n'est presque jamais voulu. Rendez les réceptivités **exclusives** : `A . B` et `A . /B` ne peuvent pas être vraies ensemble. Une écriture courante est une priorité : `b1` sur une branche, `b2 . /b1` sur l'autre.

### 4.3 Parallélisme : divergence ET et convergence ET

Une transition suivie de plusieurs étapes est une **divergence ET** (dessinée avec une double ligne horizontale) : toutes les branches démarrent ensemble et tournent en parallèle. Une transition précédée de plusieurs étapes est une **convergence ET** (double ligne aussi) : elle attend que toutes les branches aient atteint leur dernière étape, puis franchit une fois.

![Parallel sequences](pics/09-ParallelSequences.png)

*[ParallelSequences](../../examples/GRAFCET/ParallelSequences/README.md) : après `m`, les étapes 1 et 4 sont actives ensemble. La dernière transition, `true`, attend les étapes 3 et 8 : la réceptivité est toujours vraie, la convergence ET seule la fait attendre.*

### 4.4 Sauts

Un saut est une liaison qui saute des étapes (en avant) ou revient en arrière (une boucle dans une branche). Dans ParallelSequences, `c0 . m` saute de l'étape 6 à l'étape 7, et `c1` revient à l'étape 6 : une boucle dans la branche droite, quittée par `c0 . /m`.

## 5. Réceptivités

### 5.1 Syntaxe

| Écrire | Signifie | Aussi accepté |
|-------|-------|---------------|
| `a . b` | a et b | `a * b`, `a and b` |
| `a + b` | a ou b | `a \| b`, `a or b` |
| `/a` | non a | `!a`, `not a` |
| `( ... )` | parenthèses | |
| `1`, `0` | toujours vrai, jamais vrai | `true`, `false` |
| (vide) | toujours vrai | |
| `X3` | l'étape 3 de ce graphe est active | |
| `Safety:X3` | l'étape 3 du graphe nommé Safety est active | |
| `↑a` | front montant : a vient de devenir vrai (vrai un instant) | `^a`, `rising(a)` |
| `↓a` | front descendant : a vient de devenir faux | `falling(a)` |
| `3s/X2` | vrai 3 s après que l'étape 2 est devenue active | unités `ms`, `s`, `min`, `h` |
| `3s/a/7s` | vrai 3 s après que a est devenu vrai, faux 7 s après que a est devenu faux | |
| `[count >= 3]` | comparaison d'une entrée entière | `=`, `!=`, `<`, `<=`, `>`, `>=` |
| `Dcy`, `sensor_2` | une entrée de la machine | noms avec arguments, comme `holding(cube)` |

**Priorité** : `not` d'abord, puis `and`, puis `or`. Donc `a + b . /c` signifie `a + (b . (/c))`. Utilisez des parenthèses en cas de doute.

Les anciens fichiers écrivaient les réceptivités en notation polonaise inverse (`a X0 ! .` pour `a . /X0`) ; l'éditeur les convertit à l'ouverture.

### 5.2 Temps

L'**opérateur de temps** `t1/e/t2` (IEC 60848) surveille une expression `e` :

```text
      e   ____|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾|________________
t1/e/t2   ____________|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾|______
              <- t1 ->                <- t2 ->
```

L'usage le plus courant est `t1/Xn` : vrai `t1` après que l'étape `n` est devenue active. C'est ainsi qu'un GRAFCET attend : `30s/X1` laisse un moteur tourner 30 secondes.

Une transition peut aussi avoir un **délai** (le champ *Delay* du panneau *Transitions*) : elle ne franchit qu'après avoir pu l'être pendant ce temps. `Dcy` avec un délai de 2 s revient au même que `2s/Dcy`.

Pendant la simulation, chaque temporisation qui compte est dessinée sous sa réceptivité sous forme de barre, `1.0 / 3 s`, et listée dans le panneau *Transitions* ; le panneau *Steps* indique depuis quand chaque étape active l'est.

### 5.3 Dans l'éditeur

Double clic sur une transition pour écrire sa réceptivité : l'éditeur vérifie la syntaxe pendant la saisie, et une erreur reste rouge dans le panneau *Transitions*. *Syntax help* (dans ce panneau) et *Help > GRAFCET receptivities* montrent le tableau ci-dessus. Tout nom qui n'est ni une étape ni un mot-clé devient une entrée du pupitre de commande (§9).

## 6. Actions

Une action a un **qualificateur**, un **nom** (sa sortie), une **condition** optionnelle et, pour les qualificateurs temporisés, une **durée** `d`. Plusieurs actions du même nom commandent la même sortie : la sortie est active tant que l'une d'elles l'est.

### 6.1 Les dix qualificateurs

```text
 step active     ____|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾|____________________
 N               ____|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾|____________________   while the step is active
 D  (d)          ____________|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾|____________________   d after the activation
 L  (d)          ____|‾‾‾‾‾‾‾|____________________________________   during d at most
 P1              ____|‾|_______________________________________   one cycle at the activation
 P0              __________________________________|‾|_________   one cycle at the deactivation
 S               ____|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾...   until an R of the same name
 SD (d)          ____________|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾...   d after the activation, then stored
 DS (d)          ____________|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾...   stored if the step is still active after d
 SL (d)          ____|‾‾‾‾‾‾‾|____________________________________   during d, even if the step is left
```

| Qualificateur | Nom | La sortie est active ... |
|-----------|------|----------------------|
| `N` | continue | tant que l'étape est active |
| `S` | mémorisée (set) | depuis l'activation de l'étape, jusqu'à une action `R` du même nom |
| `R` | remise à zéro (reset) | (elle éteint l'action mémorisée du même nom, tant que son étape est active) |
| `D` | différée | `d` secondes après l'activation, tant que l'étape reste active |
| `L` | limitée | pendant les `d` premières secondes de l'étape |
| `SD` | mémorisée puis différée | `d` secondes après l'activation, même si l'étape a été quittée ; puis mémorisée jusqu'à `R` |
| `DS` | différée puis mémorisée | si l'étape est encore active après `d` secondes ; puis mémorisée jusqu'à `R` |
| `SL` | mémorisée et limitée | pendant `d` secondes à partir de l'activation, même si l'étape est quittée |
| `P1` | impulsion à l'activation | pendant un cycle quand l'étape devient active |
| `P0` | impulsion à la désactivation | pendant un cycle quand l'étape devient inactive |

*Help > GRAFCET actions (qualifiers)* dessine le chronogramme de chaque qualificateur ; dans le panneau *Actions*, choisir un qualificateur affiche son propre diagramme (*Qualifiers help*).

### 6.2 Actions conditionnelles

Une action peut avoir une **condition** écrite comme une réceptivité : l'action ne s'exécute que tant que son étape est active *et* sa condition est vraie. C'est dessiné `if Right: Right Motor`. Dans le séchoir du [chapitre 10](10-GEMMA.md), l'étape « Marche manuelle » a `N Right Motor` avec la condition `Right` : le moteur tourne seulement tant que l'opérateur maintient le bouton.

### 6.3 Exemple : un vérin pneumatique

![GRAFCET actions](pics/09-GrafcetActions.png)

*[GrafcetActions](../../examples/GRAFCET/GrafcetActions/README.md) : `N V1+` pendant l'extension, `D Timer1` 2 s après l'étape 2, `S Lamp1` à l'étape 2 et `R Lamp1` à l'étape 3, `N Buzzer` et `L AlarmLamp` (5 s) à l'alarme. Le panneau *Actions* liste les actions de chaque étape.*

## 7. Ordres de forçage

Un **ordre de forçage** est une action d'un graphe qui impose une situation à un autre graphe. Tant que l'étape qui le porte est active, le graphe forcé prend la situation indiquée et **n'évolue pas** : le forçage a priorité sur les règles d'évolution. C'est ainsi qu'un graphe de sécurité arrête tout.

| Ordre | Le graphe Production ... |
|-------|--------------------------|
| `Production{init}` | prend sa situation initiale |
| `Production{2,5}` | a les étapes 2 et 5 actives, et elles seules |
| `Production{}` | n'a aucune étape active |
| `Production{*}` | est figé dans sa situation actuelle |

Le deux-points de l'ancienne écriture est accepté : `Production:{init}`. Un ordre de forçage est dessiné dans un double cadre ; une action qui ne contient que des ordres de forçage n'a pas de nom. Dans le panneau *Actions*, *Add order* en ajoute un. Les ordres de forçage sont appliqués au début de chaque cycle, à partir de la situation du cycle précédent (§3.1). Le [chapitre 10](10-GEMMA.md) s'en sert pour construire une GEMMA.

## 8. Plusieurs graphes dans un document

Un document peut contenir plusieurs GRAFCET (*Edit > Add a net to the document > GRAFCET*, ou *File > New > GEMMA (3 GRAFCETs)*). Ils s'exécutent ensemble, avec un seul moteur : une recherche de situation stable pour tous. Un graphe lit les étapes d'un autre avec `Name:Xn` et le force avec `Name{...}`. Le nom d'un graphe (et sa description) se change avec *Edit* dans l'onglet *Document*, en bas à côté de la *Console*.

## 9. Entrées, sorties et pupitre de commande

Les **entrées** sont les noms lus par les réceptivités et les conditions. Les **sorties** sont les noms des actions. Quand la simulation démarre, l'éditeur les collecte et affiche :

- le **pupitre de commande** : le poste de la machine, un widget par entrée ;
- la fenêtre **Outputs** : un actionneur par sortie ;
- le **chronogramme** (un onglet à côté de la console) : étapes, entrées et sorties dans le temps.

![German traffic lights](pics/09-GermanTrafficLights-running.png)

*Le [feu tricolore allemand](../../examples/GRAFCET/GermanTrafficLights/README.md) après 5 s : l'étape 2 est active, sa temporisation `3s/X2` compte (`1.0 / 3 s`), le feu rouge est allumé dans la fenêtre Outputs.*

Par défaut une entrée est un interrupteur. *Edit* sur le pupitre choisit comment chaque entrée est dessinée et ce qu'elle fait ; c'est enregistré dans le fichier ([chapitre 15](15-File-Formats.md)) :

| Type | Dessin | Comportement |
|------|----------|----------|
| `switch` | commutateur 0 / 1 | un clic l'allume ou l'éteint, et il reste |
| `button` | bouton poussoir | actif tant qu'on appuie (au moins un cycle) ; un clic droit le maintient enfoncé |
| `pulse` | bouton carré | actif pendant un cycle par clic |
| `emergency` | champignon rouge sur jaune | enfoncé (actif) jusqu'à un second clic qui le relâche |
| `selector` | sélecteur rotatif | ses positions sont les entrées du même groupe ; une seule est active |
| `value` | jauge et curseur | un entier (température, niveau, compteur), lu avec `[Temperature >= 80]` ; peut suivre une sortie : monte tant que la sortie est active, descend sinon |
| `clock` | voyant clignotant | actif pendant la moitié de sa période |

| Type de sortie | Dessin |
|-------------|----------|
| `light` | un voyant de la couleur de son action |
| `motor` | un ventilateur qui tourne tant qu'il est actif |
| `heater` | une résistance qui rougit tant qu'elle est active |
| `valve` | un vérin dont la tige sort tant qu'il est actif |
| `horn` | des ondes sonores tant qu'il est actif |

Simulation arrêtée, un clic sur le pupitre change la **valeur initiale** de l'entrée (enregistrée avec le document). Pendant l'exécution, un clic agit tout de suite. Pour changer plusieurs entrées **au même instant** (un sélecteur et un bouton), cochez *Apply together* : les changements attendent *Apply* (ou **F7**). *Fit* dessine des boutons plus petits pour tous les voir.

Le **chronogramme** a des cases pour choisir les lignes (*Steps*, *Inputs*, *Outputs*), *Only what changed* pour masquer les lignes qui n'ont jamais bougé, et *Follow* pour afficher les dernières secondes ; la souris sur le diagramme donne l'heure et la valeur. Les étapes sont bleues, les entrées orange, les sorties vertes, une entrée `value` est une courbe.

![The timing diagram of the German traffic lights](pics/09-GermanTrafficLights-timing.png)

*Le feu tricolore allemand après 9 s : en bas, le chronogramme des quatre étapes et des sorties ; sous `3s/X0`, la temporisation de la transition compte `1.0 / 3 s` ; à droite de la console, les Outputs.*

La vitesse de la barre d'outils (x0.1 à x50) met aussi à l'échelle le temps GRAFCET : à x10, `30s/X1` dure 3 secondes réelles. **P** met la simulation en pause.

## 10. Tutoriel : votre premier GRAFCET

Nous dessinons une perceuse : l'opérateur appuie sur `Dcy`, la perceuse descend jusqu'au capteur `low`, attend 2 s en bas, remonte jusqu'à `high`, et allume une lampe 1 s à la fin.

1. *File > New > GRAFCET*.
2. **Clic gauche** dans le vide : une étape `0`. Clic gauche encore en dessous, quatre fois : étapes `1` à `4`. Dans un GRAFCET, un clic ajoute une étape.
3. **Glisser à gauche** de l'étape 0 à l'étape 1 : l'éditeur insère une transition entre deux étapes du même type (une liaison étape → étape n'est pas possible). Recommencez de 1 à 2, 2 à 3, 3 à 4, et de 4 vers 0 (la liaison remontante reçoit une flèche).
4. Mettez la souris sur l'étape 0 et appuyez sur **+** : elle devient l'étape initiale (double carré).
5. **Double clic** sur chaque transition et écrivez sa réceptivité : `↑Dcy` (ou `^Dcy`), `low`, `2s/X2`, `high`, `1s/X4`.
6. **Clic droit** sur l'étape 1 > *Add an action* ; dans le panneau *Actions* mettez le qualificateur `N` et le nom `Down`. Étape 3 : `N Up`. Étape 4 : `N Lamp` (ou `L Lamp` avec une durée de 1 s).
7. *Edit > Arrange top-down* aligne les étapes en colonne, comme on dessine un GRAFCET.
8. Appuyez sur **Space**. Le pupitre affiche `Dcy`, `low`, `high` en interrupteurs : sur le pupitre, *Edit* et faites de `Dcy` un `button`.
9. Appuyez sur `Dcy` : étape 1, la sortie `Down` est active. Activez `low` : étape 2, la temporisation `x / 2 s` compte sous `2s/X2`, puis étape 3. Désactivez `low` et activez `high` : étape 4, la lampe, puis retour à l'étape 0.
10. Ouvrez l'onglet *Timing diagram* et lisez tout le cycle. Enregistrez avec **Ctrl+S**.

Pour l'exécuter sur une vraie machine, générez son code C++ ([chapitre 12](12-Code-Generation.md)) ou appelez le moteur depuis votre programme ([chapitre 16](16-Library.md)).

## 11. À essayer

| Exemple | Que faire | Que observer |
|---------|------------|-----------------|
| [GermanTrafficLights](../../examples/GRAFCET/GermanTrafficLights/README.md) | appuyer sur Space | une séquence pilotée par le temps seul ; l'étape 3 a deux actions ; le chronogramme |
| [ExclusiveChoice](../../examples/GRAFCET/ExclusiveChoice/README.md) | Space, puis A avec et sans B | une divergence OU avec réceptivités exclusives |
| [ParallelSequences](../../examples/GRAFCET/ParallelSequences/README.md) | Space, `m`, puis les capteurs | une divergence ET, une boucle, une convergence ET |
| [GrafcetActions](../../examples/GRAFCET/GrafcetActions/README.md) | Space, `Start`, `s1`, `s0` ; `Error`, `Reset` | les qualificateurs N, D, S, R, L dans les Outputs et le chronogramme |
| [GEMMA-Dryer](../../examples/GRAFCET/GEMMA-Dryer/README.md) | voir [chapitre 10](10-GEMMA.md) | trois graphes, ordres de forçage, un pupitre avec tous les types d'entrées |

## 12. Pièges

- **Une divergence OU qui n'est pas exclusive.** Deux réceptivités vraies ensemble démarrent les deux branches (règle 4) : deux moteurs en sens opposés. Écrivez `b1` et `b2 . /b1`.
- **Un GRAFCET instable.** Une boucle de transitions dont les réceptivités sont vraies ensemble fait tourner le graphe à l'intérieur d'un cycle, indéfiniment. Ouvrez [GrafcetActions](../../examples/GRAFCET/GrafcetActions/README.md), appuyez sur Space et activez `Start`, `Error` et `Reset` ensemble : le graphe fait 0 → 1 (`Start`) → 4 (`Error`) → 0 (`Reset`) → 1 ... sans fin. Le moteur arrête la recherche après 256 évolutions à chaque cycle et avertit une fois dans la console (*"The GRAFCET does not reach a stable situation after 256 evolutions: is there a loop of transitions always true?"*). Exécuté par le moteur, trois cycles donnent :

  ```text
  cycle 0: 256 firings, steps X1  outputs: V1+
  cycle 1: 256 firings, steps X4  outputs: AlarmLamp Buzzer
  cycle 2: 256 firings, steps X0  outputs:
  ```

L'étape où la recherche s'arrête change à chaque cycle (256 n'est pas un multiple de 3), donc les sorties clignotent, et l'éditeur fait 256 franchissements par image : il utilise un cœur entier du processeur. Un vrai automate ferait pareil. Faites attendre autre chose à une réceptivité de chaque boucle : un front (`↑Start`), le niveau opposé (`/Start` au retour), ou un temps (`1s/X4`).
- **Un niveau là où il faut un front.** `Ack` sur deux transitions consécutives : une pression franchit les deux. `↑Ack` franchit une transition par pression.
- **Étapes transitoires et actions mémorisées.** L'éditeur calcule les actions uniquement à partir de la situation stable : une étape traversée pendant la recherche de stabilité n'exécute aucune de ses actions, pas même un `S` ou un `P1`. Ne comptez pas sur une étape transitoire pour activer une sortie.
- **Le forçage est lu au début du cycle.** Quand l'arrêt d'urgence active l'étape de sécurité, les graphes forcés prennent leur situation au cycle suivant : un cycle plus tard (voir l'exécution du [chapitre 10](10-GEMMA.md)).
- **Numéros d'étapes.** `X3` est l'étape dont l'*identifiant* est 3 (le grand numéro dans le carré), pas sa légende. Quand vous supprimez une étape, la dernière prend son numéro ; les réceptivités de tous les graphes (`X5`, `Production:X5`) suivent le nouveau numéro, et ses actions vont avec.

## 13. Pour aller plus loin

- La norme : IEC 60848:2013, *GRAFCET specification language for sequential function charts*.
- Cours français de la [bibliographie](Bibliography.md) : le cours GRAFCET d'O. Kamach, le cours ENSIAME de J.-C. Popieul, les vidéos *Programmation d'un Grafcet en C (pour Arduino)*.
- Le pupitre et son JSON : [chapitre 15](15-File-Formats.md) ; le moteur dans votre propre programme : [chapitre 16](16-Library.md) ; le code C++ pour une carte et son débogage : [chapitres 12](12-Code-Generation.md) et [13](13-Debugging.md).

---

Suivant : [10. GEMMA](10-GEMMA.md) — les modes de marche et d'arrêt d'une machine, avec plusieurs GRAFCET qui se forcent mutuellement.
