# 🖱️ 14. Référence de l'éditeur


[![fr](https://lecrapouille.github.io/icons/fr.png)](../fr/14-Editor-Reference.md) [![en](https://lecrapouille.github.io/icons/en.png)](../en/14-Editor-Reference.md)

Ce que vous trouverez ici : chaque fenêtre, geste de souris, touche, menu et option de ligne de commande de l'éditeur, vérifiés dans le code. Les chapitres d'apprentissage les montrent en situation : [2](02-Getting-Started.md) (premier réseau), [3](03-Petri-Nets.md) (simulation), [4](04-Analysis.md) (propriétés), [7](07-Timed-Event-Graphs.md) et [8](08-Max-Plus-Algebra.md) (l'analyse (max,+)), [9](09-GRAFCET.md) (GRAFCET), [13](13-Debugging.md) (journal et débogueur). Dans l'éditeur, **F1** ouvre la même aide.

---

## 1. La fenêtre principale

![The editor with a Petri net](pics/14-Window-Petri.png)

*`examples/Petri-Nets/Philosophers/Philosophers.json` vient d'être ouvert (fenêtre 1400 × 900 pixels).*

De haut en bas et de gauche à droite :

| Zone | Rôle |
|------|------------|
| Barre de menus | *File, Edit, View, Simulation, Analysis, Debug, Help* (§9) |
| Barre d'outils (fin de la barre de menus) | **▶ Run** / **■ Stop**, *Pause*, la vitesse `x1`, le type du réseau actif, *Fit*, *Grid*, *Snap* |
| Fenêtre réseau (au centre, onglet *Philosophers*) | le dessin d'un réseau. En haut à gauche : son type, pendant la simulation `simulating t = 3.0 s` (et *paused*), ensuite `ended at t = 12.0 s:` et pourquoi, puis sa description. En bas à droite : le zoom |
| Inspecteur (à droite) | onglets *Places* (*Steps* pour un GRAFCET), *Transitions*, *Arcs* (*Actions* pour un GRAFCET), *Colors* (réseaux de Petri et temporisés) |
| Sous l'inspecteur | *Control panel* (GRAFCET), *Analysis*, *Program* (lorsqu'un programme est connecté) |
| Bas | *Document*, *Console*, *Journal*, *Timing diagram* (GRAFCET) ; *Outputs* (GRAFCET) à droite |
| Barre d'état | à gauche, ce que feraient les boutons de la souris maintenant ; à droite, le nombre de places, transitions et arcs, la position de la souris dans le réseau et le zoom |

Un document GRAFCET de trois réseaux pendant l'exécution :

![The editor with a GEMMA running](pics/14-Window-GRAFCET.png)

*`examples/GRAFCET/GEMMA-Dryer/GEMMA-Dryer.json`, 3 s après **Space** : les trois réseaux en onglets, le premier devant avec un cadre vert (simulation), le panneau de commande (arrêt d'urgence, boutons poussoirs, sélecteur, capteur) sous l'inspecteur, les sorties (deux moteurs, un four) en bas à droite.*

Les fenêtres peuvent être déplacées, ancrées ailleurs, mises en onglets ou fermées. La disposition et le thème sont enregistrés dans le dossier de configuration de l'utilisateur :

| Système | Dossier de configuration |
|--------|-----------------|
| Linux | `$XDG_CONFIG_HOME/TimedPetriNetEditor`, sinon `~/.config/TimedPetriNetEditor` |
| macOS | `~/Library/Application Support/TimedPetriNetEditor` |
| Windows | `%APPDATA%\TimedPetriNetEditor` |

La disposition est gardée d'un document à l'autre : ouvrir un fichier ne déplace jamais les fenêtres. *View > Reset the layout of the windows* reconstruit la disposition par défaut. *Help > Mouse and keys*, onglet *Paths*, montre les dossiers utilisés.

## 2. Fenêtres

| Fenêtre | Affiche | Quand |
|--------|-------|------|
| Net (une par réseau du document) | le dessin ; les réseaux d'un document sont des onglets du même panneau, le premier devant (glisser un onglet met deux réseaux côte à côte : ils gardent leur part de largeur quand la fenêtre principale change de taille) | toujours |
| Places / Steps | légende, identifiant, `◀ ▶` et le nombre de jetons de chaque place, et ses jetons initiaux ; pour un GRAFCET en marche, depuis combien de temps chaque étape active l'est. Un réseau avec couleurs a aussi *Color of the tokens*, la couleur ajoutée ou retirée par `◀ ▶`, par **+** et **-** et par le menu d'une place ; survoler le nombre liste les jetons par couleur ; *colors* ouvre le nombre de jetons de chaque couleur, modifiable | toujours |
| Transitions | légende (la réceptivité d'un GRAFCET, vérifiée à la frappe, avec *Syntax help*) ; priorité (réseaux de Petri et temporisés) ; délai en secondes (GRAFCET) ; les minuteries qui comptent pendant la simulation | toujours |
| Arcs | un tableau : arc, type (normal, inhibitor, reset), poids, couleur des jetons, durée (réseaux temporisés) | réseaux de Petri et temporisés |
| Colors | les couleurs de jetons de l'utilisateur (*Add*) ; *Success*, *Failed* et *Canceled* sont intégrées | réseaux de Petri et temporisés |
| Actions | actions de chaque étape : qualificateur (avec son diagramme temporel), nom, condition, durée, ordres de forçage ; *Qualifiers help* | GRAFCET |
| Document | l'objet du document et la description de chaque réseau (*Edit* pour les modifier) ; les réseaux du document (un bouton radio choisit l'actif) ; les entrées et sorties GRAFCET | toujours |
| Control panel | les entrées des GRAFCET dessinées comme les boutons d'une machine ; *Apply together*, *Apply (F7)*, *Edit*, *Fit*, `?` (§8) | GRAFCET |
| Outputs | les actionneurs nommés par les actions : lampes, moteurs, résistances chauffantes, vannes, sirènes, buzzers, minuteries | GRAFCET |
| Timing diagram | étapes, entrées et sorties dans le temps (§8) | GRAFCET |
| Analysis | une rangée d'onglets : *Summary*, *Bounds*, *Invariants*, *Report* pour chaque réseau ; pour un graphe d'événements temporisé aussi *Critical circuit*, *Gantt*, *Linear system*, *Equations*, *Adjacency*, *Firing dates* (§7), pour un autre réseau temporisé *(max,+)*, qui explique pourquoi l'algèbre ne s'applique pas | fermée par sa croix, et à l'ouverture d'un document fait seulement de GRAFCET (elle ne tient pas compte de leurs règles : le panneau de commande est devant à la place) ; le menu *Analysis* l'ouvre avec un onglet au premier plan |
| Console | tous les messages (§6) | toujours |
| Journal | les tirs de la simulation, d'un journal ouvert ou d'un programme connecté ; les rejoue sur le réseau (§7) | toujours |
| Program | le programme suivi : son réseau, ses transitions sans fonction, ses entrées et sorties ([chapitre 13](13-Debugging.md)) | connecté (*Debug*) |

Un réseau est ajusté à sa fenêtre, de nouveau à chaque changement de taille de la fenêtre, jusqu'à ce que vous déplaciez ou zoomiez la vue (**F** l'ajuste à nouveau).

## 3. Souris

Il n'y a pas de mode à choisir. L'action d'un bouton dépend de ce qui est sous la souris, et de si la souris bouge avant le relâchement (moins de 4 pixels : un clic). **Le bouton gauche dessine, le bouton droit déplace.** La barre d'état indique toujours ce que feraient les boutons maintenant.

| Sous la souris | Clic | Glisser |
|-----------------|-------|------|
| rien, bouton gauche | ajouter une place (graphe d'événements temporisé : une transition) ; avec une sélection, le premier clic ne fait que l'effacer | sélectionner dans un rectangle |
| rien, bouton droit | ajouter une transition | déplacer la vue |
| nœud, bouton gauche | le sélectionner (Ctrl : l'ajouter ou le retirer de la sélection) | tracer un arc depuis lui |
| nœud, bouton droit | le menu du nœud | le déplacer, avec la sélection |
| arc, bouton gauche | le sélectionner | sélectionner dans un rectangle |
| arc, bouton droit | le menu de l'arc | déplacer la vue |
| n'importe quoi, bouton du milieu | | déplacer la vue |

- Un arc relâché dans le vide crée un nœud de l'autre type à cet endroit, déjà relié ; entre deux nœuds du même type, un nœud de l'autre type est inséré au milieu.
- **Double clic** sur un nœud : le renommer (transition GRAFCET : sa réceptivité, vérifiée à la frappe). Sur un arc : sa durée (réseaux temporisés) et son poids, et les jetons de sa place dans un graphe d'événements temporisé. Entrée ou *OK* valide (les boutons **+** et **-** ne font que changer la valeur), Échap annule.
- **Survoler** un arc d'un graphe d'événements temporisé montre ses deux transitions, sa durée et les jetons de sa place cachée.
- **Molette** : zoom autour de la souris. Maj + molette, ou pavé tactile : déplacer la vue.
- Pendant la simulation d'un réseau de Petri, un clic gauche sur une transition activée la fait tirer. Les réseaux temporisés et les GRAFCET avancent tout seuls.
- Avec plusieurs réseaux dans un document, un clic dans un réseau le rend actif (l'inspecteur l'affiche) et agit tout de suite.
- Les nœuds avec un halo sont marqués : par le *Journal* (transitions dont la fonction s'exécute), par un programme connecté (transitions sans fonction), ou par *Analysis* (transitions mortes, non vivantes, places non bornées, nœuds d'un invariant). *Debug > Clear the marks of the nodes* enlève les marques.

### Menu d'un nœud (clic droit)

| Entrée | Nœud | Effet |
|------|------|--------|
| Rename | all | comme un double clic |
| Select | all | le sélectionner |
| Add a token (+), Remove a token (-), Tokens | place | ses jetons ; avec couleurs, les jetons de la couleur choisie dans *Places* |
| Tokens by color | place d'un réseau avec couleurs | le nombre de jetons de chaque couleur, et la couleur utilisée par **+** et **-** |
| Priority | transition (pas GRAFCET) | sa priorité, pour la politique *Highest priority* |
| Result in the simulation | transition d'un réseau avec couleurs | la couleur que donnent en simulation ses arcs de sortie sans couleur : *random* parmi les couleurs demandées en aval (par défaut), ou toujours une couleur ; enregistré dans le fichier (`"result"`), non utilisé par le runtime |
| Max tokens | place (réseaux de Petri et temporisés) | cocher pour une capacité ; décoché : non bornée |
| Activate step, Deactivate step | étape GRAFCET | son jeton initial |
| Add an action | étape GRAFCET | une nouvelle action dans la fenêtre *Actions* |
| Delete | all | le supprimer et ses arcs |

### Menu d'un arc (clic droit)

| Entrée | Effet |
|------|--------|
| Duration and weight... (timed nets), Weight... | les nombres de l'arc |
| Normal, Inhibitor, Reset | son type (arcs inhibitor et reset d'une place vers une transition, réseaux de Petri et temporisés seulement, [chapitre 5](05-Extended-Petri-Nets.md)) |
| Color of the tokens | la couleur que la transition prend (arc depuis une place) ou donne (arc vers une place) ; *any*, couleurs intégrées, couleurs du réseau, ou nouvelle couleur saisie dans le champ |
| Select, Delete | |

## 4. Touches

Les touches agissent sur la fenêtre réseau qui a le focus.

| Touche | Action | En simulation |
|-----|--------|:----------------:|
| **Space** | démarrer ou arrêter la simulation | oui |
| **P** | mettre en pause ou reprendre la simulation | oui |
| **F** | ajuster le réseau à sa fenêtre | oui |
| **Esc** | annuler le glissement en cours, sinon effacer la sélection | oui |
| **+** / **-** (aussi pavé numérique, **=**) | jetons de la place sous la souris | oui |
| **G** | aligner les nœuds sur la grille (on / off) | |
| **L** | disposition ressort (encore pour arrêter) | |
| **Delete**, **Backspace** | supprimer la sélection, sinon ce qui est sous la souris | |
| Flèches (**Shift** : 1 pixel) | déplacer la sélection (d'un demi-pas de grille) | |
| **Ctrl+Z**, **Ctrl+Y** (**Ctrl+Shift+Z**) | annuler, refaire (déplacements inclus) | |
| **Ctrl+C**, **Ctrl+X**, **Ctrl+V**, **Ctrl+A** | copier, couper, coller à la souris, tout sélectionner | |
| **Ctrl+N** | vider le document | oui |
| **Ctrl+O**, **Ctrl+S**, **Ctrl+Shift+S**, **Ctrl+Q** | ouvrir, enregistrer, enregistrer sous, quitter | oui |
| **Ctrl+=**, **Ctrl+-**, **Ctrl+0** | zoom avant, zoom arrière, ajuster | oui |
| **F7** | GRAFCET : appliquer les entrées modifiées ensemble | oui |
| **F1** | aide | oui |

## 5. Simulation

| Commande | Où | Effet |
|---------|-------|--------|
| Run / Stop | barre d'outils, *Simulation > Start*, **Space** | à l'arrêt, le réseau revient à son marquage initial |
| Pause / Resume | barre d'outils, *Simulation > Pause*, **P** | le temps s'arrête |
| Vitesses x0.1, x0.25, x0.5, x1, x2, x5, x10, x20, x50 | barre d'outils, *Simulation > Speed* | à x10, une minuterie GRAFCET `30s/X1` dure 3 s et les jetons bougent 10 fois plus vite |
| Conflicts: Random, Highest priority, First declared | *Simulation > Conflicts* (pas pour un GRAFCET) | quelle transition tire en premier lorsque plusieurs le peuvent ; avec *Random*, un *Seed* (0 : nouveau à chaque fois, écrit dans la console) et *Replay the last simulation* |
| Start all the nets, the nets of the document | *Simulation* (plusieurs réseaux) | les exécuter ensemble ; choisir le réseau actif |

La politique de conflit est un réglage de l'éditeur, non enregistré dans le fichier. Ouvrir, enregistrer ou exporter un fichier pendant la simulation arrête d'abord la simulation (un message l'indique). Pendant la simulation, le réseau n'est pas modifiable : seuls les jetons (**+** / **-**) et les entrées GRAFCET changent.

## 6. Console

Un bouton par type de message, avec son compteur : *Info*, *Warnings*, *Errors*, *Simulation* (entrées GRAFCET modifiées en marche ; les tirs sont dans le *Journal*). Un clic les masque ou les affiche. Le *Filter* garde les messages contenant un texte ; *Copy* met les messages affichés dans le presse-papiers ; *Clear* vide la console. Le même message plusieurs fois de suite n'est écrit qu'une fois avec son compteur `(x3)`. Les trois derniers messages apparaissent aussi en bas à droite des réseaux, encadrés par la couleur de leur type (bleu info, orange avertissement, rouge erreur) : 6 secondes pour une info, 12 pour un avertissement ou une erreur. La souris dessus le garde affiché, un clic le ferme. Ils n'apparaissent jamais sur les images prises avec `-s` et `-w`.

La fin d'une simulation indique pourquoi elle s'est arrêtée, avec la date : *stopped by the user*, ou pour un réseau temporisé *deadlock, no transition can fire any more* (avertissement). Le coin supérieur gauche du réseau le conserve (`ended at t = 12.0 s: deadlock ...`) jusqu'à modification du réseau.

## 7. Journal et Analysis

![Journal and Analysis](pics/14-Window-Journal.png)

*`examples/Timed-Petri-Nets/Durations/Durations.json` après 4 s : le jeton de `Eat` voyage pendant 2 unités de temps ; le Journal liste les tirs.*

Le **Journal** liste les tirs : *Date*, *Instance* (le réseau, ou une instance de programme), *Transition*, son numéro *k*, l'*Event* (`Scheduled` : les jetons sont pris, `Started` : sa fonction s'exécute, `Completed` : les jetons sont donnés) et le *Result* (couleur des jetons donnés). Il contient les tirs de la dernière simulation, d'un journal ouvert (*Open...* : JSON lines), le chemin vers un deadlock trouvé par l'analyse, ou ce qu'un programme connecté publie. *Follow* garde la dernière ligne visible, *Filter* garde les lignes contenant un texte, *Save...* écrit JSON lines, CSV ou XES (selon l'extension, [chapitre 15](15-File-Formats.md#5-journals)), *Clear* le vide. **Replay** montre le marquage du réseau à une date : le curseur, `<` et `>` passent d'un événement au suivant, les flèches au premier et au dernier ; les transitions dont la fonction s'exécute sont marquées. Un clic sur une ligne va à sa date.

![Properties](pics/14-Window-Analysis.png)

*`examples/Petri-Nets/MutualExclusion/MutualExclusion.json` avec `-f Properties`.*

La fenêtre **Analysis**, onglets **Summary**, **Bounds**, **Invariants** et **Report**, pour chaque réseau (durées et règles GRAFCET non prises en compte, [chapitre 4](04-Analysis.md)). Chaque onglet commence par *Compute again*, qui explore les marquages jusqu'à *max markings* (petits réseaux : à nouveau à chaque changement). *Summary* indique combien de marquages, si le réseau peut deadlocker, a des transitions mortes, est borné et vivant, puis donne le deadlock avec son marquage et le plus court chemin (*Replay the path in the Journal*). *Bounds* donne la borne de chaque place, *Invariants* les P- et T-invariants (un clic marque leurs nœuds), *Report* le texte de `tpne-check` (*Copy the report*). *Show on the net* marque les transitions mortes, non vivantes et les places non bornées. *Graph of the markings* dessine le graphe de marquages dans sa propre fenêtre (disposition ressort, glisser et molette, *Layout again*, *Export to dot...* ; `-f Markings` l'ouvre en ligne de commande). La même chose en terminal ou en CI :

```text
tpne-check [--max-states N] [--allow-deadlock] [--dot graph.dot] net.json ...
```

(code de sortie 1 lorsqu'un réseau peut deadlocker).

L'onglet **(max,+) algebra** est là pour chaque réseau temporisé, avec les onglets ci-dessous ([chapitres 7](07-Timed-Event-Graphs.md) et [8](08-Max-Plus-Algebra.md)). Ils exigent un graphe d'événements temporisé : pour un autre réseau temporisé, l'onglet dit pourquoi ce n'en est pas un, et *Show the faulty arcs* dessine les arcs à modifier. Les résultats sont ceux des jetons initiaux, y compris pendant la simulation.

| Onglet | Affiche |
|-----|-------|
| Critical circuit | le temps de cycle λ, les arcs du circuit (aussi dessinés sur le réseau), le biais de chaque transition |
| Gantt | les dates au plus tôt des premiers cycles, une ligne par machine (curseur *Cycles*) |
| Linear system | les matrices `D`, `A`, `B`, `C` de `X(k) = D X(k) ⊕ A X(k-1) ⊕ B U(k)`, `Y(k) = C X(k)` |
| Equations | équations dater ((max,+)) et compteur ((min,+)), avec légendes ou identifiants |
| Adjacency | les matrices des *Durations* et des *Tokens* |
| Firing dates | les dates `y(k)` de chaque transition pendant la simulation, avec *Subtract drift λ·k* ; pour un flowshop, *Conveyors only* garde les sorties `out_<conveyor>` (une pièce quitte l'usine), sinon *Outputs only* garde les transitions `out_*` |

Chaque matrice a un bouton *Copy as Julia* ([chapitre 17](17-Julia.md)).

## 8. Fenêtres GRAFCET

- **Control panel** : un widget par entrée, selon le type ([chapitre 9](09-GRAFCET.md)). À l'arrêt, un clic change la valeur initiale (enregistrée avec le document) ; en marche, agit tout de suite, ou à *Apply (F7)* si *Apply together* est coché. Un clic droit maintient un bouton poussoir enfoncé. *Fit* dessine les boutons plus petits pour tous les voir ; chaque section (*Safety*, *Buttons*, *Selectors*, *Sensors* ...) se replie. *Edit* affiche deux tableaux, *Inputs* (nom, type, réglages, description) et *Outputs* (nom, type, description). `?` ouvre l'aide du panneau.
- **Outputs** : les actionneurs nommés par les actions, allumés ou éteints.
- **Timing diagram** : *Steps*, *Inputs*, *Outputs* choisissent les lignes ; *Only what changed* masque les étapes jamais actives et les signaux toujours à 0 ; le curseur fixe la plage affichée ; *Follow* montre les dernières secondes, sinon une ligne de temps choisit la plage. La souris donne l'instant et la valeur.
- **Actions** : un qualificateur montre sa signification et son diagramme temporel ; *Qualifiers help* les ouvre tous. *Forcing orders* : *Add order*, *Remove*.
- Supprimer une étape donne son numéro à la dernière étape : ses actions la suivent, et les réceptivités de tous les graphes (`X3`, `Production:X3`) s'ajustent. Ce que lisait l'étape supprimée lit une étape manquante (erreur au démarrage).

## 9. Menus

| Menu | Entrée | Effet |
|------|------|--------|
| **File** | New > Petri net, Timed Petri net, Timed event graph, GRAFCET | un nouveau document de ce type |
| | New > GEMMA... | un nouveau document fait à partir d'une petite GEMMA : ses GRAFCET Safety, Control, Production, et les deux fenêtres GEMMA ([chapitre 10 §5](10-GEMMA.md#5-make-your-own-gemma-in-the-editor)) |
| | New > Flowshop... | une fenêtre pour remplir le tableau d'un flowshop : noms et unités des machines et des convoyeurs, temps de traitement ; *From the active net* le remplit avec le flowshop du réseau actif ; *Create* construit son graphe d'événements temporisé dans un nouveau document ([chapitre 15, §4.3](15-File-Formats.md#43-flowshop)) |
| | Clear the document (Ctrl+N) | vider le document actif |
| | Open... (Ctrl+O), Open in a new document... | un fichier `.json`, `.flowshop`, `.pnml`, `.grml`, `.teg` |
| | Import > *format* | lire un fichier de ce format ([chapitre 15](15-File-Formats.md)) |
| | Save (Ctrl+S), Save as... (Ctrl+Shift+S) | le fichier JSON |
| | Export > *format* | écrire le réseau dans ce format (seuls les formats qui peuvent l'écrire sont listés) |
| | Screenshot... | une image PNG du réseau |
| | Documents > *names*, Close this document | plusieurs documents ouverts |
| | Quit (Ctrl+Q) | propose d'enregistrer un document modifié |
| **Edit** | Undo (Ctrl+Z), Redo (Ctrl+Y) | |
| | Type of net > Petri net, Timed Petri net, Timed event graph, GRAFCET | changer le type du réseau actif (refusé si le réseau utilise ce que le nouveau type n'autorise pas : les arcs fautifs sont montrés) |
| | Add a net to the document > Petri net, GRAFCET | un second réseau dans le même document |
| | GEMMA editor... | la fenêtre pour construire la GEMMA du document actif : les procédures gardées, les liaisons et leurs conditions (qui partent de la procédure sélectionnée : vert, qui y arrivent : bleu), les vérifications, *Make the GRAFCETs*. Ancrée à gauche des réseaux, sur toute la hauteur ([chapitre 10 §5.1](10-GEMMA.md#51-les-deux-fenêtres-gemma)) |
| | GEMMA graph... | la GEMMA dessinée comme son formulaire ([GEMMA.drawio](../gemma/GEMMA.drawio)), en lecture seule, ancrée entre l'éditeur de GEMMA et les réseaux (ouverte avec un document de GRAFCET) : l'étape `Xn` de chaque procédure, les conditions sur les flèches |
| | To canonical form | un graphe d'événements temporisé dont les places ont au plus un jeton ([chapitre 8](08-Max-Plus-Algebra.md)) |
| | Arrange top-down | couches depuis les étapes initiales, comme un GRAFCET se dessine |
| | Spring layout (L), Stop the spring layout | placement par forces |
| | Align to the grid | aligner tous les nœuds sur la grille |
| | Clear the net | supprimer tous ses nœuds |
| **View** | Fit the net (F), Zoom in (Ctrl+=), Zoom out (Ctrl+-) | |
| | Grid, Snap to the grid (G) | |
| | Place identifiers, Transition identifiers | `P0`, `T1` à côté des légendes ; GRAFCET : numéros des étapes, `(1)` à gauche des transitions |
| | GRAFCET action boxes | masquées : une petite étiquette avec le nombre d'actions d'une étape (verte quand une s'exécute) |
| | Theme > Dark, Light, Classic | |
| | Reset the layout of the windows | |
| | Remote control > Enable / Disable | le serveur ZeroMQ du [chapitre 18](18-Remote-Control.md), et son endpoint (*Disabled at build time* avec `TPNE_ZEROMQ=0`) |
| **Simulation** | Start / Stop (Space), Pause / Resume (P), Speed, Conflicts | §5 |
| | Start all the nets, the nets of the document | plusieurs réseaux |
| **Analysis** | *Petri net*: Properties (deadlocks, bounds, invariants) | §7 |
| | *(max,+) algebra*: Critical circuit (cycle time), Gantt chart (earliest dates), (max,+) linear system, Dater and counter equations, Adjacency matrices, Firing dates of the simulation | réseaux temporisés seulement ; pour un réseau temporisé qui n'est pas un graphe d'événements, l'onglet dit pourquoi |
| **Debug** | endpoint and *Connect* / Disconnect from ... | suivre un programme qui publie son journal ([chapitre 13](13-Debugging.md)) |
| | Open a manifest..., Open a journal..., Save the journal..., Clear the marks of the nodes | |
| **Help** | Mouse and keys (F1), GRAFCET receptivities, GRAFCET actions (qualifiers), GRAFCET control panel, About | la fenêtre d'aide a aussi un onglet *Paths* (dossiers data, temporaire, log et configuration) |

## 10. Ligne de commande

```text
TimedPetriNetEditor [-r] [-s|-w screenshot.png] [-d seconds] [-f window] [-c endpoint] [-g WxH] [petri.json]
```

| Option | Effet |
|--------|--------|
| `petri.json` | le fichier à ouvrir (aussi `.flowshop`, `.pnml`, `.symmetri.pnml`, `.grml`, `.teg`) |
| `-r` | démarrer la simulation une fois le fichier chargé |
| `-s file.png` | enregistrer le réseau en PNG une fois dessiné, puis quitter |
| `-w file.png` | idem pour toute la fenêtre |
| `-d seconds` | attendre avant la capture (la simulation tourne entre-temps) |
| `-f window` | mettre cette fenêtre au premier plan : `Journal`, `Console`, `GEMMA editor` (ou `GEMMA`), `GEMMA graph`, `Analysis` (ses propriétés) ou un onglet de l'analyse : `Properties`, `Critical circuit` (aussi marqué sur le réseau), `Gantt`, `Linear system`, `Equations`, `Adjacency`, `Firing dates` |
| `-c endpoint` | suivre le programme qui publie son journal là (`tcp://localhost:5556`) |
| `-g WxH` | taille de la fenêtre en pixels (défaut `1100x768`) |
| `-h` | l'aide |

Les images de cette documentation sont produites avec ces options par `tools/screenshots.sh`, par exemple :

```sh
./build/TimedPetriNetEditor -g 1400x900 -w doc/pics/14-Window-Journal.png \
    -r -d 4 -f Journal examples/Timed-Petri-Nets/Durations/Durations.json
```

---

Suivant : [15. Formats de fichiers](15-File-Formats.md).
