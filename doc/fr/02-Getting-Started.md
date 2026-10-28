# 🚀 2. Premiers pas


[![fr](https://lecrapouille.github.io/icons/fr.png)](../fr/02-Getting-Started.md) [![en](https://lecrapouille.github.io/icons/en.png)](../en/02-Getting-Started.md)

**Ce que vous apprendrez**

- compiler et installer l’éditeur ;
- ouvrir un exemple et l’exécuter ;
- dessiner votre premier réseau de Petri à la souris, l’exécuter et l’enregistrer ;
- où continuer, selon ce que vous voulez faire.

**Prérequis** : [chapitre 1](01-Introduction.md) pour les mots place, transition, jeton, tir.

---

## 1. 📦 Compiler et installer

L’éditeur est écrit en C++20 avec [Dear ImGui](https://github.com/ocornut/imgui) et GLFW. [ZeroMQ](https://zeromq.org/) est requis pour le contrôle à distance ([chapitre 18](18-Remote-Control.md)) et le débogueur ([chapitre 13](13-Debugging.md)). Sans ZeroMQ, compilez avec `make TPNE_ZEROMQ=0`.

Il est compilé et testé sous Linux (la CI tourne sur Ubuntu), se compile sous macOS et devrait fonctionner sous Windows.

La CI le compile avec Emscripten et le publie sur GitHub Pages : 🌐 [TimedPetriNetEditor.html](https://lecrapouille.github.io/TimedPetriNetEditor/TimedPetriNetEditor.html). Les mêmes sources se compilent aussi pour [ExaequOS](https://www.exaequos.com), un fork POSIX d’Emscripten ([section 1.4](#14-construction-pour-exaequos)).

### 1.1 Paquets système

**Debian, Ubuntu** (les paquets de la CI) :

```sh
sudo apt-get install build-essential pkg-config libdw-dev libzmq3-dev libglfw3-dev \
    libx11-dev libxinerama-dev libxrandr-dev libxi-dev libxcursor-dev \
    libgl1-mesa-dev libglu1-mesa-dev libwayland-dev libxkbcommon-x11-dev
```

**Fedora** :

```sh
sudo dnf install gcc-c++ make pkgconf elfutils-devel zeromq-devel glfw-devel \
    libX11-devel libXinerama-devel libXrandr-devel libXi-devel libXcursor-devel \
    mesa-libGL-devel mesa-libGLU-devel
```

**macOS** (Homebrew) :

```sh
brew install glfw zeromq pkg-config
```

### 1.2 Compilation

```sh
git clone https://github.com/Lecrapouille/TimedPetriNetEditor --depth=1 --recursive
cd TimedPetriNetEditor/

make download-external-libs
make compile-external-libs

make -j"$(nproc --all)"
```

On obtient, dans `build/` :

| Fichier | Rôle |
|---------|------|
| `TimedPetriNetEditor` | l’éditeur |
| `tpne-check` | vérifie les propriétés des réseaux en terminal ou en CI ([chapitre 4](04-Analysis.md)) |
| `libTimedPetri*.so`, `.a` | les bibliothèques, pour vos programmes ([chapitre 16](16-Library.md)) |

En option : `sudo make install` les installe sur le système. Les exemples exécutés par le runtime se compilent avec `make -C examples` ([chapitre 11](11-Runtime.md)), les tests unitaires avec `make -C tests`.

### 1.3 Construction pour Emscripten

Installez Emscripten :

```sh
git clone https://github.com/emscripten-core/emsdk.git ~/emsdk
cd ~/emsdk && ./emsdk install latest && ./emsdk activate latest
source ~/emsdk/emsdk_env.sh
```

Pour compiler TimedPetriNetEditor :

```sh
make BUILD=build-web download-external-libs
emmake make BUILD=build-web compile-external-libs
emmake make BUILD=build-web -j"$(nproc)"
cd build-web && python3 -m http.server 8000
```

Note : utilisez `BUILD=build-web` pour ne pas réutiliser le dossier `build` de la compilation locale.

Pour lancer TimedPetriNetEditor dans le navigateur (par ex. Firefox) : `http://localhost:8000/TimedPetriNetEditor.html`.

### 1.4 Construction pour ExaequOS

ExaequOS est un fork POSIX d’Emscripten. Le compilateur est l’image Docker [`lecrapouille/exaequos:light`](https://hub.docker.com/r/lecrapouille/exaequos) ([comment l’image est construite](https://github.com/Lecrapouille/docker-exa)). Docker est le seul outil supplémentaire. L’image tourne avec n’importe quel identifiant utilisateur.

Créez le cache du compilateur avant la première exécution. Si le dossier manque, Docker le crée en root dans votre home :

```sh
mkdir -p ~/.cache/exaequos
docker run -it --rm \
    -u "$(id -u):$(id -g)" \
    -v "$PWD:$PWD" \
    -v "$HOME/.cache/exaequos:/opt/emscripten-exa/cache" \
    -p 127.0.0.1:7777:7777 \
    -w "$PWD" \
    lecrapouille/exaequos:light
```

L’image définit `EXAEQUOS`, donc les mêmes cibles que sous Linux se compilent pour ExaequOS. `BUILD=build-exa` laisse un `build/` natif intact :

```sh
make BUILD=build-exa download-external-libs
make BUILD=build-exa compile-external-libs
make BUILD=build-exa -j"$(nproc)"
make BUILD=build-exa install
```

`download-external-libs` installe `exa-wayland`, `glfw` et `raylib` dans `exapkgs/`. `make install` écrit l’éditeur dans `media/localhost/TimedPetriNetEditor/exa/` :

| Fichier | Rôle |
|---------|------|
| `TimedPetriNetEditor.js` | le programme |
| `TimedPetriNetEditor.wasm` | son WebAssembly |
| `exa.html` | la page qui charge le programme |

Le programme ne tourne que dans le bureau ExaequOS : il demande au bureau son numéro de processus, sa fenêtre et ses fichiers. Ouvert seul dans un navigateur, la page reste vide. C’est pourquoi GitHub Pages héberge la build Emscripten.

Pour l’exécuter, démarrez le serveur de fichiers du conteneur depuis le répertoire du projet, pour qu’il voie `media/`, sur le port 7777 publié ci-dessus :

```sh
node /opt/emscripten-exa/third_party/server/server.js &
```

Ouvrez [exaequos.com](https://www.exaequos.com), lancez la console Havoc et tapez :

```text
/media/localhost/TimedPetriNetEditor
```

La CI produit aussi cette build ([`.github/workflows/publish_web_version.yml`](../.github/workflows/publish_web_version.yml)) et conserve `media/localhost/TimedPetriNetEditor/exa/` comme artefact `TimedPetriNetEditor-exaequos` de l’exécution.

## 2. ▶️ Ouvrir un exemple

```sh
./build/TimedPetriNetEditor examples/Petri-Nets/FirstNet/FirstNet.json
```

![Le premier réseau](pics/02-FirstNet.png)

Au centre, le réseau : la place `Ready` contient un jeton, la transition `Fire` est verte car elle peut tirer. Au-dessus du réseau, sa **description** indique quoi faire : la plupart des exemples en ont une. À droite, l’**inspecteur** liste places, transitions et arcs. En bas, la console. La [référence de l’éditeur](14-Editor-Reference.md) décrit chaque fenêtre ; vous n’en avez pas besoin pour commencer.

Appuyez sur **Space** : la simulation démarre, le cadre du réseau devient vert et la console indique ce que vous pouvez faire. Cliquez sur `Fire` : le jeton quitte `Ready` et arrive dans `Done`. `Fire` n’est plus verte : plus rien ne peut se produire. Appuyez encore sur **Space** : la simulation s’arrête et le réseau revient au marquage initial.

![Le premier réseau en simulation](pics/02-FirstNet-Running.png)

Un exemple plus riche :

```sh
./build/TimedPetriNetEditor examples/Petri-Nets/Philosophers/Philosophers.json
```

Trois philosophes partagent une fourchette et deux couteaux. Appuyez sur **Space** et cliquez sur les transitions vertes : quand un philosophe mange, ceux qui ont besoin du même couvert attendent.

![Les philosophes en simulation](pics/02-Philosophers-Running.png)

## 3. ✏️ Tutoriel : dessiner votre premier réseau

*File > New > Petri net*. Il n’y a ni mode ni palette : l’action d’un bouton de souris dépend de ce qui est sous le curseur. **Le bouton gauche dessine, le droit déplace.** La barre d’état, en bas, indique toujours ce que ferait chaque bouton maintenant.

1. **Clic gauche** dans le vide : une place `P0` apparaît.
2. **Clic droit** dans le vide, à sa droite : une transition `T0` apparaît.
3. **Glisser-déposer gauche** de `P0` vers `T0` : un arc. Relâcher le glisser dans le vide et l’éditeur crée un nœud de l’autre type, déjà relié : glisser gauche de `T0` vers la droite et relâcher : une place `P1` et l’arc `T0 → P1`.
4. Mettez la souris sur `P0` et appuyez sur **+** : un jeton (**-** en retire un).
5. **Glisser-déposer droit** un nœud pour le déplacer. Glisser droit dans le vide déplace la vue ; la molette zoome ; **F** cadre le réseau dans sa fenêtre.
6. **Double clic** sur un nœud pour le renommer : écrivez `Ready` sur `P0`.

```text
   Ready          T0           P1
    (●)  ──────►  ▭  ──────►  ( )
```

Exécutez : **Space**, puis clic sur `T0`. Arrêtez : **Space**. Enregistrez : **Ctrl+S** (le fichier JSON est décrit au [chapitre 15](15-File-Formats.md)). **Ctrl+Z** annule tout, même un déplacement.

Fermez maintenant la boucle : glisser gauche depuis `P1` et relâcher dans le vide (une nouvelle transition `T1`), puis glisser gauche de `T1` vers `Ready`. Relancez : le jeton tourne indéfiniment, `T0` puis `T1` puis `T0` … Vous avez dessiné un cycle, la forme de toute machine qui répète son travail. (Un arc lie toujours une place et une transition : un glisser de `P1` vers `Ready` insérerait une transition entre eux).

## 4. 🔀 Essayer les autres types de réseaux

Changez le type du réseau dans la barre d’outils (la liste déroulante après la vitesse) :

- **Timed Petri net** : double clic sur un arc sortant d’une transition et donnez-lui une durée. Appuyez sur **Space** : les jetons voyagent seuls le long des arcs, sans clic ([chapitre 6](06-Timed-Petri-Nets.md)).
- **Timed event graph** : les places disparaissent dans les arcs ; le menu *Analysis* calcule la vitesse du réseau ([chapitre 7](07-Timed-Event-Graphs.md)).
- **GRAFCET** : les places deviennent des étapes, et vous écrivez des conditions sur les transitions ([chapitre 9](09-GRAFCET.md)).

Clic droit sur un arc ou une place d’un réseau de Petri : poids, arcs inhibiteurs et de remise à zéro, capacités ([chapitre 5](05-Extended-Petri-Nets.md)).

## 5. 🗺️ Où continuer

| Vous voulez | Lire | Ouvrir |
|-------------|------|--------|
| comprendre les réseaux de Petri | [3](03-Petri-Nets.md), [4](04-Analysis.md), [5](05-Extended-Petri-Nets.md) | [examples/Petri-Nets](../../examples/README.md#-petri-nets) |
| savoir à quelle vitesse va un système, et pourquoi | [6](06-Timed-Petri-Nets.md), [7](07-Timed-Event-Graphs.md), [8](08-Max-Plus-Algebra.md) | `examples/Flowshops/simple.flowshop` |
| commander une machine | [9](09-GRAFCET.md), [10](10-GEMMA.md) | `examples/GRAFCET/GEMMA-Dryer/GEMMA-Dryer.json` |
| piloter votre programme avec un réseau | [11](11-Runtime.md), [12](12-Code-Generation.md), [13](13-Debugging.md) | `examples/Runtime/PickAndPlace/PickAndPlace.json` |

Tous les exemples, avec leurs images, sont dans la [galerie](../../examples/README.md). **F1** ouvre l’aide de l’éditeur.

---

Suivant : [3. Réseaux de Petri](03-Petri-Nets.md) — la définition, la règle de tir et les schémas de modélisation.
