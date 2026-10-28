# 10. 🛑 GEMMA


[![fr](https://lecrapouille.github.io/icons/fr.png)](../fr/10-GEMMA.md) [![en](https://lecrapouille.github.io/icons/en.png)](../en/10-GEMMA.md)

**Ce que vous allez apprendre**

- ce qu'est une GEMMA : la carte de toutes les façons dont une machine démarre, s'arrête et reprend ;
- ses trois familles de procédures (A, F, D) et ses rectangles ;
- comment une GEMMA devient une hiérarchie de GRAFCET qui s'imposent les uns aux autres ;
- à faire tourner un séchoir piloté par trois GRAFCET, depuis le pupitre de commande et depuis un programme.

**Prérequis** : [9. GRAFCET](09-GRAFCET.md), en particulier les ordres de forçage (§7) et les documents à plusieurs graphes (§8).

---

## 1. Pourquoi une GEMMA

Un GRAFCET décrit bien le **cycle normal** d'une machine : attendre le bouton de départ, produire, s'arrêter. Mais une machine réelle passe une grande part de sa vie en dehors de ce cycle : elle est mise sous tension, elle chauffe, un opérateur la fait tourner à la main, un capteur tombe en panne, quelqu'un appuie sur l'arrêt d'urgence, les pièces restées à l'intérieur doivent être évacuées avant de pouvoir redémarrer. Si ces cas s'ajoutent un par un au GRAFCET normal, il devient vite un plat de spaghettis que personne ne peut vérifier.

En 1981, l'ADEPA (l'agence française pour l'automatisation de la production, la même qui a travaillé sur le GRAFCET) a publié la **GEMMA** : *Guide d'Étude des Modes de Marches et d'Arrêts*. C'est un formulaire, une page A3, qui liste **tous** les modes dans lesquels une machine peut se trouver, et les passages d'un mode à l'autre. Le concepteur parcourt le formulaire avec le client, garde les modes dont la machine a besoin, barre les autres et trace les flèches entre ceux qui restent. Puis la GEMMA se traduit en GRAFCET.

Le formulaire vierge est dans le dépôt : [GEMMA.pdf](../gemma/GEMMA.pdf), modifiable avec [draw.io](https://app.diagrams.net) : [GEMMA.drawio](../gemma/GEMMA.drawio).

## 2. Les trois familles de procédures

Le formulaire est découpé en trois zones, une par famille de **procédures** (modes). Chaque procédure est un rectangle avec un code :

**A — Procédures d'arrêt** (*procédures d'Arrêt*) : la machine est arrêtée, ou en train de s'arrêter, pour une raison normale.

| Code | Procédure |
|------|-----------|
| A1 | Arrêt à l'état initial : la machine est prête à démarrer |
| A2 | Arrêt demandé en fin de cycle : le cycle va jusqu'à son terme |
| A3 | Arrêt demandé dans un état donné (pas la fin du cycle) |
| A4 | Arrêt obtenu (la machine s'est arrêtée dans un autre état que l'initial) |
| A5 | Préparation de la remise en marche après une défaillance (dégager, réparer, contrôler) |
| A6 | Mise en état initial de la partie opérative |
| A7 | Mise en état donné de la partie opérative |

**F — Procédures de fonctionnement** (*procédures de Fonctionnement*) : la machine travaille.

| Code | Procédure |
|------|-----------|
| F1 | Production normale : la raison d'être de la machine |
| F2 | Marche de préparation : chauffage, remplissage, avant la production |
| F3 | Marche de clôture : vidange, nettoyage, après la production |
| F4 | Marche de vérification hors cycle : chaque mouvement à la main, dans n'importe quel ordre |
| F5 | Marche de vérification en cycle : le cycle pas à pas |
| F6 | Marche d'essai : calibrage, tests des capteurs |

**D — Procédures en défaillance** (*procédures en Défaillance*) : quelque chose s'est mal passé.

| Code | Procédure |
|------|-----------|
| D1 | Arrêt d'urgence : tout ce qui est dangereux s'arrête immédiatement |
| D2 | Diagnostic et traitement de la défaillance |
| D3 | Production malgré la défaillance (mode dégradé) |

Les rectangles à l'intérieur du grand cadre du formulaire sont « en production » (des produits sont fabriqués), les autres non. Autour, le formulaire sépare aussi les états où la partie commande est **hors tension** et **sous tension** : la GEMMA commence à la mise sous tension.

## 3. De la GEMMA aux GRAFCET

La traduction habituelle est une **hiérarchie** de GRAFCET, chacun forçant ceux du dessous ([chapitre 9 §7](09-GRAFCET.md#7-forcing-orders)) :

```text
   ┌──────────────┐  forces   ┌──────────────────┐  forces / lit  ┌──────────────┐
   │ Safety       │ ────────► │ Control (modes)  │ ───────────────► │ Production   │
   │ D1           │ ────────────────────────────────────────────►   │ F1           │
   └──────────────┘           └──────────────────┘                  └──────────────┘
```

1. **Safety** (le niveau le plus haut) : surveille l'arrêt d'urgence. Tant qu'il est enfoncé, et jusqu'à ce que l'opérateur réarme, une étape force tous les autres graphes à leur situation initiale (D1).
2. **Control**, ou modes : suit les rectangles A et F de la GEMMA. Chaque mode est une étape ; les transitions sont les flèches du formulaire. Le graphe de production lit ses étapes (`Control:X4`) ou est forcé par lui.
3. **Production** : le cycle normal (F1), aussi simple que si rien d'autre n'existait.

Comme le forçage a priorité sur l'évolution (le graphe forcé n'évolue pas), un graphe supérieur est toujours obéi : aucune transition de la production ne peut franir tant que la sécurité la force.

## 4. Tutoriel : la GEMMA d'un séchoir

L'exemple [`examples/GRAFCET/GEMMA-Dryer`](../../examples/GRAFCET/GEMMA-Dryer/README.md) est un séchoir : un tapis amène des caisses dans un four, elles sèchent, le tapis les sort. Il vient des leçons GEMMA de NetProf ([vidéo](https://www.youtube.com/watch?v=y8lWIG71040), en français).

```sh
./build/TimedPetriNetEditor examples/GRAFCET/GEMMA-Dryer/GEMMA-Dryer.json
```

![GEMMA d'un séchoir](pics/10-GEMMA-Dryer.png)

Le document contient trois graphes, côte à côte :

```text
  Safety      X0 force Production{init} et Control{init}  ──/EStop . Rearm──►  X1 surveillance  ──EStop──► X0

  Control     X0 attente de l'accusé de réception  ──↑Ack──►  X1 dégager les caisses (N Left Motor)  ──3s/X1──►  X2 choix
              X2 ──Auto . ↑Ack──►  X3 chauffage (N Heat Oven)  ──[Temperature >= 80]──►  X4 production normale
              X2 ──Manual . ↑Ack──► X5 marche manuelle : si Left : Left Motor, si Right : Right Motor, Production{init}
              X4 ──Manual . ↑Ack──► X5          X5 ──Auto . ↑Ack──► X1

  Production  X0 attente  ──Control:X4 . Start──►  X1 tapis et four  ──Stop──►  X2 tapis et four  ──3s/X2──►  X0
```

On peut les lire sur le formulaire GEMMA :

| Étape | Procédure GEMMA |
|-------|-----------------|
| Safety:X0 | D1, arrêt d'urgence, jusqu'au réarmement |
| Control:X0 | A5, préparation de la remise en marche après la défaillance |
| Control:X1 | A6, mise à l'état initial (évacuation des caisses) |
| Control:X2 | A1, arrêt à l'état initial : le mode se choisit ici |
| Control:X3 | F2, marche de préparation (chauffage) |
| Control:X4 | F1 autorisée : le graphe de production peut démarrer |
| Control:X5 | F4, marche de vérification hors cycle (manuel) |
| Production:X1, X2 | F1, production normale ; X2 est la fin de cycle demandée par Stop (A2) |

Les **entrées** sont le pupitre du séchoir, chacune de son type : l'arrêt d'urgence `EStop` (`emergency`), les boutons-poussoirs `Rearm`, `Ack`, `Start`, `Stop`, `Left`, `Right` (`button`), le sélecteur `Mode` aux positions `Auto` et `Manual` (`selector`), et le capteur `Temperature` (`value`, de 20 à 120 °C, piloté par la sortie `Heat Oven` : +6 °C par seconde tant que le four chauffe, −1 °C par seconde sinon). Les **sorties** sont les moteurs `Left Motor`, `Right Motor` et le chauffage `Heat Oven`.

### 4.1 Le faire tourner dans l'éditeur

Appuyez sur **Space**.

![GEMMA en marche](pics/10-GEMMA-Dryer-running.png)

*`Safety:X0` est active : ses deux ordres de forçage (double cadre) maintiennent `Production` et `Control` à leurs étapes initiales. Le pupitre affiche l'arrêt d'urgence, les boutons, le sélecteur sur Auto et le thermomètre.*

| Vous faites | Ce qui se passe |
|-------------|-----------------|
| appuyer sur **Rearm** | `Safety` passe à X1 : le forçage s'arrête |
| appuyer sur **Ack** | `Control:X1` : le moteur gauche tourne (évacuation des caisses) ; 3 s plus tard, `Control:X2` |
| le sélecteur est sur **Auto** : appuyer sur **Ack** | `Control:X3` : le four chauffe, le thermomètre monte |
| attendre environ 10 s | à 80 °C, `Control:X4` |
| appuyer sur **Start** | `Production:X1` : le tapis (moteur droit) tourne et le four chauffe |
| appuyer sur **Stop** | `Production:X2` : le temporisateur `x / 3 s` compte sous `3s/X2`, puis `Production:X0` |
| enfoncer **EStop** | `Safety:X0` : tout revient au départ ; les moteurs s'arrêtent |
| relâcher EStop, **Rearm**, **Ack**, attendre 3 s, sélecteur sur **Manual**, **Ack** | `Control:X5`, le mode manuel |
| maintenir **Right** (un clic droit le garde enfoncé) | le moteur droit tourne seulement tant que le bouton est maintenu |

Suivez le tout dans l'onglet *Timing diagram* (les étapes des trois graphes, les boutons, la température en courbe, les moteurs), et dans la console. Une vitesse plus faible (**x0.5**) aide.

### 4.2 Le faire tourner depuis un programme

Le même scénario, exécuté par le moteur GRAFCET de la bibliothèque sans l'éditeur (un cycle toutes les 0,1 s, la température suivant le four comme sur le pupitre), affiche les étapes actives et les sorties après chaque action :

```text
start (forced by Safety)     t=  0.1 s  T= 20 °C  steps: Production:X0 Control:X0 Safety:X0  outputs:
Rearm                        t=  0.3 s  T= 20 °C  steps: Production:X0 Control:X0 Safety:X1  outputs:
Ack: clear the boxes         t=  0.5 s  T= 20 °C  steps: Production:X0 Control:X1 Safety:X1  outputs: [Left Motor]
3 s later                    t=  3.5 s  T= 20 °C  steps: Production:X0 Control:X2 Safety:X1  outputs:
Ack (selector on Auto)       t=  3.7 s  T= 21 °C  steps: Production:X0 Control:X3 Safety:X1  outputs: [Heat Oven]
80 °C reached               t= 13.6 s  T= 80 °C  steps: Production:X0 Control:X3 Safety:X1  outputs: [Heat Oven]
Start                        t= 13.8 s  T= 81 °C  steps: Production:X1 Control:X4 Safety:X1  outputs: [Heat Oven] [Right Motor]
5 s of drying                t= 18.8 s  T=111 °C  steps: Production:X1 Control:X4 Safety:X1  outputs: [Heat Oven] [Right Motor]
Stop                         t= 19.0 s  T=112 °C  steps: Production:X2 Control:X4 Safety:X1  outputs: [Heat Oven] [Right Motor]
3 s later                    t= 22.0 s  T=119 °C  steps: Production:X0 Control:X4 Safety:X1  outputs:
Start again                  t= 22.2 s  T=120 °C  steps: Production:X1 Control:X4 Safety:X1  outputs: [Heat Oven] [Right Motor]
EStop pushed                 t= 22.3 s  T=120 °C  steps: Production:X1 Control:X4 Safety:X0  outputs: [Heat Oven] [Right Motor]
EStop pulled back            t= 22.4 s  T=119 °C  steps: Production:X0 Control:X0 Safety:X0  outputs:
Rearm                        t= 22.6 s  T=119 °C  steps: Production:X0 Control:X0 Safety:X1  outputs:
Ack, 3 s later               t= 25.8 s  T=116 °C  steps: Production:X0 Control:X2 Safety:X1  outputs:
selector on Manual, Ack      t= 26.1 s  T=116 °C  steps: Production:X0 Control:X5 Safety:X1  outputs:
Right held                   t= 27.1 s  T=115 °C  steps: Production:X0 Control:X5 Safety:X1  outputs: [Right Motor]
Right released               t= 27.2 s  T=115 °C  steps: Production:X0 Control:X5 Safety:X1  outputs:
Start in manual mode         t= 27.4 s  T=114 °C  steps: Production:X0 Control:X5 Safety:X1  outputs:
```

Trois lignes méritent un second regard :

- **Start** (13,8 s) : en un cycle, `Control` passe de X3 à X4 (la température a été lue au cycle précédent) *et* `Production` passe de X0 à X1, car sa réceptivité `Control:X4 . Start` devient vraie pendant la recherche de la situation stable ([chapitre 9 §3.1](09-GRAFCET.md#31-the-stable-situation)).
- **EStop pushed** (22,3 s) : `Safety` est de nouveau en X0 tout de suite, mais les moteurs tournent encore pendant ce cycle. Les ordres de forçage sont lus à partir de la situation au début du cycle, donc les autres graphes sont forcés au cycle suivant (22,4 s), 0,1 s plus tard. Sur une vraie machine, l'arrêt d'urgence coupe aussi l'alimentation des actionneurs par câble : le GRAFCET ne fait que suivre.
- **Start in manual mode** : rien ne se passe. `Control:X5` force `Production{init}`, et `Production` a besoin de `Control:X4` de toute façon.

Le programme est court : il charge le fichier avec `importAllNetsFromJSON()`, donne les trois réseaux à un `GrafcetEngine`, puis appelle `setInput()`, `step(0.1f)` et lit `outputs()`. Voir le [chapitre 16](16-Library.md) pour l'API.

## 5. Faire votre propre GEMMA

1. *File > New > GEMMA (3 GRAFCETs)* : un document avec trois GRAFCET vides, nommés `Securite`, `Production` et `Controle` (les noms français), côte à côte. Renommez-les avec *Edit* dans l'onglet *Document* si vous voulez.
2. Imprimez le [formulaire GEMMA](../gemma/GEMMA.pdf), barrez les procédures dont votre machine n'a pas besoin, tracez les flèches entre les autres, et notez sur chaque flèche sa condition (un bouton, un capteur, un mode).
3. Dessinez le graphe **Safety** : une étape initiale dont l'action maintient les ordres de forçage `Production{init}` et `Controle{init}` (*Add an action*, puis *Add order* dans le panneau *Actions*), et une étape « surveillance » atteinte quand l'arrêt d'urgence est relâché et que l'opérateur réarme.
4. Dessinez le graphe **Control** : une étape par rectangle conservé, une transition par flèche. Utilisez les fronts (`↑Ack`) pour les boutons qui changent de mode, et rendez les choix exclusifs (`Auto . ↑Ack` et `Manual . ↑Ack`).
5. Dessinez le graphe **Production** comme s'il était seul ; ajoutez le mode dont il a besoin à sa première réceptivité (`Controle:X4 . Start`), et forcez-le depuis les modes où il ne doit pas tourner (`Production{init}`).
6. Sur le pupitre, *Edit* les types des entrées : l'arrêt d'urgence en `emergency`, le mode en groupe `selector`, les capteurs en `value`.
7. Lancez la simulation, et essayez chaque flèche de votre GEMMA, l'arrêt d'urgence en premier.

## 6. Pièges

- **Deux graphes forçant le même graphe différemment.** Les ordres de forçage s'appliquent dans l'ordre des graphes et des étapes : le dernier l'emporte. Un seul maître par graphe.
- **Un mode atteint via une étape transitoire.** La recherche de la situation stable peut traverser un mode sans exécuter ses actions ([chapitre 9 §12](09-GRAFCET.md#12-pitfalls)) : utilisez des fronts sur les boutons qui changent de mode.
- **Oublier la remise en marche après l'arrêt d'urgence.** Après D1, la machine ne doit pas redémarrer toute seule : le séchoir demande `Rearm` (sécurité), puis `Ack` et une évacuation des caisses (A5, A6) avant le choix du mode.

## 7. Pour aller plus loin

- Le [formulaire GEMMA](../gemma/GEMMA.pdf) ([draw.io](../gemma/GEMMA.drawio)).
- NetProf, *Introduction au GEMMA* : <https://www.youtube.com/watch?v=y8lWIG71040> (en français), la source du séchoir.
- La [bibliographie](Bibliography.md) pour les cours sur le GRAFCET et la GEMMA.
- Faire tourner les GRAFCET du séchoir sur un Arduino : [chapitre 12](12-Code-Generation.md) ; les suivre depuis l'éditeur : [chapitre 13](13-Debugging.md).

---

Suite : [11. Le runtime](11-Runtime.md) — un réseau qui appelle les fonctions de votre programme.
