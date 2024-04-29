using Suppressor, SparseArrays, MaxPlus

# Note: soon will included in MaxPlus.jl
include("TimedPetriNetEditor.jl")

# Create an empty Petri net and return its handle. You can create several nets.
pn = petri_net()
@assert pn.handle == 0

# Or create new Petri net by loading it.
pn1 = petri_net("../../data/examples/TrafficLights.json")
@assert pn1.handle == 1

# Duplicate the net
pn2 = petri_net(pn)
@assert pn2.handle == 2

# Failed loading file. FIXME shall not store pn3 internally
pn3 = petri_net("doesnotexist.json")
pn3 = petri_net(pn1)
@assert pn3.handle == 4 # FIXME shall be 3

# Has no places and no transitions? Return true in this case.
is_empty(pn)
@assert ans == true
is_empty(pn1)
@assert ans == false
is_empty(pn2)
@assert ans == true
is_empty(pn3)
@assert ans == false

# Clear the Petri net (remove all nodes and arcs)
clear!(pn3)
@assert is_empty(pn3) == true

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
is_empty(pn3)
@assert ans == false
save_petri(pn3, "/tmp/petri.json")
@assert ans == true
clear!(pn3)
@assert ans == true
is_empty(pn3)
@assert ans == true
# Load the same file back (old net is deleted)
load_petri!(pn3, "/tmp/petri.json")
@assert ans == true
is_empty(pn2)
@assert ans == true
save_petri(pn2, "/tmp/dummy.json")
@assert ans == true
load_petri!(pn3, "/tmp/dummy.json")
@assert ans == true
is_empty(pn3)
@assert ans == true

# Or create one
pn4 = load_petri("../../data/examples/Howard2.json")
@assert pn4.handle == 5 # FIXME should be ideally 4

# Number of nodes and arcs
count_places(pn4)
@assert ans == 5
count_transitions(pn4)
@assert ans == 4
# TODO count_arcs(pn4)
# @assert ans == 4

# Get the list of marks (number of tokens for each place P0, P1 .. Pn)
tokens(pn4)
@assert ans == [2; 0; 0; 0; 0]

# Check if Petri net is an event graph
is_event_graph(pn4)
@assert ans == true
is_event_graph(pn1)
@assert ans == false

# If the Petri net is an event graph, you can return the canonical form
pn5 = canonic(pn4)
@assert pn5.handle == 6 # FIXME should be ideally 5

# Show the counter and dater form
output = @capture_out begin
show_dater_equation(pn4, false, false)
end
@assert output == "Timed event graph represented as dater equation:\nT0(n) = max(1 + T3(n), 5 + T1(n))\nT1(n) = max(3 + T2(n))\nT2(n) = max(5 + T0(n - 2))\nT3(n) = max(1 + T2(n))\n\n"

output = @capture_out begin
show_dater_equation(pn4, false, true)
end
@assert output == "Timed event graph represented as dater equation (max-plus algebra):\nT0(n) = 1 T3(n) (+) 5 T1(n)\nT1(n) = 3 T2(n)\nT2(n) = 5 T0(n - 2)\nT3(n) = 1 T2(n)\n\n"

output = @capture_out begin
show_counter_equation(pn4, false, false)
end
@assert output == "Timed event graph represented as counter equation:\nT0(t) = min(T3(t - 1), T1(t - 5))\nT1(t) = min(T2(t - 3))\nT2(t) = min(2 + T0(t - 5))\nT3(t) = min(T2(t - 1))\n\n"

output = @capture_out begin
show_counter_equation(pn4, false, true)
end
@assert output == "Timed event graph represented as counter equation (min-plus algebra):\nT0(t) = T3(t - 1) (+) T1(t - 5)\nT1(t) = T2(t - 3)\nT2(t) = 2 T0(t - 5)\nT3(t) = T2(t - 1)\n\n"

# If the Petri net is an event graph, you can generate the graph the (max,+)
# adjacency sparse matrices (that could be used with SimpleGraphs.jl).
N,T = to_graph(pn4);
@assert full(N) == [mp0 mp0 2 mp0; 0 mp0 mp0 mp0; mp0 0 mp0 0; 0 mp0 mp0 mp0]
@assert full(T) == [mp0 mp0 5 mp0; 5 mp0 mp0 mp0; mp0 3 mp0 1; 1 mp0 mp0 mp0]

# If the Petri net is an event graph, you can generate the implicit dynamic
# linear (max,+) system.
S = to_syslin(pn4)
@assert full(S.D) == [mp0 5 mp0 1 mp0; mp0 mp0 3 mp0 mp0; mp0 mp0 mp0 mp0 mp0; mp0 mp0 1 mp0 mp0; mp0 mp0 mp0 mp0 mp0]
@assert full(S.A) == [mp0 mp0 mp0 mp0 mp0; mp0 mp0 mp0 mp0 mp0; mp0 mp0 mp0 mp0 5; mp0 mp0 mp0 mp0 mp0; 0 mp0 mp0 mp0 mp0]
@assert full(S.B) == reshape([], 5, 0)
@assert full(S.C) == reshape([], 0, 5)
@assert full(S.x0) == reshape([mp0; mp0; mp0; mp0; mp0], 5, 1)

# Modify the number of tokens for each place
tokens!(pn4, [0; 1; 2; 3; 4])
@assert ans == true
tokens(pn4)
@assert ans == [0; 1; 2; 3; 4]

pn6 = load_petri("../../data/examples/JPQ.json")
@assert pn6.handle == 7
is_event_graph(pn6)
@assert ans == true

S = to_syslin(pn6)
@assert full(S.D) == [mp0 mp0; mp0 mp0]
@assert full(S.A) == [MP(3) 7; 2 4]
@assert full(S.B) == reshape([mp0; 1], 2, 1)
@assert full(S.C) == reshape([MP(3) mp0], 1, 2)
@assert full(S.x0) == reshape([mp0; mp0], 2, 1)

### UNCOMMENT for manual tests

# Launch the GUI editor to edit the net graphically. Once pressed ESCAPE key,
# the editor will quit and modifications applied on the net.
#petri_editor!(pn6)
#@assert ans == true

# Safer version of editor!() because the Petri net is not modified but a new one
# is created based on the original net.
#pn3 = petri_editor(pn)
#@assert pn3.handle == 3
