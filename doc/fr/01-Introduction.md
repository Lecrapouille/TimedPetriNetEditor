# 🧩 1. Introduction


[![fr](https://lecrapouille.github.io/icons/fr.png)](../fr/01-Introduction.md) [![en](https://lecrapouille.github.io/icons/en.png)](../en/01-Introduction.md)

**Ce que vous apprendrez**

- ce qu’est un système à événements discrets, et pourquoi on le dessine avec un réseau de Petri ;
- les familles de réseaux de l’éditeur (réseaux de Petri, réseaux de Petri étendus, réseaux de Petri temporisés, graphes d’événements temporisés, GRAFCET) et la question à laquelle chacune répond ;
- ce que l’éditeur propose pour chaque famille, et où la documentation l’explique ;
- l’histoire du projet, d’une boîte à outils (max,+) à un runtime.

**Prérequis** : aucun. Les mots en **gras** sont dans le [glossaire](Glossary.md).

---

## 1. 🏭 Systèmes à événements discrets

Regardez une machine à café. Elle attend une pièce ; une pièce tombe ; elle vérifie la pièce ; elle chauffe l’eau ; elle sert un café ; elle attend à nouveau. Son état ne varie pas en continu comme la température d’une pièce : il saute d’une situation à l’autre quand quelque chose se produit. Chacun de ces « quelque chose » est un **événement**, et un système dont l’état ne change qu’aux événements est un **système à événements discrets**.

La plupart des systèmes que construisent les ingénieurs sont de ce type :

| Système | Ses événements | Ce que l’on veut savoir |
|---------|----------------|-------------------------|
| une usine | une pièce entre dans une machine, une machine termine son travail | combien de pièces par heure ? quelle machine freine les autres ? |
| un carrefour | un feu passe au vert, une voiture passe | deux flux de voitures peuvent-ils se croiser en même temps ? |
| un centre d’appels | un appel arrive, un opérateur répond, raccroche | qui attend, et combien de temps ? |
| un robot | un objet est vu, saisi, déposé | que se passe-t-il si la pince rate ? |
| un programme informatique | un message arrive, une tâche se termine, un verrou est pris | peut-il rester bloqué pour toujours (un interblocage) ? |
| un réseau ferroviaire | un train part, arrive | quel horaire peut-il tenir ? |

Quatre phénomènes rendent ces systèmes difficiles à comprendre, et ils apparaissent dans chacun d’eux :

- **Concurrence** : plusieurs choses se produisent en même temps et indépendamment (deux machines travaillent en parallèle).
- **Synchronisation** : une chose attend plusieurs autres (un assemblage attend toutes ses pièces ; un café est servi seulement quand l’eau est chaude *et* la pièce acceptée).
- **Choix** (conflit) : une situation peut mener à plusieurs futurs (la pièce est bonne ou fausse ; l’appelant veut un conseil ou une ambulance).
- **Ressources partagées** : plusieurs acteurs utilisent quelque chose en nombre limité (opérateurs, fourchettes, un bras robot, un tampon mémoire).

Un modèle de système à événements discrets doit montrer clairement ces quatre aspects, et permettre de calculer des réponses aux questions du tableau.

## 2. 🔵 Pourquoi les réseaux de Petri

Pourquoi pas une machine à états (un automate), le dessin que tout le monde connaît, avec des cercles pour les états et des flèches pour les événements ? Parce qu’un automate n’a qu’**un** état courant. Deux machines de 3 états chacune, travaillant en parallèle, exigent un automate de 3 × 3 = 9 états ; dix machines en exigent 3¹⁰ = 59049. C’est l’**explosion d’états** : le dessin croît exponentiellement avec le nombre de parties du système, et personne ne peut plus le lire.

En 1962, dans sa thèse *Kommunikation mit Automaten*, **Carl Adam Petri** a proposé un autre dessin, où l’état est **distribué** :

```text
   place          transition        arc            token
   (a condition,  (an event)        (who feeds     (a resource, a part,
    a stock)                         whom)          "this is true")
     ( )             ▭               ──►              ●
```

- une **place** (cercle) est une condition locale ou un stock : « la machine est libre », « pièces en attente » ;
- un **jeton** (point) dans une place indique que la condition est vraie, ou compte les éléments du stock ;
- une **transition** (barre) est un événement ; elle peut se produire quand chaque place en amont contient un jeton, et quand elle se produit elle **tire** : elle retire un jeton de chaque place en amont et en met un dans chaque place en aval.

L’état de tout le système, le **marquage**, est le nombre de jetons dans chaque place. Dix machines de 3 états deviennent dix petites boucles de 3 places, 30 places au total, côte à côte : le dessin croît comme le système, pas comme le nombre de ses états. Et les quatre phénomènes prennent une forme simple :

```text
 concurrency        synchronization      choice              shared resource
   ┌─► ▭ ─►            ─► ( ) ─┐           ┌─► ▭ ─►            ▭ ◄── (●) ──► ▭
  ( )                          ▭ ─►       (●)                   │ mutex ▲      │
   └─► ▭ ─►            ─► ( ) ─┘           └─► ▭ ─►            └───────┴──────┘
 two events that     an event that       two events that     two users of one
 can happen in       waits for two       want the same       token, given back
 any order           conditions          token               after use
```

Un réseau de Petri est à la fois un **dessin** que l’on peut discuter et un **objet mathématique** : deux matrices (ce que chaque transition prend et donne) et un vecteur (le marquage initial). À partir de là, des algorithmes répondent aux questions du tableau : le réseau peut-il se bloquer ? une place peut-elle croître sans limite ? quels marquages sont atteignables ? Avec le temps, à quelle vitesse va-t-il ? Ce sont les sujets des chapitres suivants.

![Une machine à café en réseau de Petri](pics/03-Editor-Coffee.png)

*Une machine à café en réseau de Petri dans l’éditeur ([examples/Petri-Nets/Coffee](../../examples/Petri-Nets/Coffee/README.md)) : les transitions vertes peuvent tirer.*

## 3. 🧭 Les familles de réseaux de l’éditeur

Les réseaux de Petri ont de nombreuses variantes. L’éditeur propose celles qui répondent aux questions des ingénieurs, chacune ajoutant quelque chose à la précédente :

```text
                        ┌──────────────── extended Petri nets ────────────────┐
                        │  weights, capacities, inhibitor and reset arcs,     │
                        │  priorities, colored tokens                         │
                        │   ┌──────────────── Petri nets ─────────────────┐   │
                        │   │  places, transitions, tokens, firing rule   │   │
                        │   │   ┌───────── event graphs ──────────┐       │   │
                        │   │   │ each place: one transition in,  │       │   │
                        │   │   │ one transition out: no choice   │       │   │
                        │   │   └─────────────────────────────────┘       │   │
                        │   └─────────────────────────────────────────────┘   │
                        └─────────────────────────────────────────────────────┘
   + time (durations, delays)   →  timed Petri nets, timed event graphs + (max,+)
   + inputs, outputs, conditions →  GRAFCET (one token per step at most), GEMMA
   + functions of a program      →  the runtime
```

| Famille | Elle ajoute | La question à laquelle elle répond | Chapitres |
|---------|-------------|-------------------------------------|-----------|
| **Réseau de Petri** | la règle de tir | Quelles situations peuvent se produire ? Peut-il se bloquer ? Une ressource n’est-elle jamais utilisée deux fois ? | [3](03-Petri-Nets.md), [4](04-Analysis.md) |
| **Réseau de Petri étendu** | poids, capacités, **arcs inhibiteurs** (tirer seulement si une place est vide), **arcs de remise à zéro** (vider une place), priorités, **jetons colorés** | La même chose, pour les systèmes qui doivent compter, tester le zéro, vider un stock, choisir selon un résultat | [5](05-Extended-Petri-Nets.md) |
| **Réseau de Petri temporisé** | **durées** sur les arcs, **délais** sur les transitions | Quand chaque événement se produit-il ? Que fait un timeout ? | [6](06-Timed-Petri-Nets.md) |
| **Graphe d’événements temporisé** | un réseau temporisé sans choix | Quelle vitesse maximale (le **temps de cycle**) ? Quel est le goulot (**circuit critique**) ? Quel est l’ordonnancement (diagramme de Gantt) ? Répondu exactement par l’**algèbre (max,+)** | [7](07-Timed-Event-Graphs.md), [8](08-Max-Plus-Algebra.md) |
| **GRAFCET** | étapes avec **actions**, transitions avec **réceptivités** sur entrées et temps (IEC 60848) | Comment l’automate d’une machine réagit-il à ses boutons et capteurs ? | [9](09-GRAFCET.md) |
| **GEMMA** | plusieurs GRAFCET qui se forcent mutuellement | Comment une machine démarre, s’arrête et se remet après une panne ? | [10](10-GEMMA.md) |
| **Runtime** | une fonction de votre programme par transition | Comment faire suivre mon programme au réseau que j’ai dessiné ? | [11](11-Runtime.md), [12](12-Code-Generation.md), [13](13-Debugging.md) |

Vous choisissez le type d’un réseau dans la barre d’outils de l’éditeur (ou avec *Edit > Type of net*) ; les extensions se règlent sur les arcs et les places d’un réseau de Petri ou d’un réseau de Petri temporisé.

## 4. 🛠️ Ce que propose l’éditeur

| | Pour chaque famille | Réseaux de Petri et extensions | Réseaux temporisés | Graphes d’événements temporisés | GRAFCET, GEMMA | Programmes |
|-|-|-|-|-|-|-|
| **Dessiner** | la souris seule : le bouton gauche dessine, le droit déplace ; annuler, copier, coller, dispositions | poids, types d’arcs, capacités, couleurs au clic droit | durées, délais | le dessin compact (places sur les arcs), la forme canonique | réceptivités vérifiées à la frappe, actions, ordres de forçage | |
| **Exécuter** | jetons qui se déplacent sur le réseau, vitesse x0,1 à x50, un journal des tirs | clic sur une transition pour la tirer | le temps avance, conflits par priorité ou au hasard | idem, comparé à la théorie | un pupitre avec boutons, sélecteurs, capteurs ; sorties ; un chronogramme | le runtime appelle vos fonctions |
| **Analyser** | | interblocages et chemin jusqu’à eux, bornes, vivacité, P- et T-invariants, `tpne-check` pour une CI | tracé des dates de tir | temps de cycle, circuit critique, diagramme de Gantt, équations et matrices (max,+) | | |
| **Exporter** | JSON, PNML, GRML, SVG, LaTeX, Graphviz, draw.io ... | | | Julia (MaxPlus.jl), `.teg` | code C++ pour un Arduino | squelette d’un programme C++, journaux pour l’extraction de processus |
| **Déboguer** | | | | | le code généré suivi en direct | l’éditeur suit votre programme en cours |

Trois vues de l’éditeur, produites par l’éditeur lui-même :

![Un centre d’appels en marche](pics/01-Editor-EmergencyCalls.png)

*Un centre d’appels d’urgence (réseau de Petri à jetons colorés, [examples/Petri-Nets/EmergencyCalls](../../examples/Petri-Nets/EmergencyCalls/README.md)).*

![Un flowshop et son diagramme de Gantt](pics/01-Editor-Flowshop.png)

*Une usine en graphe d’événements temporisé : son circuit critique en rouge, son ordonnancement en diagramme de Gantt ([chapitre 7](07-Timed-Event-Graphs.md)).*

![Un séchoir piloté par une GEMMA](pics/01-Editor-GEMMA.png)

*Un séchoir piloté par trois GRAFCET (une GEMMA) : le pupitre et les sorties à droite et en bas ([chapitre 10](10-GEMMA.md)).*

## 5. 📜 L’histoire du projet

**L’algèbre (max,+).** Dans les années 1980, à l’INRIA, un groupe de chercheurs (Guy Cohen, Didier Dubois, Jean-Pierre Quadrat, Michel Viot, puis le groupe de travail *Max-Plus*) a montré que les dates des événements d’un graphe d’événements temporisé obéissent à des équations *linéaires*, à condition de remplacer l’addition par `max` et la multiplication par `+`. L’ordonnancement d’une usine ou d’un réseau ferroviaire devenait un problème d’algèbre linéaire ; sa vitesse, une valeur propre. L’ouvrage *Synchronization and Linearity* (Baccelli, Cohen, Olsder, Quadrat, 1992) rassemble cette théorie, et une boîte à outils (max,+) pour Scilab, puis ScicosLab, la calculait, avec l’algorithme de Howard pour la valeur propre.

**MaxPlus.jl.** ScicosLab n’est plus développé. L’auteur de cet éditeur, fils de Jean-Pierre Quadrat, a porté la boîte à outils de son père vers Julia : [MaxPlus.jl](https://github.com/Lecrapouille/MaxPlus.jl). Il manquait dans les deux outils : il fallait saisir les matrices à la main.

**Un éditeur de graphes d’événements temporisés (2021).** Cet éditeur a commencé en juin 2021 comme la moitié graphique manquante de la boîte à outils : dessiner le réseau à la souris, et les matrices, équations, temps de cycle et circuit critique viennent tout seuls. L’algorithme de Howard de ScicosLab a été intégré (juillet 2021), l’export Julia, puis les flowshops (une usine décrite comme un tableau de temps, transformée en graphe d’événements temporisé) et leurs diagrammes de Gantt. Les éditeurs de réseaux de Petri trouvés sur GitHub n’aidaient pas : la plupart étaient abandonnés, exigeaient Windows, Java ou C#, ou ne se prêtaient pas à être étendus.

**GRAFCET (2021 – 2026).** L’auteur est ingénieur, et les ingénieurs en automatisme dessinent des GRAFCET. Deux semaines après le premier commit, après une vidéo où un GRAFCET est codé à la main en C pour un Arduino, l’éditeur générait déjà le code C++ d’un GRAFCET. Des années plus tard, c’est devenu un GRAFCET complet selon la norme IEC 60848 (arcs, temporisations, tous les qualificateurs d’action, ordres de forçage entre graphes), une GEMMA à plusieurs graphes, un pupitre avec de vrais boutons et un chronogramme, et du code C++ vérifié contre le moteur de l’éditeur.

**Réseaux de Petri étendus et runtime (2026).** Les réseaux de Petri ont ensuite reçu leurs extensions classiques : poids, capacités, arcs inhibiteurs et de remise à zéro, priorités, jetons colorés, et l’analyse de leurs propriétés (interblocages, bornes, vivacité, invariants). En dernier est venue l’idée de [Symmetri](https://github.com/thorstink/Symmetri) : un réseau dessiné dans l’éditeur exécute un programme, chaque transition appelant une fonction, le résultat de la fonction choisissant la suite. L’éditeur est devenu l’IDE de ce runtime : il écrit le squelette du programme, et il suit le programme pendant son exécution.

Les projets qui l’utilisent aujourd’hui incluent [RobotIK](https://github.com/Lecrapouille/Robotik), qui pilote des cellules robot simulées avec le moteur GRAFCET et les algorithmes (max,+).

## 6. 📖 Comment lire cette documentation

- Chaque chapitre d’apprentissage va **d’abord la théorie** (intuition, définition, exemple calculé à la main avec chaque nombre), puis **dans l’éditeur** (comment l’éditeur montre et résout le problème), puis **essayez** (les exemples à ouvrir).
- Les exemples sont dans [`examples/`](../../examples/README.md), un dossier par exemple, regroupés par famille ; chacun a une image, un README et souvent une description affichée sur le canevas au-dessus du réseau.
- Les menus s’écrivent *File > Open*, les touches **Space**, les fichiers [comme ceci](../../examples/README.md).
- Chaque sortie et chaque image provient d’une exécution réelle : vous pouvez tout reproduire (`tools/screenshots.sh` régénère les images).

---

Suivant : [2. Premiers pas](02-Getting-Started.md) — compiler l’éditeur et dessiner votre premier réseau.
