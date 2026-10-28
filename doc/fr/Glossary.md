# Glossaire


[![fr](https://lecrapouille.github.io/icons/fr.png)](../fr/Glossary.md) [![en](https://lecrapouille.github.io/icons/en.png)](../en/Glossary.md)

Les mots de TimedPetriNetEditor, en français simple. Le chapitre 1 explique les premiers avec des figures.

| Word | Meaning |
|------|---------|
| **Petri net** | Un dessin d'un système fait de places, de transitions et d'arcs, où des jetons se déplacent. |
| **Place** | Un cercle : un état ou un stock. Il contient des jetons. Nommée `P0`, `P1` ... |
| **Token** | Un point noir dans une place : une ressource, une pièce, une personne, « cet état est vrai ». |
| **Transition** | Une barre : un événement. Nommée `T0`, `T1` ... |
| **Arc** | Une flèche d'une place vers une transition ou d'une transition vers une place. |
| **Enabled** | Une transition est activée lorsque chaque place en amont a un jeton. |
| **Fire** | Une transition activée tire : elle prend un jeton de chaque place en amont et en met un dans chaque place en aval. |
| **Marking** | Combien de jetons tient chaque place : l'état de tout le réseau. |
| **Identifier** | Le nom d'un nœud donné par l'éditeur (`P3`, `T1`) ; la **caption** est le texte que vous écrivez. |
| **Duration** | Un temps inscrit sur un arc sortant d'une transition : les jetons mettent ce temps pour arriver. |
| **Timed Petri net** | Un réseau de Petri avec des durées. |
| **Event graph** | Un réseau de Petri où chaque place a exactement un arc entrant et un arc sortant : pas de choix, seulement de la synchronisation. |
| **Timed event graph** | Un graphe d'événements avec des durées. L'éditeur le dessine sans ses places (elles sont sur les arcs). |
| **(max,+) algebra** | Une algèbre où « plus » est `max` et « fois » est `+`. Elle transforme les dates d'un graphe d'événements temporisé en équations linéaires. |
| **ε (epsilon)** | Le « zéro » de l'algèbre (max,+), `-∞` : il n'y a pas d'arc. |
| **Dater equation** | `x(k)` : la date du k-ième tir d'une transition, calculée à partir des dates des transitions en amont. |
| **Counter equation** | `x(t)` : combien de fois une transition a tiré avant l'instant t. |
| **k (cycle)** | Le numéro d'un tir : k = 2 est la deuxième fois que la transition tire. |
| **Cycle time (λ)** | À long terme, le temps entre deux tirs d'une transition d'un graphe d'événements temporisé : la vitesse du système. |
| **Critical circuit** | La boucle du réseau qui fixe le temps de cycle : le goulot d'étranglement. |
| **Flowshop** | Une usine qui produit des pièces. Sur son graphe d'événements temporisé, chaque machine est un cycle horizontal et chaque convoyeur un cycle vertical. |
| **Gantt chart** | Une ligne par machine, une barre par opération et par cycle. |
| **GRAFCET** | Une norme française (IEC 60848, aussi appelée SFC) pour décrire la commande d'une machine : étapes, transitions avec conditions, actions. |
| **Step** | La place d'un GRAFCET : active (elle a un jeton) ou non. Nommée `X0`, `X1` ... |
| **Initial step** | Une étape active au départ, dessinée avec un double carré. |
| **Receptivity** | La condition écrite à côté d'une transition GRAFCET, comme `Dcy . /Au`. |
| **Action** | Ce qu'une étape active fait : mettre un moteur, une lampe en marche ... |
| **Qualifier** | La lettre d'une action : `N` (tant que l'étape est active), `S` (set), `R` (reset), `D` (delayed) ... |
| **Input** | Ce que le GRAFCET lit : un bouton, un capteur. |
| **Output** | Ce que le GRAFCET commande : un moteur, une résistance chauffante, une lampe. Il porte le nom de ses actions. |
| **Edge** | `↑a` : a vient de devenir vrai (front montant) ; `↓a` : il vient de devenir faux. |
| **Forcing order** | Une étape d'un graphe met un autre graphe dans une situation donnée : `Production{init}`. |
| **GEMMA** | Une méthode française (Guide d'Étude des Modes de Marche et d'Arrêt) pour décrire comment une machine démarre, s'arrête et reprend. Souvent trois graphes : sécurité, commande, production. |
| **Control panel** | La fenêtre de l'éditeur qui affiche les entrées d'un GRAFCET comme les boutons d'une machine. |
| **Timing diagram** | La fenêtre qui trace les étapes, les entrées et les sorties dans le temps. |
| **Document** | Un fichier : un ou plusieurs réseaux du même type, leur description, les entrées et les sorties. |
