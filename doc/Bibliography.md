# 📚 Bibliography

Books, papers, lectures and tools about Petri nets, (max,+) algebra,
GRAFCET and GEMMA, and the runtimes of nets. 🇬🇧 English, 🇫🇷 French.
Back to the [documentation](README.md).

## 🔵 Petri nets

- 🇩🇪 C. A. Petri, *Kommunikation mit Automaten*, PhD thesis, University of
  Bonn, 1962. The birth of Petri nets.
- 🇬🇧 T. Murata, *Petri Nets: Properties, Analysis and Applications*,
  Proceedings of the IEEE, 77(4), 1989. The classic survey: definitions,
  properties, reachability, invariants, subclasses.
- 🇬🇧 🇫🇷 R. David, H. Alla, *Discrete, Continuous, and Hybrid Petri Nets*,
  Springer, 2005 (French: *Du Grafcet aux réseaux de Petri*, Hermès, 1992).
  Petri nets for engineers, from GRAFCET to timed nets.
- 🇬🇧 R. M. Karp, R. E. Miller, *Parallel Program Schemata*, Journal of
  Computer and System Sciences, 3(2), 1969. The coverability tree.
- 🇬🇧 K. Jensen, L. M. Kristensen, *Coloured Petri Nets: Modelling and
  Validation of Concurrent Systems*, Springer, 2009. Colored Petri nets in
  general (this editor implements the simpler colors of Symmetri).
- 🇬🇧 ISO/IEC 15909-2, *Petri Net Markup Language (PNML)*. The exchange
  format.

## ➕ (max,+) algebra and timed event graphs

- 🇬🇧 🇫🇷 [jpquadrat.github.io](https://jpquadrat.github.io/): research,
  lectures and books of Jean-Pierre Quadrat on timed Petri nets and (max,+)
  algebra.
- 🇬🇧 G. Cohen, D. Dubois, J.-P. Quadrat, M. Viot, *A Linear System-Theoretic
  View of Discrete-Event Processes and its Use for Performance Evaluation in
  Manufacturing*, IEEE Transactions on Automatic Control, 30(3), 1985.
- 🇬🇧 F. Baccelli, G. Cohen, G. J. Olsder, J.-P. Quadrat, [*Synchronization
  and Linearity: An Algebra for Discrete Event Systems*](https://www.rocq.inria.fr/metalau/cohen/SED/book-online.html),
  Wiley, 1992, free online. The reference book; the Scilab toolbox follows it.
- 🇬🇧 B. Heidergott, G. J. Olsder, J. van der Woude, *Max Plus at Work*,
  Princeton University Press, 2006. A gentle introduction, with trains.
- 🇬🇧 J. Cochet-Terrasson, G. Cohen, S. Gaubert, M. McGettrick,
  J.-P. Quadrat, *Numerical Computation of Spectral Elements in Max-Plus
  Algebra*, IFAC Conference on System Structure and Control, 1998. The
  [Howard algorithm](http://www.cmap.polytechnique.fr/~gaubert/HOWARD2.html)
  used by ScicosLab and by this editor.
- 🇬🇧 [MaxPlus.jl](https://github.com/Lecrapouille/MaxPlus.jl): the (max,+)
  toolbox for Julia, a port of the toolbox of [ScicosLab](http://www.scicoslab.org/),
  with tutorials.

## 🔘 GRAFCET and GEMMA

- 🇬🇧 🇫🇷 IEC 60848, *GRAFCET specification language for sequential function
  charts*. The standard followed by the GRAFCET engine of the editor.
- 🇫🇷 ADEPA, *Le GEMMA, Guide d'Étude des Modes de Marche et d'Arrêt*, 1981.
- 🇫🇷 [ENSIAME IMS](https://www.fichier-pdf.fr/2014/06/16/cours-api-ims-2014-v1/),
  a lecture on PLCs by J.-C. Popieul.
- 🇫🇷 [Cours sur le GRAFCET](https://www.electronique-mixte.fr/wp-content/uploads/2018/07/Formation-GRAFCET-cours-9.pdf)
  by O. Kamach.
- 🇫🇷 [Introduction to the GEMMA](https://www.youtube.com/watch?v=y8lWIG71040)
  (video by NetProf), the origin of the dryer example. The GEMMA template:
  [PDF](../data/GEMMA.pdf), [draw.io](../data/GEMMA.drawio).
- 🇫🇷 Programming a GRAFCET in C: [video](https://youtu.be/l1F2dIA90s0), and
  [for an Arduino](https://youtu.be/v5FwJvtGaEw), the video that started the
  C++ code generation of the editor.

## ⚙️ Runtimes, workflows, process mining

- 🇬🇧 [Symmetri](https://github.com/thorstink/Symmetri) by Thomas Horstink:
  a C++ library running Petri nets whose transitions are functions. The
  idea of the runtime of this project.
- 🇬🇧 [GreatSPN](https://github.com/greatspn/SOURCES): a Petri net tool whose
  GRML format the editor reads and writes.
- 🇬🇧 W. van der Aalst, *Process Mining: Data Science in Action*, Springer,
  2016; IEEE 1849 (XES), the format of the journals for
  [ProM](https://promtools.org) and [PM4Py](https://pm4py.fit.fraunhofer.de).
- 🇬🇧 [Symfony workflow](https://symfony.com/doc/current/components/workflow.html):
  workflows as Petri nets in PHP, an export of the editor.
- 🇬🇧 Videos of the YAWL User Group, the origin of four examples:
  [chain saw](https://www.youtube.com/shorts/V42WiF2Ib3w),
  [landing gear](https://www.youtube.com/shorts/IqMyF5JKoE4),
  [smart home](https://www.youtube.com/shorts/BB5N9Nxtq00),
  [German traffic lights](https://www.youtube.com/shorts/ZQZCpDRqwKY).

## 🖥️ Other editors

- 🇬🇧 [pnet-simulator](https://github.com/igorakim/pnet-simulator): an online
  Petri net editor that inspired the first interface of this editor.
- 🇬🇧 [pn-editor](https://gitlab.com/porky11/pn-editor): a Petri net editor,
  an export of this one.
- 🇫🇷 [Outils GRAFCET](http://www.unice.fr/dgaffe/recherche/outils_grafcet.html)
  by D. Gaffé: GRAFCET tools.
- 🇫🇷 [APIMOU](https://sites.google.com/view/apimou/accueil): a GRAFCET editor
  generating code for Arduino ([sources](https://sites.google.com/view/apimou/sources?authuser=0)).
- 🇬🇧 [Open sequential logic simulation](https://github.com/Kersoph/open-sequential-logic-simulation):
  a GRAFCET made with the Godot engine.
- 🇬🇧 For state machines, which the editor does not handle:
  [Harel's statecharts](https://www.itemis.com/en/products/itemis-create/documentation/user-guide/overview_what_are_state_machines)
  and [SyncCharts](http://www-sop.inria.fr/members/Charles.Andre/CA%20Publis/Cesa96/SyncCharts_Cesa96.pdf),
  the graphical side of [Esterel](https://www.college-de-france.fr/sites/default/files/documents/gerard-berry/UPL8106359781114103786_Esterelv5_primer.pdf).
