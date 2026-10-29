# 📚 Documentation TimedPetriNetEditor


[![fr](https://lecrapouille.github.io/icons/fr.png)](../fr/README.md) [![en](https://lecrapouille.github.io/icons/en.png)](../en/README.md)

Cette documentation enseigne la théorie de chaque famille de réseaux, puis montre ce que l’éditeur propose pour résoudre ses problèmes, avec de nombreux exemples. Elle avance pas à pas : chaque chapitre reprend le vocabulaire des chapitres précédents. Si vous n’avez jamais vu un réseau de Petri, commencez au chapitre 1 et lisez dans l’ordre ; si vous connaissez la théorie, allez directement à la partie qui vous intéresse.

Chaque nombre, équation, sortie et image de ces pages a été produit par l’éditeur, par `tpne-check` ou par un programme de [`examples/`](../../examples/README.md), jamais recopié à la main.

## 🧭 Partie I — Découvrir

| # | Chapitre | Ce que vous apprendrez |
|---|----------|------------------------|
| 1 | 🧩 [Introduction](01-Introduction.md) | Ce qu’est un système à événements discrets, pourquoi les réseaux de Petri, les familles de réseaux de l’éditeur, l’histoire du projet |
| 2 | 🚀 [Premiers pas](02-Getting-Started.md) | Compiler l’éditeur, ouvrir un exemple, dessiner et exécuter votre premier réseau |

## 🔵 Partie II — Réseaux de Petri

| # | Chapitre | Ce que vous apprendrez |
|---|----------|------------------------|
| 3 | 🔵 [Réseaux de Petri](03-Petri-Nets.md) | La définition formelle, la règle de tir, le marquage, les schémas de modélisation (séquence, parallélisme, synchronisation, choix, exclusion mutuelle, ressources) |
| 4 | 🔍 [Analyse des réseaux de Petri](04-Analysis.md) | Atteignabilité, bornitude, vivacité, interblocages, la matrice d’incidence et l’équation d’état, P- et T-invariants, et comment l’éditeur et `tpne-check` les trouvent |
| 5 | 🧪 [Réseaux de Petri étendus](05-Extended-Petri-Nets.md) | Poids, capacités, arcs inhibiteurs, arcs de remise à zéro, priorités, jetons colorés : ce que chaque extension apporte, ce qu’elle coûte à l’analyse |

## ⏱️ Partie III — Le temps

| # | Chapitre | Ce que vous apprendrez |
|---|----------|------------------------|
| 6 | ⏱️ [Réseaux de Petri temporisés](06-Timed-Petri-Nets.md) | Durées et délais, fonctionnement d’un réseau temporisé, temporisations, sources et puits, conflits dans le temps |
| 7 | 🔁 [Graphes d’événements temporisés](07-Timed-Event-Graphs.md) | Graphes d’événements, le dessin compact, équations de dateurs et de compteurs, une usine (flowshop) : machines, convoyeurs, diagramme de Gantt |
| 8 | ➕ [L’algèbre (max,+)](08-Max-Plus-Algebra.md) | L’algèbre, ses matrices, les systèmes linéaires des graphes d’événements temporisés, le temps de cycle comme valeur propre, l’algorithme de Howard et le circuit critique |

## 🏭 Partie IV — Automatisme

| # | Chapitre | Ce que vous apprendrez |
|---|----------|------------------------|
| 9 | 🔘 [GRAFCET](09-GRAFCET.md) | La norme IEC 60848 : étapes, réceptivités, règles d’évolution, actions et leurs qualificateurs, temps, forçages ; le pupitre de commande et le chronogramme |
| 10 | 🛑 [GEMMA](10-GEMMA.md) | Les modes marche et arrêt d’une machine, un séchoir piloté par trois GRAFCET, et la fenêtre GEMMA qui les fait (une ligne d’embouteillage) |

## ⚙️ Partie V — Du modèle au programme

| # | Chapitre | Ce que vous apprendrez |
|---|----------|------------------------|
| 11 | ⚙️ [Le runtime](11-Runtime.md) | Une transition est une fonction de votre programme (l’idée de Symmetri) : couleurs de résultats, instances, gardes, événements injectés, exécuteurs, journaux ; du dessin à un robot en marche |
| 12 | 🏗️ [Génération de code](12-Code-Generation.md) | Le squelette d’un programme pour le runtime, et le code C++ des GRAFCET pour un Arduino |
| 13 | 🐞 [Débogage](13-Debugging.md) | L’éditeur comme débogueur visuel d’un programme en cours, le journal, sa relecture, l’extraction de processus |

## 📖 Partie VI — Référence

| # | Chapitre | Contenu |
|---|----------|---------|
| 14 | 🖱️ [Référence de l’éditeur](14-Editor-Reference.md) | Chaque fenêtre, geste souris, touche, menu et option de ligne de commande |
| 15 | 📄 [Formats de fichiers](15-File-Formats.md) | Le fichier JSON, et chaque format que l’éditeur lit et écrit |
| 16 | 🧰 [Bibliothèque C++](16-Library.md) | Les bibliothèques du projet et leur API |
| 17 | 🔢 [Julia](17-Julia.md) | L’éditeur et l’algèbre (max,+) depuis Julia |
| 18 | 📡 [Contrôle à distance](18-Remote-Control.md) | Piloter l’éditeur depuis un autre programme avec des commandes JSON sur ZeroMQ |
| — | 🔤 [Glossaire](Glossary.md) | Chaque mot en gras de ces pages |
| — | 📚 [Bibliographie](Bibliography.md) | Livres, cours et autres outils |

## 🗺️ Parcours dans la documentation

- **« Je veux voir un réseau tourner, tout de suite. »** → [2](02-Getting-Started.md), puis les [exemples](../../examples/README.md).
- **« J’apprends les réseaux de Petri. »** → [1](01-Introduction.md), [3](03-Petri-Nets.md), [4](04-Analysis.md), [5](05-Extended-Petri-Nets.md).
- **« À quelle vitesse peut aller mon usine, mes trains, mon processus ? Qu’est-ce qui le freine ? »** → [6](06-Timed-Petri-Nets.md), [7](07-Timed-Event-Graphs.md), [8](08-Max-Plus-Algebra.md).
- **« J’enseigne ou j’apprends l’automatisme. »** → [9](09-GRAFCET.md), [10](10-GEMMA.md), [12](12-Code-Generation.md) pour un Arduino.
- **« Je veux que mon programme soit piloté par un réseau. »** → [3](03-Petri-Nets.md), [5](05-Extended-Petri-Nets.md), [11](11-Runtime.md), [12](12-Code-Generation.md), [13](13-Debugging.md).
- **« Mon réseau peut-il s’interbloquer ? »** → [4](04-Analysis.md).
- **« Quelle touche fait quoi ? »** → [14](14-Editor-Reference.md).
