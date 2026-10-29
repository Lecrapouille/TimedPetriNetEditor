# 📄 15. Formats de fichiers


[![fr](https://lecrapouille.github.io/icons/fr.png)](../fr/15-File-Formats.md) [![en](https://lecrapouille.github.io/icons/en.png)](../en/15-File-Formats.md)

Ce que vous trouverez ici : le fichier JSON de l'éditeur champ par champ (§1 à §3), puis chaque format que l'éditeur lit et écrit (§4) et les formats des journaux (§5). Les extraits ont été produits en exportant les exemples de [`examples/`](../../examples/README.md) avec la bibliothèque de ce commit.

---

## 1. Le fichier JSON

*File > Save* écrit un fichier JSON. Vous pouvez aussi l'écrire à la main, ou le générer depuis un autre programme. Voici [`FirstNet.json`](../../examples/Petri-Nets/FirstNet/FirstNet.json) tel que l'éditeur l'enregistre :

```json
{
  "revision": 2,
  "type": "Petri net",
  "nets": [
    {
       "name": "First net",
       "description": "The smallest Petri net. Press Space to simulate, then click on the green transition Fire: ...",
       "places": [
            { "id": 0, "caption": "Ready", "tokens": 1 },
            { "id": 1, "caption": "Done", "tokens": 0 }
       ],
       "transitions": [
            { "id": 0, "caption": "Fire" }
       ],
       "arcs": [
            { "from": "P0", "to": "T0" },
            { "from": "T0", "to": "P1", "duration": 0 }
       ],
       "actions": [
       ],
       "layout": {
          "places": [
             [150, 200],
             [450, 200]
          ],
          "transitions": [
             [300, 200]
          ]
       }
    }
  ],
  "inputs": [
  ]
}
```

```text
   (●) Ready ──► ▭ Fire ──► ( ) Done
```

### 1.1 Le document

| Champ | Signification |
|-------|---------|
| `revision` | version de la syntaxe. **2** : le dessin est dans un objet `layout`, séparé de la structure. La révision 1 (positions `x`, `y` dans les nœuds) et les anciens fichiers (`revision` jusqu'à 5 de l'ancien numérotage) sont encore lus ; leurs réceptivités GRAFCET en notation polonaise inverse (`a b .`) sont converties |
| `type` | `"Petri net"`, `"Timed Petri net"`, `"Timed event graph"` ou `"GRAFCET"` : tous les réseaux du fichier ont ce type. `"Timed event graph"` est lu comme un réseau de Petri temporisé dessiné de façon compacte (places sur les arcs) |
| `description` | optionnel : objet du document, affiché dans la fenêtre *Document* |
| `nets` | un ou plusieurs réseaux (un GEMMA en a trois) |
| `inputs`, `outputs` | GRAFCET seulement (§3) |

### 1.2 Un réseau

| Champ | Signification |
|-------|---------|
| `name` | nom du réseau ; les autres graphes d'un document GRAFCET l'utilisent (`Production:X3`, `Production{init}`) |
| `description` | optionnel : rôle du réseau, écrit en haut à gauche sur le canevas |
| `colors` | optionnel : noms des couleurs de jetons de l'utilisateur, `["Good", "Bad"]` (§2) |
| `places` | les places (§1.3) |
| `transitions` | les transitions (§1.3) |
| `arcs` | les arcs (§1.3) |
| `actions` | GRAFCET seulement (§3) |
| `layout` | le dessin : `places` et `transitions` donnent le `[x, y]` de chaque nœud, dans l'ordre des identifiants. Absent : `x` et `y` sont lus dans les nœuds (révision 1) ; un réseau sans aucune position est placé par une disposition ressort à l'ouverture |

### 1.3 Places, transitions et arcs

| Élément | Champ | Signification |
|---------|-------|---------|
| place | `id` | 0, 1, 2 ... : consécutifs, sans trou, dans n'importe quel ordre. La clé de la place est `P` + `id` |
| | `caption` | texte quelconque (espaces autorisés) ; une étape GRAFCET l'affiche sous son numéro |
| | `tokens` | nombre initial de jetons (étape GRAFCET : 0 ou 1, 1 = étape initiale) |
| | `capacity` | optionnel : nombre maximum de jetons ; non bornée si absent |
| transition | `id`, `caption` | comme pour les places, clé `T` + `id` ; transition GRAFCET : la légende est la réceptivité |
| | `delay` | optionnel : secondes pendant lesquelles la transition reste activée avant de tirer (temporisation GRAFCET, timeout d'un réseau temporisé) |
| | `priority` | entier optionnel : avec la politique *Highest priority* (*Simulation > Conflicts*, et le runtime), la plus haute tire en premier |
| arc | `from`, `to` | `P<id>` ou `T<id>` : place vers transition ou transition vers place. Jamais deux places ni deux transitions ; la paire est unique, donc un arc n'a pas d'identifiant |
| | `duration` | sur un arc depuis une transition : temps de voyage des jetons (réseaux temporisés) ; ignoré sur les arcs vers une transition |
| | `weight` | optionnel, 1 si absent : nombre de jetons pris ou donnés |
| | `kind` | optionnel : `"normal"` (défaut), `"inhibitor"` ou `"reset"`, seulement sur un arc depuis une place |
| | `color` | optionnel : couleur des jetons pris (arc depuis une place) ou donnés (arc vers une place) (§2) |

Le sens des extensions est enseigné au [chapitre 5](05-Extended-Petri-Nets.md). En bref : un arc normal de poids `w` prend (ou donne) `w` jetons ; un **arc inhibiteur** de poids `w` laisse tirer la transition seulement tant que sa place a strictement moins de `w` jetons ; un **arc reset** vide sa place quand la transition tire ; une transition n'est pas activée si son tir mettrait plus de jetons dans une place que sa capacité.

Poids, arcs inhibiteurs et reset, capacités et couleurs n'étendent que les réseaux de Petri et les réseaux de Petri temporisés : un graphe d'événements temporisé ou un GRAFCET qui les utilise est refusé à la lecture du fichier.

## 2. Couleurs

Une **color** est le nom d'une sorte de jeton. Les couleurs intégrées sont `Success`, `Failed` et `Canceled` (résultats de la fonction d'une transition), les autres sont listées dans `colors`. Un arc depuis une place ne prend que les jetons de sa `color` (toute couleur si absent) ; un arc vers une place donne des jetons de sa `color`, sinon du résultat de la fonction de la transition. L'éditeur dessine les couleurs sur les arcs ; sa simulation ne les utilise pas : le runtime le fait ([chapitre 11](11-Runtime.md)). Exemple, [`Colors.json`](../../examples/Extended-Petri-Nets/Colors/Colors.json) :

```json
"colors": ["Good", "Bad"],
...
"arcs": [
     { "from": "P0", "to": "T0" },
     { "from": "T0", "to": "P1" },
     { "from": "P1", "to": "T1", "color": "Good" },
     ...
     { "from": "P1", "to": "T2", "color": "Bad" },
```

## 3. GRAFCET : actions, entrées, sorties

Un réseau GRAFCET a, après ses arcs, les actions de ses étapes. Extraits de [`GrafcetActions.json`](../../examples/GRAFCET/GrafcetActions/GrafcetActions.json) et des graphes Control et Safety de `GEMMA-Dryer.json` :

```json
"actions": [
  { "place_id": 2, "qualifier": "D", "color": "green", "name": "Timer1", "script": "Wait before retract", "duration": 2 },
  { "place_id": 2, "qualifier": "S", "color": "green", "name": "Lamp1", "script": "Position indicator ON" },
  { "place_id": 5, "qualifier": "N", "color": "green", "name": "Right Motor", "script": "Manual run of the right motor", "condition": "Right" },
  { "place_id": 0, "script": "Production and Control in their initial state", "forcing": ["Production:{init}", "Control:{init}"] }
]
```

| Champ | Signification |
|-------|---------|
| `place_id` | l'étape |
| `qualifier` | `N` (défaut), `S`, `R`, `D`, `L`, `SD`, `DS`, `SL`, `P1`, `P0` ([chapitre 9](09-GRAFCET.md)) |
| `name` | la sortie ; vide pour une action avec ordres de forçage seulement |
| `duration` | le d des qualificateurs temporisés, en secondes |
| `condition` | optionnel : l'action ne s'exécute que tant qu'elle est vraie (dessiné `if Dr: Motor`) |
| `color` | couleur du voyant : `green`, `red`, `orange`, `yellow`, `blue`, `white` |
| `script` | un commentaire, écrit dans le code généré |
| `forcing` | ordres de forçage, dessinés `Production{init}` (les deux-points sont optionnels) |

Après les réseaux, le document liste les **inputs** de ses GRAFCET (ce que lisent les réceptivités) et leurs **outputs** (noms des actions). Leur `kind` indique comment le panneau de commande et la fenêtre Outputs les dessinent. Extrait de [`GEMMA-Dryer.json`](../../examples/GRAFCET/GEMMA-Dryer/GEMMA-Dryer.json), avec un `clock` ajouté pour montrer ses champs :

```json
"inputs": [
  { "name": "EStop", "kind": "emergency", "description": "Emergency stop (mushroom button)", "initial": 0 },
  { "name": "Start", "kind": "button", "description": "Start of the cycle", "initial": 0, "color": "green" },
  { "name": "Manual", "kind": "selector", "description": "Manual mode", "initial": 0, "group": "Mode" },
  { "name": "Auto", "kind": "selector", "description": "Automatic mode", "initial": 1, "group": "Mode" },
  { "name": "Temperature", "kind": "value", "description": "Temperature sensor of the dryer", "initial": 20,
    "min": 20, "max": 120, "unit": "°C", "driven_by": "Heat Oven", "rise": 6, "fall": 1 },
  { "name": "Blink", "kind": "clock", "initial": 0, "period": 1 }
],
"outputs": [
  { "name": "Left Motor", "kind": "motor", "description": "Moves the belt to the left" },
  { "name": "Heat Oven", "kind": "heater", "description": "Heating resistor of the dryer" }
]
```

| Champ entrée | Signification |
|-------------|---------|
| `name` | tel qu'écrit dans les réceptivités |
| `kind` | `switch` (défaut), `button`, `pulse`, `emergency`, `selector`, `value`, `clock` ([chapitre 9](09-GRAFCET.md)) |
| `description` | ce que c'est, affiché dans l'infobulle du panneau |
| `initial` | valeur au début de la simulation |
| `color` | capuchon d'un bouton ou interrupteur : `green`, `red`, `black`, `yellow`, `blue`, `white`, `orange` |
| `group` | `selector` : les entrées du même groupe sont les positions d'un sélecteur |
| `min`, `max`, `unit` | `value` : plage et unité du capteur |
| `driven_by`, `rise`, `fall` | `value` : monte de `rise` par seconde tant que la sortie `driven_by` est active, descend de `fall` sinon (modèle simple de la machine) |
| `period` | `clock` : période en secondes |

| Champ sortie | Signification |
|--------------|---------|
| `name` | tel qu'écrit dans les actions |
| `kind` | `light` (défaut), `motor`, `heater`, `valve` (ou vérin), `horn`, `buzzer`, `timer` (son aiguille tourne tant qu'il est actif) |
| `description` | ce que c'est |

L'éditeur n'enregistre que les entrées et sorties qui ont été décrites (*Edit* sur le panneau de commande) ; les autres sont retrouvées depuis les réceptivités et les actions à l'ouverture du fichier. Les fichiers d'anciennes versions avec `sensors` au lieu de `inputs` sont encore lus.

## 4. Autres formats

*File > Open* et *File > Save as* choisissent le format par l'extension du fichier ; *File > Import* et *File > Export* listent les formats par nom. La plus longue extension l'emporte : `net.symmetri.pnml` est la variante Symmetri du PNML. *File > Export* ne liste que les formats qui peuvent écrire le réseau actif : C++ GRAFCET et GRAFCET LaTeX pour un GRAFCET ; C++ pour le runtime, Petri LaTeX et PNML Symmetri pour les autres réseaux ; formats Timed Event Graph et Julia pour un graphe d'événements temporisé ; Flowshop pour un réseau fait à partir d'un flowshop.

| Format | Extension | Lire | Écrire | Pour |
|--------|-----------|:----:|:-----:|-----|
| JSON | `.json` | oui | oui | le format de l'éditeur (§1) |
| Petri Net Markup Language | `.pnml` | oui | oui | format d'échange standard des outils de réseaux de Petri (§4.1) |
| PNML for Symmetri | `.symmetri.pnml` | oui | oui | [Symmetri](https://github.com/thorstink/Symmetri) (§4.1) |
| GreatSPN GRML | `.grml` | oui | oui | [GreatSPN](https://github.com/greatspn/SOURCES), lu aussi par Symmetri (§4.2) |
| Flowshop | `.flowshop` | oui | oui | une usine comme tableau de temps de traitement (§4.3) |
| Timed Event Graph | `.teg` | oui | oui | format texte compact des graphes d'événements temporisés (§4.4) |
| SVG picture | `.svg` | | oui | une image pour un document ou une page web (§4.5) |
| Petri LaTeX | `.tex` | | oui | un dessin TikZ pour un document (§4.6) |
| Grafcet LaTeX | `.tex` | | oui | un dessin GRAFCET avec le paquet `grafcet2` (§4.6) |
| Graphviz | `.gv`, `.dot` | | oui | un dessin mis en page par Graphviz (§4.7) |
| Draw.io | `.drawio.xml` | | oui | un dessin modifiable dans [draw.io](https://app.diagrams.net) (§4.8) |
| PN-Editor | `.pns` (avec `.pnl`, `.pnk`, `.pnkp`) | | oui | [pn-editor](https://gitlab.com/porky11/pn-editor) (§4.8) |
| Symfony | `.yaml` | | oui | un [workflow Symfony](https://symfony.com/doc/current/components/workflow.html) (§4.8) |
| Julia | `.jl` | | oui | les matrices (max,+) d'un graphe d'événements temporisé pour [MaxPlus.jl](https://github.com/Lecrapouille/MaxPlus.jl) (§4.9) |
| C++ functions for the runtime | `.cpp`, `.cc`, `.cxx` | | oui | un programme appelant une fonction par transition ([chapitre 12](12-Code-Generation.md)) |
| C++ GRAFCET (Arduino, computer) | `.hpp`, `.h`, `.hh`, `.h++` | | oui | les GRAFCET d'un document en code C++ ([chapitre 12](12-Code-Generation.md)) |

Ce que chaque format conserve :

| | poids | arcs inhibiteurs | arcs reset | durées | délais | couleurs | capacités | priorités | positions |
|---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| JSON | oui | oui | oui | oui | oui | oui | oui | oui | oui |
| PNML | oui | oui (\*) | oui (\*) | oui (\*) | oui (\*) | oui | oui (\*) | oui (\*) | oui |
| GRML | oui | oui | refusé | comme délais des transitions | | oui | | oui | oui |
| SVG, Petri LaTeX, draw.io | affichés | affichés | affichés | affichés | | affichés | | | oui |
| Graphviz | affichés | affichés | affichés | affichés | | affichés | affichés | | mise en page Graphviz |
| PN-Editor | oui (un arc de poids 3 est écrit 3 fois) | refusé | refusé | | | | | | oui |
| Symfony | | | | | | | | | |

(\*) dans des éléments `<toolspecific tool="TimedPetriNetEditor">`, ignorés par les autres outils : un PNML écrit par l'éditeur revient identique.

Un format refuse ce qu'il ne peut pas conserver, avec un message :

```text
Failed to export the Petri net to 'BoundedBuffer.grml'. Reason: 'GRML has no reset arcs'
Failed to export the Petri net to 'BB.pns'. Reason: 'PN-Editor has no inhibitor nor reset arcs'
Failed to export the Petri net to 'c.teg'. Reason: 'the net is not an event graph'
Export GRAFCET LaTeX: the net must be of type GRAFCET
```

### 4.1 PNML et PNML pour Symmetri

Le PNML écrit les places avec leur marquage initial, les transitions et les arcs avec leur poids (`<inscription>`). Les identifiants des nœuds sont leurs légendes lorsqu'elles sont uniques (Symmetri appelle les fonctions par elles), sinon leurs clés. Ce que le PNML ne connaît pas va dans `<toolspecific>`. [`BoundedBuffer.json`](../../examples/Extended-Petri-Nets/BoundedBuffer/BoundedBuffer.json) exporté (extrait) :

```xml
      <place id="Buffer">
        <name><text>Buffer</text></name>
        <graphics><position x="300" y="150"/></graphics>
        <initialMarking><text>1</text></initialMarking>
        <toolspecific tool="TimedPetriNetEditor" version="1"><capacity>4</capacity></toolspecific>
      </place>
      ...
      <arc id="A3" source="Stopped" target="Produce">
        <toolspecific tool="TimedPetriNetEditor" version="1"><kind>inhibitor</kind></toolspecific>
      </arc>
      <arc id="A4" source="Buffer" target="Pack">
        <inscription><text>2</text></inscription>
      </arc>
```

La variante Symmetri écrit un `<initialMarking>` par jeton et les couleurs des arcs dans un attribut `color` ; [`PickAndPlace.json`](../../examples/Runtime/PickAndPlace/PickAndPlace.json) exporté en `PickAndPlace.symmetri.pnml` (extrait) :

```xml
      <arc id="A4" source="Held" target="place" color="Success">
      </arc>
      <arc id="A5" source="place" target="Done">
      </arc>
      <arc id="A6" source="Held" target="retry" color="Failed">
      </arc>
```

L'import lit les deux variantes, et les PNML des anciennes versions de l'éditeur (où l'inscription était la durée).

### 4.2 GRML (GreatSPN)

Le GRML est le XML de CosyVerif, écrit par GreatSPN et lu par Symmetri : places avec marquage, transitions avec priorité et distribution, arcs avec valuation (poids) et type (`arc` ou `inhibitorarc`). Un réseau temporisé exporte une transition en `DETERMINISTIC` avec la plus grande durée de ses arcs de sortie ; l'import donne le délai d'une transition `DETERMINISTIC` à ses arcs de sortie. [`Durations.json`](../../examples/Timed-Petri-Nets/Durations/Durations.json) exporté (extrait) :

```xml
<node id="4" nodeType="transition" x="250" y="200">
  <attribute name="name">Toast</attribute>
  <attribute name="distribution"><attribute name="type">DETERMINISTIC</attribute><attribute name="param"><attribute name="expr"><attribute name="numValue">3</attribute></attribute></attribute></attribute>
  <attribute name="priority"><attribute name="expr"><attribute name="intValue">0</attribute></attribute></attribute>
</node>
...
<arc id="7" arcType="arc" source="1" target="4">
  <attribute name="valuation"><attribute name="expr"><attribute name="intValue">1</attribute></attribute></attribute>
</arc>
```

### 4.3 Flowshop

Un **flowshop** est une usine qui produit des pièces ([chapitre 7](07-Timed-Event-Graphs.md)). Chaque pièce visite les machines dans le même ordre, et chaque machine travaille sur les convoyeurs dans le même ordre. Sur le graphe d'événements temporisé, les machines sont les cycles horizontaux et les convoyeurs les cycles verticaux. Le fichier est un tableau de temps de traitement ; l'éditeur le transforme en ce graphe. Un fichier `.flowshop` contient le tableau dans l'une de deux syntaxes : un tableau CSV, lu et écrit, ou les anciennes lignes `clé: valeurs` de ScicosLab et MaxPlus.jl, lues seulement. L'éditeur les distingue par la première ligne qui n'est pas un commentaire : une `,` en fait un tableau CSV.

*File > New > Flowshop...* remplit le même tableau dans une fenêtre : les noms et leur nombre d'unités dans la première ligne et la première colonne, les temps de traitement dans les autres cases. *From the active net* le remplit avec le flowshop du réseau actif, pour le modifier et refaire le réseau.

#### Tableau CSV

[`simple.flowshop`](../../examples/Flowshops/simple.flowshop) :

```text
# Simple flowshop: 2 machines, 3 conveyors, one of each resource.
machine\conveyor,C1,C2,C3
M1,2.0,3.0,1.0
M2,1.0,2.0,1.5
```

| Partie | Signification |
|------|---------|
| lignes commençant par `#` | commentaires ; les lignes vides sont ignorées |
| première ligne | les convoyeurs (les colonnes) ; sa première case est un titre, ignoré |
| première colonne | les machines (les lignes) |
| `Nom*n` | l'usine possède `n` machines, ou convoyeurs, de ce type : les jetons de son cycle. Sans `*n` : un seul |
| `"Nom"` | un nom entre guillemets peut contenir `,`, `*` ou `#` ; `""` y est un guillemet : `"C*1"*2` est deux convoyeurs nommés `C*1` |
| autres cases | temps de traitement du convoyeur sur la machine, dans l'unité de temps du réseau (les durées des arcs) |
| case vide (ou `-`, `-inf`, `eps`) | le convoyeur ne passe pas par cette machine (zéro (max,+)) |

Le nombre de machines et de convoyeurs est la taille du tableau. [`complex-conveyors.flowshop`](../../examples/Flowshops/complex-conveyors.flowshop) possède deux `Conveyor3`, écrit `Conveyor3*2` dans sa première ligne.

*File > Export > Flowshop* écrit le tableau du réseau actif, après des commentaires qui expliquent le format. Il n'est proposé que pour les réseaux faits à partir d'un flowshop : l'éditeur retrouve le tableau à partir des opérations `<machine>_<conveyor>`, de l'ordre des arcs entre elles, et des jetons donnés par les transitions `<machine>` et `<conveyor>`. Un nom de machine ne doit pas contenir de `_` : l'opération `M1_C2` est coupée sur son premier `_`, et l'export ne retrouverait pas le tableau. La fenêtre refuse aussi un nom contenant un espace.

#### Lignes `clé: valeurs` (ancienne syntaxe)

La syntaxe des flowshops de ScicosLab et de [MaxPlus.jl](https://github.com/Lecrapouille/MaxPlus.jl). [`complex.flowshop`](../../examples/Flowshops/complex.flowshop) commence par :

```text
nconveyors: 6
nmachines: 8

# How many of each machine (tokens of the horizontal cycle)
nm: 1 1 1 1 1 1 1 1

# How many of each conveyor (tokens of the vertical cycle)
np: 1 1 1 1 1 1

# Conveyor names (columns)
conveyors: Conveyor1 Conveyor2 Conveyor3 Conveyor4 Conveyor5 Conveyor6

# Processing times: machine: time on each conveyor
# -inf means (max,+) zero: this conveyor does not visit this machine
Machine1: 2.0  3.9  0.95 1.1  0.7  1.4
Machine2: -inf -inf 2.0  1.2  -inf 1.7
```

| Ligne | Signification |
|------|---------|
| `nconveyors`, `nmachines` | taille du tableau |
| `nm` | combien de machines de chaque ligne (jetons du cycle horizontal) |
| `np` | combien de convoyeurs de chaque colonne (jetons du cycle vertical) |
| `conveyors` | noms des convoyeurs (colonnes) |
| `M1: ...` | une machine et son temps de traitement sur chaque convoyeur ; `-inf` : le convoyeur ne passe pas par cette machine |

### 4.4 Graphe d'événements temporisé (`.teg`)

Format texte compact pour graphes d'événements temporisés, du projet [algorithme de Howard](http://www.cmap.polytechnique.fr/~gaubert/HOWARD2.html). La première ligne donne le nombre de transitions et d'arcs, puis une ligne par arc entre deux transitions (place cachée) : `<from> <to>: <duration> <tokens>`. Le format n'a ni noms ni positions. [`Howard2.json`](../../examples/Timed-Event-Graphs/Howard2/Howard2.json) exporté :

```text
TimedEventGraph 4 5

0 2:   5    2
1 0:   5    0
2 1:   3    0
3 0:   1    0
2 3:   1    0
```

Le premier arc va de `T0` à `T2`, dure 5 et contient 2 jetons.

### 4.5 SVG

Une image du réseau aux positions de l'éditeur : places avec leurs jetons (GRAFCET : étapes, double pour les initiales), transitions, arcs avec poids, durée et couleur ; les arcs inhibiteurs se terminent par un cercle, les arcs reset sont en pointillés. Un graphe d'événements temporisé dessiné de façon compacte garde ses places sur les arcs. Ouvrez-le dans un navigateur, insérez-le dans un document.

![SVG export](pics/15-SVG.png)

*`BoundedBuffer.json` exporté en SVG (rendu ici en PNG).*

### 4.6 LaTeX

Deux exports écrivent un fichier `.tex` à compiler avec `pdflatex file.tex` (sur Debian ou Ubuntu : `sudo apt install texlive-pictures`) :

- **Petri LaTeX** utilise la bibliothèque `petri` de TikZ : places avec jetons, transitions, arcs avec poids, durée et couleur (arcs inhibiteurs terminés par un cercle ouvert, arcs reset à double tête en pointillés). 50 pixels de l'éditeur valent 1 cm.
- **Grafcet LaTeX** (GRAFCET seulement) utilise le paquet [grafcet2](https://github.com/nagimov/grafcet2), fourni dans [`data/grafcet2.sty`](../data/grafcet2.sty) : étapes (initiales en double), transitions avec réceptivité.

`FirstNet.json` en Petri LaTeX (complet) :

```latex
% Generated by TimedPetriNetEditor from the net 'First net'.
\documentclass[border=0.2cm]{standalone}
\usepackage{tikz}
\usetikzlibrary{petri,arrows.meta}
\begin{document}
\begin{tikzpicture}[
  every place/.style={draw=blue!75, fill=blue!10, minimum size=7mm, thick},
  every transition/.style={draw=black, fill=black!70, minimum width=6mm, minimum height=2mm},
  every label/.style={font=\small},
  arc/.style={-{Latex}, thick},
  inhibitor/.style={-{Circle[open]}, thick},
  reset/.style={-{Latex}{Latex}, thick, dashed}]

% Places
\node[place, label=below:{Ready}, tokens=1] (P0) at (3.00, -4.00) {};
\node[place, label=below:{Done}] (P1) at (9.00, -4.00) {};

% Transitions
\node[transition, label=above:{Fire}] (T0) at (6.00, -4.00) {};

% Arcs
\draw[arc] (P0) -- (T0);
\draw[arc] (T0) -- (P1);
\end{tikzpicture}
\end{document}
```

[`GermanTrafficLights.json`](../../examples/GRAFCET/GermanTrafficLights/GermanTrafficLights.json) en Grafcet LaTeX (corps) :

```latex
\begin{grafcet}
\Loop{
  \FirstStep[Green]{}
  \Transition{3s/X0}
  \Step[Yellow]{}
  \Transition{1s/X1}
  \Step[Red]{}
  \Transition{3s/X2}
  \Step[Red and yellow]{}
  \Transition{1s/X3}
}
\end{grafcet}
```

### 4.7 Graphviz

Un fichier `.dot` : places en cercles avec jetons (`•`) et capacité (`max 4`), transitions en boîtes, arcs avec poids, durée et couleur ; arcs inhibiteurs terminés par `odot`, arcs reset en pointillés à double tête. La mise en page est celle de Graphviz, pas celle de l'éditeur (l'export SVG garde la seconde). Visualisez avec `xdot file.dot`, dessinez avec `dot -Tpng file.dot -o file.png`.

```dot
digraph "Bounded buffer" {
  rankdir=LR;
  ...
  P1 [label="Buffer\n•", xlabel="max 4"];
  ...
  P3 -> T0 [arrowhead=odot];
  P1 -> T1 [label="2"];
  ...
  P1 -> T2 [arrowhead=normalnormal, style=dashed];
```

![Graphviz export](pics/15-Graphviz.png)

*`BoundedBuffer.json` exporté vers Graphviz, dessiné par `dot -Tpng`.*

### 4.8 Draw.io, PN-Editor, Symfony

- **Draw.io** : un `mxfile` avec une cellule par nœud (et une par légende) et par arc, aux positions de l'éditeur, modifiable dans draw.io.
- **PN-Editor** : les quatre fichiers binaires de [pn-editor](https://gitlab.com/porky11/pn-editor) (`.pns` la structure, `.pnl` les positions, `.pnk` les noms des transitions, `.pnkp` les noms des places ; pn-editor s'installe à la main). Un arc de poids 3 est écrit trois fois.
- **Symfony** : un workflow du framework Symfony (PHP) : places, transitions avec `from` et `to`, marquage initial. Les noms sont en minuscules avec `_` :

```yaml
# Generated by TimedPetriNetEditor from the net 'First net'.
framework:
    workflows:
        first_net:
            type: 'workflow'
            ...
            initial_marking:
                - 'ready'
            places:
                - 'ready'
                - 'done'
            transitions:
                'fire':
                    from:
                        - 'ready'
                    to:
                        - 'done'
```

### 4.9 Julia

Écrit, pour un graphe d'événements temporisé, un script Julia pour MaxPlus.jl : transitions, arcs avec durées et jetons, équations compteur et dater en commentaires, et matrices `D`, `A`, `B`, `C` du système linéaire (max,+) de la forme canonique ([chapitre 8](08-Max-Plus-Algebra.md)). `Howard2.json` exporté (extrait) :

```julia
using MaxPlus, SparseArrays
...
# Timed event graph represented as dater equation:
# T0(k) = max(1 + T3(k), 5 + T1(k))
# T1(k) = max(3 + T2(k))
# T2(k) = max(5 + T0(k - 2))
# T3(k) = max(1 + T2(k))
...
D = sparse([1, 1, 2, 4], [2, 4, 3, 3], MP([5, 1, 3, 1]), 5, 5) # States without tokens
A = sparse([3, 5], [5, 1], MP([5, 0]), 5, 5) # States with 1 token
B = sparse([], [], MP([]), 5, 0) # Inputs
C = sparse([], [], MP([]), 0, 5) # Outputs
S = MPSysLin(A, B, C, D)
```

Les tableaux de la fenêtre *Analysis* ont aussi un bouton *Copy as Julia* ([chapitre 17](17-Julia.md)).

## 5. Journaux

La fenêtre *Journal* de l'éditeur et le runtime écrivent leurs journaux (tirs : instance, transition, event, result, date) en :

| Format | Extension | Pour |
|--------|-----------|-----|
| JSON lines | `.jsonl` (aussi `.json`, `.log`) | l'éditeur (*Journal > Open*), `Runtime::restore()` après un crash |
| CSV | `.csv` | tableur |
| XES (IEEE 1849) | `.xes` | process mining : [ProM](https://promtools.org), [PM4Py](https://pm4py.fit.fraunhofer.de), Disco |

JSON lines, un objet par ligne (depuis `build/examples/PickAndPlace`) :

```text
{"instance":"order_42","transition":"grasp","event":"Completed","result":"Failed","date":0.150442702}
{"instance":"order_42","transition":"retry","event":"Scheduled","date":0.150442702}
{"instance":"order_42","transition":"retry","event":"Completed","result":"Success","date":0.150442702}
```

CSV (depuis `build/examples/MicroServices`, `orders.csv`) :

```text
instance,transition,event,result,date
fast,request,Scheduled,,0
fast,request,Started,,0
fast,request,Completed,Success,0.0101056
fast,paid,Scheduled,,0.100624
```

XES : une trace par instance, un événement par ligne ; `concept:name` est la transition, `lifecycle:transition` vaut `schedule`, `start` ou `complete`, `org:resource` le résultat, `time:timestamp` la date en secondes depuis une origine (0 donne des dates en 1970) :

```xml
  <trace>
    <string key="concept:name" value="fast"/>
    <event>
      <string key="concept:name" value="request"/>
      <string key="lifecycle:transition" value="complete"/>
      <date key="time:timestamp" value="1970-01-01T00:00:00.010Z"/>
      <string key="org:resource" value="Success"/>
    </event>
```

Dans un programme : `saveJournal(instance->log(), net.colors, "orders.xes")` (`Runtime/Journal.hpp`) choisit le format par l'extension ([chapitre 16](16-Library.md)).

---

Suivant : [16. Bibliothèque C++](16-Library.md).
