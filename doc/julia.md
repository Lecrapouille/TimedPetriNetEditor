# Interface with Julia

*(The Julia API is still in gestation, API for arcs is missing).*

For [Julia](https://github.com/JuliaLang/julia) developers, I made an API, to
allows editing Petri nets either from function or allow to launch the graphical
editor.

To achieve it, first [MaxPlus](https://github.com/Lecrapouille/MaxPlus.jl) Julia
package (for the moment you need the `dev` branch still in gestation).

```julia
import Pkg; Pkg.add("MaxPlus")
```

Then install the TimedPetriNetEditor with `make install` (need sudo rights)
because the path of the shared library `libtimedpetrineteditor.so` has to be
known to be used by Julia.

From your Julia REPL (call Julia at the root of the TimedPetriNetEditor
repository), you can type this kind of code (consider this code as a cheatsheet):

```julia
using SparseArrays, MaxPlus

# Note: soon will included in MaxPlus.jl
include("src/julia/TimedPetriNetEditor.jl")

# Create an empty Petri net and return its handle. You can create several nets.
pn = petri_net()
@assert pn.handle == 0

# Or create new Petri net by loading it.
pn1 = petri_net("examples/TrafficLight.json")
@assert pn1.handle == 1

# Duplicate the net
pn2 = petri_net(pn)
@assert pn2.handle == 2

# Launch the GUI editor to edit the net graphically. Once pressed ESCAPE key,
# the editor will quit and modifications applied on the net.
editor!(pn)
@assert ans == true

# Clear the Petri net (remove all nodes and arcs)
clear!(pn)
@assert is_empty(pn) == true

# Safer version of editor!() because the Petri net is not modified but a new one
# is created based on the original net.
pn3 = editor(pn)
@assert pn3.handle == 3

# Has no places and no transitions? Return true in this case.
is_empty(pn1)
@assert ans == false
is_empty(pn2)
@assert ans == true

# Create places. X-Y coordinate (3.15, 4.15) and 5 tokens for Place 0.
# Return its identifier.
p0 = add_place!(pn, 100.0, 100.0, 5)
@assert ans == 0
p1 = add_place!(pn, 200.0, 200.0, 0)
@assert ans == 1
p2 = add_place!(pn, Place(210.0, 210.0, 10))
@assert ans == 2

# Get the place content
p3 = place(pn, p2)
@assert p3.x == 210.0
@assert p3.y == 210.0
@assert p3.tokens == 10

# Set/Get the number of tokens
tokens(pn, p0)
@assert ans == 5
tokens!(pn, p0, 2)
@assert ans == true
tokens(pn, p0)
@assert ans == 2

# Create transitions. X-Y coordinate (1.0, 2.0) for Transition 0.
# Return its identifier.
t0 = add_transition!(pn, 150.0, 150.0)
@assert ans == 0
t1 = add_transition!(pn, 250.0, 250.0)
@assert ans == 1
t2 = add_transition!(pn, Transition(200.0, 240.0))
@assert ans == 2

# Get the transition content
t3 = transition(pn, t2)
@assert t3.x == 200.0
@assert t3.y == 240.0

# Remove nodes. Be careful the handle of the latest inserted node is invalidated
remove_place!(pn, p1)
@assert ans == true
remove_transition!(pn, t0)
@assert ans == true

# Get the number of nodes
count_transitions(pn)
@assert ans == 2
count_places(pn)
@assert ans == 2

# Get the list of places
places(pn)
@assert size(ans) == (2,)

# Get the list of transitions
transitions(pn)
@assert size(ans) == (2,)

# TODO missing API for arcs :(

# You can save the Petri net to JSON file
# Note: you cannot save empty net
add_place!(pn3, 100.0, 100.0, 5)
save(pn3, "/home/qq/petri.json")
@assert ans == true

# Or load it (old net is deleted)
load!(pn, "examples/Howard2.json")
@assert ans == true

# Or create one
pn4 = load("examples/Howard2.json")
@assert pn4.handle == 4

# Get the list of marks (number of tokens for each place P0, P1 .. Pn)
tokens(pn)
@assert ans == [2; 0; 0; 0; 0]

# Modify the number of tokens for each place
tokens!(pn, [0; 1; 2; 3; 4])
@assert ans == true

# Check if Petri net is an event graph
is_event_graph(pn4)
@assert ans == true

# If the Petri net is an event graph, you can return the canonical form
pn5 = canonic(pn4)
@assert pn5.handle == 5

# Show the counter and dater form
counter(pn5)
@assert ans == true
dater(pn5)
@assert ans == true

# If the Petri net is an event graph, you can generate the graph the (max,+)
# adjacency sparse matrices (that could be used with SimpleGraphs.jl).
N,T = to_graph(pn4)

# Sparse to full (max,+) matrices
full(N)
full(T)

# If the Petri net is an event graph, you can generate the implicit dynamic
# linear (max,+) system.
S = to_syslin(pn5)
show(S.D)
show(S.A)
show(S.B)
show(S.C)
show(S.x0)

# For more interaction with the (max,+) algebra see tutorials on the repository
# of the MaxPlus package. For example, MP(3) * MP(2) will return MP(5).
```
