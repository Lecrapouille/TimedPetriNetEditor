# 📚 Bibliographie


[![fr](https://lecrapouille.github.io/icons/fr.png)](../fr/Bibliography.md) [![en](https://lecrapouille.github.io/icons/en.png)](../en/Bibliography.md)

Livres, articles, cours et outils sur les réseaux de Petri, l'algèbre (max,+), le GRAFCET et le GEMMA, et les runtimes de réseaux. 🇬🇧 anglais, 🇫🇷 français. Retour à la [documentation](README.md).

## 🔵 Réseaux de Petri

- 🇩🇪 C. A. Petri, *Kommunikation mit Automaten*, thèse de doctorat, université de Bonn, 1962. La naissance des réseaux de Petri.
- 🇬🇧 T. Murata, *Petri Nets: Properties, Analysis and Applications*, Proceedings of the IEEE, 77(4), 1989. L'article de référence : définitions, propriétés, accessibilité, invariants, sous-classes.
- 🇬🇧 🇫🇷 R. David, H. Alla, *Discrete, Continuous, and Hybrid Petri Nets*, Springer, 2005 (français : *Du Grafcet aux réseaux de Petri*, Hermès, 1992). Les réseaux de Petri pour ingénieurs, du GRAFCET aux réseaux temporisés.
- 🇬🇧 R. M. Karp, R. E. Miller, *Parallel Program Schemata*, Journal of Computer and System Sciences, 3(2), 1969. L'arbre de couverture.
- 🇬🇧 K. Jensen, L. M. Kristensen, *Coloured Petri Nets: Modelling and Validation of Concurrent Systems*, Springer, 2009. Les réseaux de Petri colorés en général (cet éditeur implémente les couleurs plus simples de Symmetri).
- 🇬🇧 ISO/IEC 15909-2, *Petri Net Markup Language (PNML)*. Le format d'échange.

## ➕ Algèbre (max,+) et graphes d'événements temporisés

- 🇬🇧 🇫🇷 [jpquadrat.github.io](https://jpquadrat.github.io/) : recherches, cours et ouvrages de Jean-Pierre Quadrat sur les réseaux de Petri temporisés et l'algèbre (max,+).
- 🇬🇧 G. Cohen, D. Dubois, J.-P. Quadrat, M. Viot, *A Linear System-Theoretic View of Discrete-Event Processes and its Use for Performance Evaluation in Manufacturing*, IEEE Transactions on Automatic Control, 30(3), 1985.
- 🇬🇧 F. Baccelli, G. Cohen, G. J. Olsder, J.-P. Quadrat, [*Synchronization and Linearity: An Algebra for Discrete Event Systems*](https://www.rocq.inria.fr/metalau/cohen/SED/book-online.html), Wiley, 1992, en ligne gratuitement. L'ouvrage de référence ; la boîte à outils Scilab s'en inspire.
- 🇬🇧 B. Heidergott, G. J. Olsder, J. van der Woude, *Max Plus at Work*, Princeton University Press, 2006. Une introduction accessible, avec des trains.
- 🇬🇧 J. Cochet-Terrasson, G. Cohen, S. Gaubert, M. McGettrick, J.-P. Quadrat, *Numerical Computation of Spectral Elements in Max-Plus Algebra*, IFAC Conference on System Structure and Control, 1998. L'[algorithme de Howard](http://www.cmap.polytechnique.fr/~gaubert/HOWARD2.html) utilisé par ScicosLab et par cet éditeur.
- 🇬🇧 [MaxPlus.jl](https://github.com/Lecrapouille/MaxPlus.jl) : la boîte à outils (max,+) pour Julia, port de celle de [ScicosLab](http://www.scicoslab.org/), avec des tutoriels.

## 🔘 GRAFCET et GEMMA

- 🇬🇧 🇫🇷 IEC 60848, *GRAFCET specification language for sequential function charts*. La norme suivie par le moteur GRAFCET de l'éditeur.
- 🇫🇷 ADEPA, *Le GEMMA, Guide d'Étude des Modes de Marche et d'Arrêt*, 1981.
- 🇫🇷 [ENSIAME IMS](https://www.fichier-pdf.fr/2014/06/16/cours-api-ims-2014-v1/), un cours sur les API par J.-C. Popieul.
- 🇫🇷 [Cours sur le GRAFCET](https://www.electronique-mixte.fr/wp-content/uploads/2018/07/Formation-GRAFCET-cours-9.pdf) par O. Kamach.
- 🇫🇷 [Introduction au GEMMA](https://www.youtube.com/watch?v=y8lWIG71040) (vidéo NetProf), à l'origine de l'exemple du séchoir. Modèle GEMMA : [PDF](../gemma/GEMMA.pdf), [draw.io](../gemma/GEMMA.drawio).
- 🇫🇷 Programmer un GRAFCET en C : [vidéo](https://youtu.be/l1F2dIA90s0), et [pour un Arduino](https://youtu.be/v5FwJvtGaEw), la vidéo qui a lancé la génération de code C++ de l'éditeur.

## ⚙️ Runtimes, workflows, process mining

- 🇬🇧 [Symmetri](https://github.com/thorstink/Symmetri) par Thomas Horstink : une bibliothèque C++ qui exécute des réseaux de Petri dont les transitions sont des fonctions. L'idée du runtime de ce projet.
- 🇬🇧 [GreatSPN](https://github.com/greatspn/SOURCES) : un outil de réseaux de Petri dont l'éditeur lit et écrit le format GRML.
- 🇬🇧 W. van der Aalst, *Process Mining: Data Science in Action*, Springer, 2016 ; IEEE 1849 (XES), le format des journaux pour [ProM](https://promtools.org) et [PM4Py](https://pm4py.fit.fraunhofer.de).
- 🇬🇧 [Symfony workflow](https://symfony.com/doc/current/components/workflow.html) : des workflows comme réseaux de Petri en PHP, un export de l'éditeur.
- 🇬🇧 Vidéos du YAWL User Group, à l'origine de quatre exemples : [tronçonneuse](https://www.youtube.com/shorts/V42WiF2Ib3w), [train d'atterrissage](https://www.youtube.com/shorts/IqMyF5JKoE4), [maison connectée](https://www.youtube.com/shorts/BB5N9Nxtq00), [feux tricolores allemands](https://www.youtube.com/shorts/ZQZCpDRqwKY).

## 🖥️ Autres éditeurs

- 🇬🇧 [pnet-simulator](https://github.com/igorakim/pnet-simulator) : un éditeur de réseaux de Petri en ligne qui a inspiré la première interface de cet éditeur.
- 🇬🇧 [pn-editor](https://gitlab.com/porky11/pn-editor) : un éditeur de réseaux de Petri, un export de celui-ci.
- 🇫🇷 [Outils GRAFCET](http://www.unice.fr/dgaffe/recherche/outils_grafcet.html) par D. Gaffé : outils GRAFCET.
- 🇫🇷 [APIMOU](https://sites.google.com/view/apimou/accueil) : un éditeur GRAFCET générant du code pour Arduino ([sources](https://sites.google.com/view/apimou/sources?authuser=0)).
- 🇬🇧 [Open sequential logic simulation](https://github.com/Kersoph/open-sequential-logic-simulation) : un GRAFCET réalisé avec le moteur Godot.
- 🇬🇧 Pour les machines à états, que l'éditeur ne traite pas : [statecharts de Harel](https://www.itemis.com/en/products/itemis-create/documentation/user-guide/overview_what_are_state_machines) et [SyncCharts](http://www-sop.inria.fr/members/Charles.Andre/CA%20Publis/Cesa96/SyncCharts_Cesa96.pdf), le côté graphique d'[Esterel](https://www.college-de-france.fr/sites/default/files/documents/gerard-berry/UPL8106359781114103786_Esterelv5_primer.pdf).
