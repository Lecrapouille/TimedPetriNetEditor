# 10. 🛑 GEMMA


[![fr](https://lecrapouille.github.io/icons/fr.png)](../fr/10-GEMMA.md) [![en](https://lecrapouille.github.io/icons/en.png)](../en/10-GEMMA.md)

**Ce que vous allez apprendre**

- ce qu'est une GEMMA : la carte de toutes les façons dont une machine démarre, s'arrête et reprend ;
- ses trois familles de procédures (A, F, D) et ses rectangles ;
- comment une GEMMA devient une hiérarchie de GRAFCET qui s'imposent les uns aux autres ;
- à faire tourner un séchoir piloté par trois GRAFCET, depuis le pupitre de commande et depuis un programme ;
- à faire les GRAFCET à partir d'une GEMMA dans l'éditeur : garder les procédures, les relier, vérifier, générer ; puis une ligne d'embouteillage de 13 procédures.

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

Le document contient trois graphes, un onglet chacun :

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

## 5. Faire votre propre GEMMA dans l'éditeur

Le séchoir du §4 a été dessiné à la main. L'éditeur peut aussi faire les trois GRAFCET à partir de la GEMMA elle-même : vous gardez les procédures, vous donnez les conditions des liaisons, il écrit Safety, Control et Production.

### 5.1 Les deux fenêtres GEMMA

*File > New > GEMMA...* fait un nouveau document à partir d'une petite GEMMA (A1, F1, A2, D1, A5, A6 et leurs liaisons), ses trois GRAFCET, et ouvre deux fenêtres : l'**éditeur de GEMMA** (*GEMMA editor*), où la GEMMA se construit, et la **GEMMA**, où elle est dessinée comme son formulaire. Ouvrir un document de GRAFCET les ouvre aussi (avec *Start a GEMMA* quand il n'en a pas). Pour le document actif, *Edit > GEMMA editor...* et *Edit > GEMMA graph...* les ouvrent (ainsi que *Show the graph* dans l'éditeur).

Les deux fenêtres sont ancrées dans la fenêtre principale, pour voir en même temps la GEMMA et ses GRAFCET : quand l'une s'ouvre, elle est découpée dans le panneau des réseaux : l'éditeur de GEMMA à gauche, sur toute la hauteur ; le graphe de la GEMMA entre lui et les réseaux ; Safety, Control et Production restent des onglets du même panneau, à droite. Les autres fenêtres gardent leur place. Chaque panneau se déplace ailleurs en glissant son onglet, comme toute fenêtre de l'éditeur, et y revient quand il est rouvert ; *View > Reset the layout of the windows* remet tout en place. Quand l'éditeur est étroit, ses procédures sont au-dessus de ses liaisons au lieu d'être à côté. Sur le graphe, la molette zoome autour de la souris, le bouton droit ou du milieu fait glisser le formulaire, *Fit* le montre à nouveau en entier.

```sh
./build/TimedPetriNetEditor examples/GRAFCET/GEMMA-Bottling/GEMMA-Bottling.json   # les fenêtres GEMMA s'ouvrent avec
```

![L'éditeur de GEMMA](pics/10-GEMMA-Window.png)

*La GEMMA de la ligne d'embouteillage du §5.3. À gauche l'éditeur : les 16 procédures du guide, 13 gardées, puis les 17 liaisons avec leurs conditions. Au milieu le graphe de la GEMMA, à sa droite les trois GRAFCET faits à partir d'elle, en onglets.*

| Partie | Rôle |
|------|--------------|
| *Names: English / Français* | les noms des procédures, dans la fenêtre et dans les légendes des étapes de Control |
| à gauche : A, F, D | les 16 procédures du guide. La case garde la procédure ; la survoler dit à quoi elle sert ; un clic la sélectionne |
| en dessous | la procédure sélectionnée : ce que fait Production tant qu'elle est active (*runs* : tourne, *is in its initial situation* `{init}`, *is frozen* `{*}` : figée), et ses problèmes. À droite, les liaisons qui en partent sont vertes (sur une ligne verte), celles qui y arrivent bleues (sur une ligne bleue), les autres grisées ; un nouveau clic la désélectionne |
| *Links* | une ligne par liaison, c'est-à-dire une transition de Control : de, vers, et sa condition, écrite comme une réceptivité (`Start`, `[Tank >= 80]`, `Ack . /Jam`, `Production:X0`) ; **x** la supprime |
| *Links of the guide* | les flèches du formulaire ([GEMMA.drawio](../gemma/GEMMA.drawio)) entre deux procédures gardées, avec une condition de départ : un clic l'ajoute |
| *Another link* | toute autre liaison : choisissez ses deux bouts, *Add*, puis écrivez sa condition |
| *Checks* | ce qui est faux (rouge, erreurs) ou peut l'être (orange, avertissements) |
| *Make the GRAFCETs* | écrit les GRAFCET ; désactivé tant qu'il y a des erreurs |
| *Show the graph* | ouvre ou ferme la fenêtre de la GEMMA dessinée comme son formulaire, en lecture seule (ci-dessous) |

Les vérifications :

| Vérification | Type |
|-------|------|
| A1 est gardée (c'est l'étape initiale de Control) | erreur |
| chaque liaison a une condition, et c'est une réceptivité valide | erreur |
| chaque procédure gardée a une sortie, est atteinte depuis A1, et a un chemin de retour vers A1 (D1 est atteinte de partout : c'est l'arrêt d'urgence) | erreur |
| D1 est gardée | avertissement : sans elle, l'arrêt d'urgence force Control dans son étape initiale |
| une liaison va vers une procédure non gardée | avertissement : la liaison est ignorée |
| deux liaisons quittent une procédure avec la même condition | avertissement : les deux franchiraient ensemble, et Control aurait deux étapes actives |

### 5.2 Ce que fait l'éditeur

| GRAFCET | Fait à partir de la GEMMA |
|---------|---------------------|
| **Safety** | `0 Watch` (initiale) ──`EStop`──► `1 Emergency stop` ──`/EStop . Rearm`──► 0. L'étape 1 force Control dans l'étape de D1 (`Control{n}`) et Production dans sa situation initiale (`Production{init}`) |
| **Control** | le GMMA : une étape par procédure gardée, A1 d'abord (étape 0, initiale), nommée d'après son code (`A1 Arrêt dans l'état initial`) ; une transition par liaison, sa condition comme réceptivité ; chaque étape force Production comme choisi (`Production{init}`, `Production{*}`, rien quand elle tourne) |
| **Production** | faite seulement quand le document n'en a pas : `0 Wait` ──`Control:Xn`(F1)──► `1 Cycle (to complete)` ──`5s/X1`──► 0. C'est à vous de la compléter : le cycle normal, comme s'il était seul |

*Make the GRAFCETs* peut être pressé de nouveau après chaque modification de la GEMMA :

- **Safety** et **Control** sont refaits ; leurs positions viennent de *Arrange top-down*.
- **Production** est gardée. Les étapes de Control sont renumérotées, donc ses réceptivités (`Control:X4`) et ordres de forçage (`Control{4}`) sont changés vers le nouveau numéro de la même procédure.
- Les **actions** que vous avez ajoutées aux étapes de Control (un voyant en F1, une vanne en F2) vont à la nouvelle étape de la même procédure.
- Les entrées usuelles lues par les conditions prennent leur type sur le pupitre : `EStop` (arrêt d'urgence), `Rearm`, `Ack`, `Start`, `Stop`, `Pause` (boutons poussoirs). Les autres sont des interrupteurs jusqu'à ce que vous les changiez avec *Edit*.
- La GEMMA est enregistrée avec le document, dans sa partie `gemma` ([chapitre 15 §3.1](15-File-Formats.md#31-the-gemma)) : rouvrez le fichier (ses fenêtres GEMMA s'ouvrent avec), modifiez-la, refaites les GRAFCET.

La fenêtre **GEMMA** dessine la GEMMA comme le [formulaire](../gemma/GEMMA.pdf), avec les places et les tailles de [GEMMA.drawio](../gemma/GEMMA.drawio) : les zones A, F, D, la P.C. hors énergie et les flèches de sa mise en et hors énergie, le cadre de la production, les 16 rectangles (ceux qui ne sont pas gardés sont pâles ; A1 a le double cadre d'une étape initiale), et dans chaque rectangle gardé son étape de Control (`X6` pour F1). Une liaison du formulaire suit le tracé de sa flèche sur le formulaire, à angles droits ; une liaison en sens inverse prend le même tracé à rebours ; une autre liaison a son propre tracé, droit ou avec un ou deux coudes, autour des procédures gardées. Sa condition est écrite sur son plus long segment, à l'écart des autres conditions. D1 est aussi atteinte depuis tous les états par l'arrêt d'urgence de Safety (flèche rouge). C'est en lecture seule : un clic sur un rectangle sélectionne la procédure, dans les deux fenêtres : les liaisons qui en partent passent en vert, celles qui y arrivent en bleu. La souris dessus dit à quoi elle sert.

![La GEMMA dessinée comme son formulaire](pics/10-GEMMA-Graph.png)

*La GEMMA de la ligne d'embouteillage (§5.3) : F1 est l'étape X6 de Control, D3 l'étape X12.*

### 5.3 Tutoriel : une ligne d'embouteillage

L'exemple [`examples/GRAFCET/GEMMA-Bottling`](../../examples/GRAFCET/GEMMA-Bottling/README.md) a été fait avec la fenêtre. Une bouteille sous le bec est remplie jusqu'à ce qu'elle soit pleine, bouchée, et sortie par le convoyeur. Avant la production, la cuve est remplie ; après, elle est vidée.

![La ligne d'embouteillage](../../examples/GRAFCET/GEMMA-Bottling/GEMMA-Bottling.png)

Sa GEMMA garde 13 procédures :

| Procédure | Production | Dans cette machine |
|-----------|------------|-----------------|
| A1 | `{init}` | arrêtée, prête |
| F2 | `{init}` | préparation : `TankValve` remplit la cuve |
| F1 | tourne | production normale, `RunLamp` |
| A2 | tourne | `Stop` demandé : la bouteille en cours est terminée |
| F3 | `{init}` | clôture : `DrainValve` vide la cuve |
| A3, A4 | tourne, `{*}` | `Pause` : la bouteille est terminée, puis la ligne attend (`PauseLamp`) |
| F4 | `{init}` | manuel : `Jog` fait tourner le convoyeur, `FillByHand` la vanne |
| D1 | `{init}` | arrêt d'urgence, `FaultLamp` |
| A5, A6 | `{init}` | après une défaillance : `Ack`, puis plus de bouteille sous le bec |
| D2 | `{*}` | bourrage de la boucheuse : `Buzzer`, `FaultLamp` |
| D3 | tourne | marche dégradée : les bouteilles sortent sans bouchon |

et 17 liaisons :

```text
A1 ──Start──► F2 ──[Tank >= 80]──► F1 ──Stop──► A2 ──Production:X0──► F3 ──[Tank <= 5]──► A1
F1 ──Pause──► A3 ──Production:X0──► A4 ──Start──► F1
A1 ──Manual──► F4 ──/Manual──► A6 ──/Bottle──► A1
D1 ──Safety:X0──► A5 ──Ack──► A6
F1 ──Jam──► D2 ──Ack . /Jam──► A5        D2 ──Degraded . /Jam──► D3 ──Stop──► A2
```

`Production:X0` (la production est revenue dans son étape initiale) est la façon dont A2 et A3 attendent la fin de la bouteille. `Safety:X0` (la sécurité surveille de nouveau) laisse D1 continuer une fois l'arrêt d'urgence relâché et réarmé.

Production a ensuite été écrite à la main. Elle lit les étapes de F1 et D3 de Control (`Control:X6`, `Control:X12`) et choisit entre boucher ou non :

```text
X0 Wait for a bottle ──(Control:X6 + Control:X12) . Bottle──► X1 Fill (FillValve)
X1 ──[Level >= 100] . /Control:X12──► X2 Cap (Capper, CapTimer) ──2s/X2──► X3 Take the bottle out (Conveyor)
X1 ──[Level >= 100] . Control:X12───► X3 ──/Bottle──► X0
```

Les entrées : `EStop`, les boutons `Rearm`, `Ack`, `Start`, `Stop`, `Pause`, `Jog`, `FillByHand`, les interrupteurs `Manual`, `Degraded`, les capteurs `Bottle`, `Jam` (des interrupteurs que vous basculez), `Level` et `Tank` (des jauges : `Level` monte de 40 % par seconde tant que `FillValve` est ouverte, `Tank` de 20 % par seconde tant que `TankValve` est ouverte et descend de 4 % par seconde sinon). Les sorties montrent les nouveaux dessins du pupitre : le convoyeur (moteur), les vannes, une minuterie, les voyants et un buzzer.

Appuyez sur **Espace** :

| Vous faites | Ce qui se passe |
|--------|--------------|
| **Start** | Control en F2 : `TankValve` s'ouvre, la jauge `Tank` monte |
| attendez 4 s | à 80 %, F1 : `RunLamp` |
| basculez **Bottle** | Production X1 : `FillValve`, la jauge `Level` monte ; à 100 %, X2 : `Capper` et la minuterie `CapTimer` pendant 2 s ; puis X3 : le convoyeur tourne |
| rebasculez **Bottle** | Production X0 : elle attend la bouteille suivante |
| basculez **Jam** | D2 : Production figée, `Buzzer` et `FaultLamp` |
| basculez **Degraded**, rebasculez **Jam** | D3 : la bouteille suivante est remplie et sortie sans bouchon |
| **Stop** | A2, aussitôt F3 (la production est en X0) : `DrainValve` ; quand la cuve est sous 5 %, A1 |
| enfoncez **EStop** | D1 (forcé par Safety) ; Production revient en X0 |
| relâchez EStop, **Rearm** | A5 |
| **Ack** | A6, puis A1 quand aucune bouteille n'est sous le bec |
| basculez **Manual** (en A1) | F4 : maintenez **Jog** pour faire tourner le convoyeur, **FillByHand** pour remplir ; **Manual** rebasculé : A6, puis A1 |

Modifiez ensuite la GEMMA : par exemple gardez F5 (marche de vérification dans l'ordre), ajoutez ses liaisons du guide (A1 → F5 et F1 → F5, condition `StepByStep`), puis le retour que le formulaire ne dessine pas : *Another link* F5 → A1, condition `/StepByStep . Production:X0`. *Make the GRAFCETs*. Control a une étape de plus ; les réceptivités de Production qui lisent `Control:X6` et `Control:X12` suivent F1 et D3 vers leurs nouvelles étapes.

## 6. Pièges

- **Deux graphes forçant le même graphe différemment.** Les ordres de forçage s'appliquent dans l'ordre des graphes et des étapes : le dernier l'emporte. Un seul maître par graphe.
- **Un mode atteint via une étape transitoire.** La recherche de la situation stable peut traverser un mode sans exécuter ses actions ([chapitre 9 §12](09-GRAFCET.md#12-pitfalls)) : utilisez des fronts sur les boutons qui changent de mode.
- **Modifier Control à la main, puis refaire les GRAFCET.** Safety et Control sont refaits à partir de la GEMMA : une transition ou une étape ajoutée à la main est perdue (les actions des étapes sont gardées). Modifiez plutôt la GEMMA.
- **Deux liaisons qui quittent une procédure vraies ensemble.** Control aurait deux étapes actives (la vérification avertit quand les deux conditions sont le même texte, pas quand elles sont seulement vraies ensemble) : rendez-les exclusives (`Ack . /Jam` et `Degraded . /Jam`).
- **Oublier la remise en marche après l'arrêt d'urgence.** Après D1, la machine ne doit pas redémarrer toute seule : le séchoir demande `Rearm` (sécurité), puis `Ack` et une évacuation des caisses (A5, A6) avant le choix du mode.

## 7. Pour aller plus loin

- Le [formulaire GEMMA](../gemma/GEMMA.pdf) ([draw.io](../gemma/GEMMA.drawio)).
- NetProf, *Introduction au GEMMA* : <https://www.youtube.com/watch?v=y8lWIG71040> (en français), la source du séchoir.
- La [bibliographie](Bibliography.md) pour les cours sur le GRAFCET et la GEMMA.
- Faire tourner les GRAFCET du séchoir sur un Arduino : [chapitre 12](12-Code-Generation.md) ; les suivre depuis l'éditeur : [chapitre 13](13-Debugging.md).

---

Suite : [11. Le runtime](11-Runtime.md) — un réseau qui appelle les fonctions de votre programme.
