# 10. 🛑 GEMMA


[![fr](https://lecrapouille.github.io/icons/fr.png)](../fr/10-GEMMA.md) [![en](https://lecrapouille.github.io/icons/en.png)](../en/10-GEMMA.md)

**What you will learn**

- what a GEMMA is: the map of all the ways a machine starts, stops and recovers;
- its three families of procedures (A, F, D) and its rectangles;
- how a GEMMA becomes a hierarchy of GRAFCETs that force each other;
- to run a dryer driven by three GRAFCETs, from its control panel and from a program;
- to make the GRAFCETs from a GEMMA in the editor: keep the procedures, link them, check, make; then a bottling line with 13 procedures.

**Prerequisites**: [9. GRAFCET](09-GRAFCET.md), in particular the forcing orders (§7) and the documents with several graphs (§8).

---

## 1. Why a GEMMA

A GRAFCET describes the **normal cycle** of a machine well: wait for the start button, run, stop. But a real machine spends a large part of its life outside this cycle: it is switched on, it heats up, an operator runs it by hand, a sensor fails, someone hits the emergency stop, the parts left inside must be cleared before it can start again. If these cases are added one by one to the normal GRAFCET, it soon becomes a plate of spaghetti that nobody can check.

In 1981 the ADEPA (the French agency for the automation of production, the same one that worked on the GRAFCET) published the **GEMMA**: *Guide d'Étude des Modes de Marches et d'Arrêts*, the study guide of the run and stop modes. It is a form, one A3 page, that lists **all** the modes a machine may be in, and the ways from one to another. The designer goes through the form with the customer, keeps the modes the machine needs, crosses out the others and draws the arrows between them. Then the GEMMA is translated into GRAFCETs.

The blank form is in the repository: [GEMMA.pdf](../gemma/GEMMA.pdf), and editable with [draw.io](https://app.diagrams.net): [GEMMA.drawio](../gemma/GEMMA.drawio).

## 2. The three families of procedures

The form is split in three zones, one per family of **procedures** (modes). Each procedure is a rectangle with a code:

**A — Stop procedures** (*procédures d'Arrêt*): the machine is stopped, or is stopping, for a normal reason.

| Code | Procedure |
|------|-----------|
| A1 | Stop in the initial state: the machine is ready to start |
| A2 | Stop asked at the end of the cycle: the cycle goes on to its end |
| A3 | Stop asked in a given state (not the end of the cycle) |
| A4 | Stop obtained (the machine stopped in another state than the initial one) |
| A5 | Preparation of the restart after a failure (clear, repair, check) |
| A6 | Setting the operative part in its initial state |
| A7 | Setting the operative part in a given state |

**F — Operating procedures** (*procédures de Fonctionnement*): the machine works.

| Code | Procedure |
|------|-----------|
| F1 | Normal production: the reason the machine exists |
| F2 | Preparation run: heating, filling, before the production |
| F3 | Closing run: emptying, cleaning, after the production |
| F4 | Verification run out of order: each movement by hand, in any order |
| F5 | Verification run in order: the cycle step by step |
| F6 | Test run: calibration, tests of the sensors |

**D — Failure procedures** (*procédures en Défaillance*): something went wrong.

| Code | Procedure |
|------|-----------|
| D1 | Emergency stop: everything that is dangerous stops at once |
| D2 | Diagnosis and treatment of the failure |
| D3 | Production despite the failure (degraded mode) |

The rectangles inside the large frame of the form are "in production" (products are made), the others are not. Around them, the form also separates the states where the control part is **off** (no power) and **on**: the GEMMA starts at the power on.

## 3. From the GEMMA to GRAFCETs

The usual translation is a **hierarchy** of GRAFCETs, each one forcing the ones below it ([chapter 9 §7](09-GRAFCET.md#7-forcing-orders)):

```text
   ┌──────────────┐  forces   ┌──────────────────┐  forces / reads  ┌──────────────┐
   │ Safety       │ ────────► │ Control (modes)  │ ───────────────► │ Production   │
   │ D1           │ ────────────────────────────────────────────►   │ F1           │
   └──────────────┘           └──────────────────┘                  └──────────────┘
```

1. **Safety** (the highest level): watches the emergency stop. While it is pushed, and until the operator rearms, a step forces all the other graphs to their initial situation (D1).
2. **Control**, or modes: follows the rectangles A and F of the GEMMA. Each mode is a step; the transitions are the arrows of the form. The production graph reads its steps (`Control:X4`) or is forced by it.
3. **Production**: the normal cycle (F1), as simple as if nothing else existed.

Because forcing has priority over the evolution (the forced graph does not evolve), a higher graph is always obeyed: no transition of the production can fire while the safety forces it.

## 4. Tutorial: the GEMMA of a dryer

The example [`examples/GRAFCET/GEMMA-Dryer`](../../examples/GRAFCET/GEMMA-Dryer/README.md) is a dryer: a belt brings boxes into an oven, they dry, the belt takes them out. It comes from the GEMMA lessons of NetProf ([video](https://www.youtube.com/watch?v=y8lWIG71040), in French).

```sh
./build/TimedPetriNetEditor examples/GRAFCET/GEMMA-Dryer/GEMMA-Dryer.json
```

![GEMMA of a dryer](pics/10-GEMMA-Dryer.png)

The document holds three graphs, one tab each:

```text
  Safety      X0 forces Production{init} and Control{init}  ──/EStop . Rearm──►  X1 watch  ──EStop──► X0

  Control     X0 wait for the acknowledgement  ──↑Ack──►  X1 clear the boxes (N Left Motor)  ──3s/X1──►  X2 choose
              X2 ──Auto . ↑Ack──►  X3 heating (N Heat Oven)  ──[Temperature >= 80]──►  X4 normal production
              X2 ──Manual . ↑Ack──► X5 manual run: if Left: Left Motor, if Right: Right Motor, Production{init}
              X4 ──Manual . ↑Ack──► X5          X5 ──Auto . ↑Ack──► X1

  Production  X0 wait  ──Control:X4 . Start──►  X1 belt and oven  ──Stop──►  X2 belt and oven  ──3s/X2──►  X0
```

They can be read on the GEMMA form:

| Step | GEMMA procedure |
|------|-----------------|
| Safety:X0 | D1, emergency stop, until the rearm |
| Control:X0 | A5, preparation of the restart after the failure |
| Control:X1 | A6, setting in the initial state (clearing the boxes) |
| Control:X2 | A1, stop in the initial state: the mode is chosen here |
| Control:X3 | F2, preparation run (heating) |
| Control:X4 | F1 allowed: the production graph may start |
| Control:X5 | F4, verification run out of order (manual) |
| Production:X1, X2 | F1, normal production; X2 is the end of the cycle asked by Stop (A2) |

The file also keeps this GEMMA, in its `gemma` part: the procedures D1, A5, A6, A1, F2, F1, F4 and the links of the table, with the receptivities of Control. The GEMMA windows (§5) open with the file and show it. The GRAFCETs were drawn by hand, so the step `Xn` written in each procedure of the GEMMA graph is the one the editor would give (A1 first: X0), not the one of this Control; *Make the GRAFCETs* would replace Safety and Control by the ones of the editor.

The **inputs** are the desk of the dryer, each one of its own kind: the emergency stop `EStop` (`emergency`), the push buttons `Rearm`, `Ack`, `Start`, `Stop`, `Left`, `Right` (`button`), the selector `Mode` with the positions `Auto` and `Manual` (`selector`), and the sensor `Temperature` (`value`, from 20 to 120 °C, driven by the output `Heat Oven`: +6 °C per second while the oven heats, -1 °C per second otherwise). The **outputs** are the motors `Left Motor`, `Right Motor` and the heater `Heat Oven`.

### 4.1 Run it in the editor

Press **Space**.

![GEMMA running](pics/10-GEMMA-Dryer-running.png)

*`Safety:X0` is active: its two forcing orders (double frames) keep `Production` and `Control` in their initial steps. The control panel shows the emergency stop, the buttons, the selector on Auto and the thermometer.*

| You do | What happens |
|--------|--------------|
| press **Rearm** | `Safety` goes to X1: the forcing stops |
| press **Ack** | `Control:X1`: the left motor turns (it clears the boxes); 3 s later, `Control:X2` |
| the selector is on **Auto**: press **Ack** | `Control:X3`: the oven heats, the thermometer rises |
| wait about 10 s | at 80 °C, `Control:X4` |
| press **Start** | `Production:X1`: the belt (right motor) turns and the oven heats |
| press **Stop** | `Production:X2`: the timer `x / 3 s` counts under `3s/X2`, then `Production:X0` |
| push **EStop** | `Safety:X0`: everything goes back to its start; the motors stop |
| pull EStop back, **Rearm**, **Ack**, wait 3 s, selector on **Manual**, **Ack** | `Control:X5`, the manual mode |
| hold **Right** (a right click keeps it pressed) | the right motor turns only while the button is held |

Follow it on the *Timing diagram* tab (the steps of the three graphs, the buttons, the temperature as a curve, the motors), and in the console. A lower speed (**x0.5**) helps.

### 4.2 Run it from a program

The same scenario, run by the GRAFCET engine of the library without the editor (one cycle every 0.1 s, the temperature following the oven as in the control panel), prints the active steps and outputs after each action:

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

Three lines deserve a second look:

- **Start** (13.8 s): in one cycle, `Control` goes from X3 to X4 (the temperature was read at the previous cycle) *and* `Production` goes from X0 to X1, because its receptivity `Control:X4 . Start` becomes true during the search of the stable situation ([chapter 9 §3.1](09-GRAFCET.md#31-the-stable-situation)).
- **EStop pushed** (22.3 s): `Safety` is back in X0 at once, but the motors still turn during this cycle. The forcing orders are read from the situation at the beginning of the cycle, so the other graphs are forced at the next cycle (22.4 s), 0.1 s later. On a real machine the emergency stop also cuts the power of the actuators by wires: the GRAFCET only follows.
- **Start in manual mode**: nothing happens. `Control:X5` forces `Production{init}`, and `Production` needs `Control:X4` anyway.

The program is short: it loads the file with `importAllNetsFromJSON()`, gives the three nets to a `GrafcetEngine`, then calls `setInput()`, `step(0.1f)` and reads `outputs()`. See [chapter 16](16-Library.md) for the API.

## 5. Make your own GEMMA in the editor

The dryer of §4 was drawn by hand. The editor can also make the three GRAFCETs from the GEMMA itself: you keep the procedures, you give the conditions of the links, it writes Safety, Control and Production.

### 5.1 The two GEMMA windows

*File > New > GEMMA...* makes a new document from a small GEMMA (A1, F1, A2, D1, A5, A6 and their links), its three GRAFCETs, and opens two windows: the **GEMMA editor**, where the GEMMA is built, and the **GEMMA**, where it is drawn as its form. Opening a document of GRAFCETs opens them too (with *Start a GEMMA* when it has none). For the active document, *Edit > GEMMA editor...* and *Edit > GEMMA graph...* open them (and *Show the graph* in the editor).

The two windows are docked in the main window, so that the GEMMA and its GRAFCETs are seen at once: when one opens, it is cut from the pane of the nets: the GEMMA editor on the left, all the height; the GEMMA graph between it and the nets; Safety, Control and Production stay tabs of the same pane, on the right. The other windows keep their place. Each pane can be dragged by its tab to another place, like any window of the editor, and comes back there when opened again; *View > Reset the layout of the windows* puts all back. When the editor is narrow, its procedures are above its links instead of beside them. On the graph, the wheel zooms around the mouse, the right or middle button drags the form, *Fit* shows it whole again.

```sh
./build/TimedPetriNetEditor examples/GRAFCET/GEMMA-Bottling/GEMMA-Bottling.json   # the GEMMA windows open with it
```

![The GEMMA editor](pics/10-GEMMA-Window.png)

*The GEMMA of the bottling line of §5.3. On the left the editor: the 16 procedures of the guide, 13 kept, then the 17 links with their conditions. In the middle the GEMMA graph, on its right the three GRAFCETs made from it, as tabs.*

| Part | What it does |
|------|--------------|
| *Names: English / Français* | the names of the procedures, in the window and in the captions of the steps of Control |
| left: A, F, D | the 16 procedures of the guide. The box keeps the procedure; hovering it tells what it is for; a click selects it |
| under them | the procedure selected: what Production does while it is active (*runs*, *is in its initial situation* `{init}`, *is frozen* `{*}`), and its problems. On the right, the links leaving it are green (on a green row), the links arriving at it blue (on a blue row), the others grey; a click on it again unselects it |
| *Links* | one line per link, that is one transition of Control: from, to, and its condition, written as a receptivity (`Start`, `[Tank >= 80]`, `Ack . /Jam`, `Production:X0`); **x** removes it |
| *Links of the guide* | the arrows of the form ([GEMMA.drawio](../gemma/GEMMA.drawio)) between two procedures kept, with a condition to start from: one click adds it |
| *Another link* | any other link: choose its two ends, *Add*, then write its condition |
| *Checks* | what is wrong (red, errors) or may be wrong (orange, warnings) |
| *Make the GRAFCETs* | writes the GRAFCETs; disabled while there are errors |
| *Show the graph* | opens or closes the window of the GEMMA drawn as its form, read only (below) |

The checks:

| Check | Kind |
|-------|------|
| A1 is kept (it is the initial step of Control) | error |
| each link has a condition, and it is a valid receptivity | error |
| each procedure kept has a way out, is reached from A1, and has a way back to A1 (D1 is reached from everywhere: it is the emergency stop) | error |
| D1 is kept | warning: without it, the emergency stop forces Control to its initial step |
| a link goes to a procedure not kept | warning: the link is ignored |
| two links leave one procedure with the same condition | warning: the two would fire together, and Control would have two active steps |

### 5.2 What the editor makes

| GRAFCET | Made from the GEMMA |
|---------|---------------------|
| **Safety** | `0 Watch` (initial) ──`EStop`──► `1 Emergency stop` ──`/EStop . Rearm`──► 0. Step 1 forces Control to the step of D1 (`Control{n}`) and Production to its initial situation (`Production{init}`) |
| **Control** | the GMMA: one step per procedure kept, A1 first (step 0, initial), named after its code (`A1 Stop in the initial state`); one transition per link, its condition as receptivity; each step forces Production as chosen (`Production{init}`, `Production{*}`, nothing when it runs) |
| **Production** | made only when the document has none: `0 Wait` ──`Control:Xn`(F1)──► `1 Cycle (to complete)` ──`5s/X1`──► 0. It is yours to complete: the normal cycle, as if it were alone |

*Make the GRAFCETs* can be pressed again after each change of the GEMMA:

- **Safety** and **Control** are made again; their positions come from *Arrange top-down*.
- **Production** is kept. The steps of Control are numbered again, so its receptivities (`Control:X4`) and forcing orders (`Control{4}`) are changed to the new number of the same procedure.
- The **actions** you added to the steps of Control (a lamp in F1, a valve in F2) go to the new step of the same procedure.
- The usual inputs read by the conditions get their kind on the control panel: `EStop` (emergency stop), `Rearm`, `Ack`, `Start`, `Stop`, `Pause` (push buttons). The others are switches until you *Edit* them.
- The GEMMA is saved with the document, in its `gemma` part ([chapter 15 §3.1](15-File-Formats.md#31-the-gemma)): open the file again (its GEMMA windows open with it), change it, make the GRAFCETs again.

The window **GEMMA** draws the GEMMA as the [form](../gemma/GEMMA.pdf), with the places and sizes of [GEMMA.drawio](../gemma/GEMMA.drawio): the zones A, F, D, the control part off and the arrows of its power on and off, the frame of the production, the 16 rectangles (the ones not kept are faint; A1 has the double frame of an initial step), and in each kept one its step of Control (`X6` for F1). A link of the form follows the path of its arrow on the form, with right angles; a link the other way takes the same path backward; another link gets a path of its own, straight or with one or two bends, around the procedures kept. Its condition is written on its longest segment, away from the other conditions. D1 is also reached from every state by the emergency stop of Safety (red arrow). It is read only: a click on a rectangle selects the procedure, in both windows: the links leaving it turn green, the links arriving at it blue. The mouse on it tells what it is for.

![The GEMMA drawn as its form](pics/10-GEMMA-Graph.png)

*The GEMMA of the bottling line (§5.3): F1 is step X6 of Control, D3 step X12.*

### 5.3 Tutorial: a bottling line

The example [`examples/GRAFCET/GEMMA-Bottling`](../../examples/GRAFCET/GEMMA-Bottling/README.md) was made with the window. A bottle under the nozzle is filled until it is full, capped, and taken out by the conveyor. Before the production the tank is filled; after it the tank is emptied.

![The bottling line](../../examples/GRAFCET/GEMMA-Bottling/GEMMA-Bottling.png)

Its GEMMA keeps 13 procedures:

| Procedure | Production | In this machine |
|-----------|------------|-----------------|
| A1 | `{init}` | stopped, ready |
| F2 | `{init}` | preparation: `TankValve` fills the tank |
| F1 | runs | normal production, `RunLamp` |
| A2 | runs | `Stop` asked: the bottle under way is finished |
| F3 | `{init}` | closing: `DrainValve` empties the tank |
| A3, A4 | runs, `{*}` | `Pause`: the bottle is finished, then the line waits (`PauseLamp`) |
| F4 | `{init}` | manual: `Jog` runs the conveyor, `FillByHand` the valve |
| D1 | `{init}` | emergency stop, `FaultLamp` |
| A5, A6 | `{init}` | after a failure: `Ack`, then no bottle under the nozzle |
| D2 | `{*}` | jam of the capper: `Buzzer`, `FaultLamp` |
| D3 | runs | degraded: the bottles go out without cap |

and 17 links:

```text
A1 ──Start──► F2 ──[Tank >= 80]──► F1 ──Stop──► A2 ──Production:X0──► F3 ──[Tank <= 5]──► A1
F1 ──Pause──► A3 ──Production:X0──► A4 ──Start──► F1
A1 ──Manual──► F4 ──/Manual──► A6 ──/Bottle──► A1
D1 ──Safety:X0──► A5 ──Ack──► A6
F1 ──Jam──► D2 ──Ack . /Jam──► A5        D2 ──Degraded . /Jam──► D3 ──Stop──► A2
```

`Production:X0` (the production is back in its initial step) is how A2 and A3 wait for the end of the bottle. `Safety:X0` (the safety watches again) lets D1 go on once the emergency stop is released and rearmed.

Production was then written by hand. It reads the steps of F1 and D3 of Control (`Control:X6`, `Control:X12`) and chooses between capping and not:

```text
X0 Wait for a bottle ──(Control:X6 + Control:X12) . Bottle──► X1 Fill (FillValve)
X1 ──[Level >= 100] . /Control:X12──► X2 Cap (Capper, CapTimer) ──2s/X2──► X3 Take the bottle out (Conveyor)
X1 ──[Level >= 100] . Control:X12───► X3 ──/Bottle──► X0
```

The inputs: `EStop`, the buttons `Rearm`, `Ack`, `Start`, `Stop`, `Pause`, `Jog`, `FillByHand`, the switches `Manual`, `Degraded`, the sensors `Bottle`, `Jam` (switches you toggle), `Level` and `Tank` (gauges: `Level` rises by 40 % per second while `FillValve` is open, `Tank` by 20 % per second while `TankValve` is open and falls by 4 % per second otherwise). The outputs show the new drawings of the panel: the conveyor (motor), the valves, a timer, the lamps and a buzzer.

Press **Space**:

| You do | What happens |
|--------|--------------|
| **Start** | Control in F2: `TankValve` opens, the gauge `Tank` rises |
| wait 4 s | at 80 %, F1: `RunLamp` |
| switch **Bottle** on | Production X1: `FillValve`, the gauge `Level` rises; at 100 %, X2: `Capper` and the timer `CapTimer` for 2 s; then X3: the conveyor turns |
| switch **Bottle** off | Production X0: it waits for the next bottle |
| switch **Jam** on | D2: Production frozen, `Buzzer` and `FaultLamp` |
| switch **Degraded** on, **Jam** off | D3: the next bottle is filled and taken out without cap |
| **Stop** | A2, at once F3 (the production is in X0): `DrainValve`; when the tank is under 5 %, A1 |
| push **EStop** | D1 (forced by Safety); Production back to X0 |
| pull EStop back, **Rearm** | A5 |
| **Ack** | A6, then A1 when no bottle is under the nozzle |
| switch **Manual** on (in A1) | F4: hold **Jog** to run the conveyor, **FillByHand** to fill; **Manual** off: A6, then A1 |

Then change the GEMMA: for example keep F5 (verification run in order), add its links of the guide (A1 → F5 and F1 → F5, condition `StepByStep`), then the way back that the form does not draw: *Another link* F5 → A1, condition `/StepByStep . Production:X0`. *Make the GRAFCETs*. Control gets a step more; the receptivities of Production that read `Control:X6` and `Control:X12` follow F1 and D3 to their new steps.

## 6. Pitfalls

- **Two graphs forcing the same graph differently.** The forcing orders are applied in the order of the graphs and of the steps: the last one wins. Keep one master per graph.
- **A mode reached through a transient step.** The search of the stable situation may cross a mode without running its actions ([chapter 9 §12](09-GRAFCET.md#12-pitfalls)): use edges on the buttons that change the mode.
- **Changing Control by hand, then making the GRAFCETs again.** Safety and Control are made again from the GEMMA: a transition or a step added by hand is lost (the actions of the steps are kept). Change the GEMMA instead.
- **Two links that leave a procedure true together.** Control would have two active steps (the check warns when the two conditions are the same text, not when they are only true together): make them exclusive (`Ack . /Jam` and `Degraded . /Jam`).
- **Forgetting the restart after the emergency stop.** After D1 the machine must not restart by itself: the dryer asks `Rearm` (safety), then `Ack` and a clearing of the boxes (A5, A6) before the choice of the mode.

## 7. Going further

- The [GEMMA form](../gemma/GEMMA.pdf) ([draw.io](../gemma/GEMMA.drawio)).
- NetProf, *Introduction au GEMMA*: <https://www.youtube.com/watch?v=y8lWIG71040> (in French), the source of the dryer.
- The [bibliography](Bibliography.md) for the courses on GRAFCET and GEMMA.
- Run the GRAFCETs of the dryer on an Arduino: [chapter 12](12-Code-Generation.md); watch them run from the editor: [chapter 13](13-Debugging.md).

---

Next: [11. The runtime](11-Runtime.md) — a net that calls the functions of your program.
