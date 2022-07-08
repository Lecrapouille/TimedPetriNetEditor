##=====================================================================
## TimedPetriNetEditor: A timed Petri net editor.
## Copyright 2021 -- 2022 Quentin Quadrat <lecrapouille@gmail.com>
##
## This file is part of TimedPetriNetEditor.
##
## PetriEditor is free software: you can redistribute it and/or modify it
## under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with GNU Emacs.  If not, see <http://www.gnu.org/licenses/>.
##=====================================================================

#module TimedPetriNetEditor

push!(LOAD_PATH, pwd())
using MaxPlus, SparseArrays

#export
#    PetriNet, Place, Transition, Arc,
#    petri_net, is_empty, clear!, editor!, editor, load!, save,
#    add_place!, remove_place!, places, count_places,
#    add_transition!, remove_transition!, transitions, count_transitions,
#    tokens, tokens!,
#    is_event_graph, canonic

# https://github.com/Lecrapouille/TimedPetriNetEditor
# make && make install
const libtpne = "/usr/lib/libtimedpetrineteditor.so"

"""
    PetriNet

Immutable structure holding the handle of a Petri net.
Since the Petri net comes from a C++ class, the handle is simply the
index of the container in which it is hold. This handle is never invalidated
even if the container changes of size.
"""
struct PetriNet
    handle::Int
end

"""
    Place

Immutable structure holding information of a Petri place (X-Y coordinate in the
window and number of tokens). Unique identifier is not given since the index of
the Place is the unique identifier.
"""
struct Place
    x::Float64
    y::Float64
    tokens::Int
end

"""
    Transition

Immutable structure holding information of a Petri transition (X-Y coordinate in the
window). Unique identifier is not given since the index of the Transition is the unique
identifier.
"""
struct Transition
    x::Float64
    y::Float64
end

"""
    Arc

Immutable structure holding information of a Petri Arc (origin and destination nodes and
duration). Node identifiers is a string "P0", "P1" .. for places and "T0", "T1" for
transitions.
"""
struct Arc
    from::String
    to::String
    duration::Float64
end

# Factorize the error message used when checking the validity of the Petri net handle
const throw_error() = error("Invalid Petri net handle. Please use handles returned by petri_net()")

"""
    petri_net

Create an empty Petri net and return its handle. You can create several nets.

# Examples
```julia-repl
julia> pn = petri_net()
PetriNet(0)

julia> typeof(pn)
PetriNet
```
"""
function petri_net()
    PetriNet(ccall((:petri_create, libtpne), Clonglong, ()))
end

"""
    petri_net

Load a Petri from a given JSON file and return its handle. You can create several nets.
Throw an exception if the Petri net handle cannot be loaded.

# Examples
```julia-repl
julia> pn = petri_net("TimedPetriNetEditor/examples/Example1.json")
PetriNet(0)

julia> typeof(pn)
PetriNet
```
"""
function petri_net(file::String)
    pn = petri_net()
    load!(pn, file)
    return pn
end

"""
    petri_net

Duplicate the Petri net. Return the new handle.
"""
function petri_net(pn::PetriNet)
    pn1 = ccall((:petri_copy, libtpne), Clonglong, (Clonglong,), pn.handle)
    (pn1 < 0) && throw_error()
    PetriNet(pn1)
end

"""
    is_empty

Check if the Petri net has no places and no transitions.
Throw an exception if the Petri net handle is invalid.
Return true is the net is empty, else return false.

# Examples
```julia-repl
julia> pn = petri_net()
PetriNet(0)

julia> is_empty(pn)
true
```
"""
function is_empty(pn::PetriNet)
    empty = Ref{Bool}(false)
    ccall((:petri_is_empty, libtpne), Bool, (Clonglong, Ref{Bool}),
          pn.handle, empty) || throw_error()
    return empty[]
end

"""
    clear!

Clear the Petri nets: remove all nodes and arcs.
Throw an exception if the Petri net handle is invalid.

# Examples
```julia-repl
julia> is_empty(pn)
false

julia> clear!(pn)
true

julia> is_empty(pn)
true
```
"""
function clear!(pn::PetriNet)
    ccall((:petri_reset, libtpne), Bool, (Clonglong,), pn.handle) || throw_error()
end

"""
    editor

Launch the GUI editor to edit the net graphically. Once presed ESCAPE key,
the editor will quit and modifications applied on the net. No cancel action
is possible!
Throw an exception if the Petri net handle is invalid.

# Examples
```julia-repl
julia> pn = petri_net()
PetriNet(0)

julia> editor!(pn)
true

julia> places(pn)
2-element Vector{Place}:
 Place(304.0, 266.0, 0)
 Place(607.0, 266.0, 0)
```
"""
function editor!(pn::PetriNet)
    ccall((:petri_editor, libtpne), Bool, (Clonglong,), pn.handle) || throw_error()
end

"""
    editor

Duplicate the Petri net and launch the GUI editor to edit graphically the duplicated net.
Once presed ESCAPE key, the editor will quit and modifications applied on the duplicated net
while the original net is not modified.
Throw an exception if the Petri net handle is invalid.
"""
function editor(pn::PetriNet)
    pn1 = petri_net(pn)
    ccall((:petri_editor, libtpne), Bool, (Clonglong,), pn1.handle) || throw_error()
    return pn1
end

"""
    add_place!

Add a new Place at coordinate X-Y and the number of tokens.
Throw an exception if the Petri net handle is invalid.
Return the identifier of the newly created place.

# Examples
```julia-repl
julia> pn = petri_net()
PetriNet(0)

julia> p0 = add_place!(pn, 3.15, 4.15, 5)
0

julia> p1 = add_place!(pn, 4.0, 5.0, 0)
1

julia> places(pn)
2-element Vector{Place}:
 Place(3.15, 4.15, 5)
 Place(4.0, 5.0, 0)
```
"""
function add_place!(pn::PetriNet, x::Float64, y::Float64, tokens::Int)
    (tokens < 0) && error("Number of tokens shall be >= 0")
    id = ccall((:petri_add_place, libtpne), Clonglong, (Clonglong, Cfloat, Cfloat, Clonglong),
                pn.handle, x, y, tokens)
    (id < 0) && throw_error()
    return id
end

"""
    remove_place!

Remove a place refered by its unique identifier.
Throw an exception if the Petri net handle or the palce identifier are invalid.

# Examples
```julia-repl
julia> pn = petri_net()
PetriNet(0)

# Petri with two Places
julia> p0 = add_place!(pn, 3.15, 4.15, 5)
0

julia> p1 = add_place!(pn, 4.0, 5.0, 0)
1

julia> places(pn)
2-element Vector{Place}:
 Place(3.15, 4.15, 5)
 Place(4.0, 5.0, 0)

# Remove place p0
julia> remove_place!(pn, p0)
true

# Place p1 is now Place p0
julia> places(pn)
1-element Vector{Place}:
 Place(4.0, 5.0, 0)

# Cannot remove Place p1 since its now p0!
julia> remove_place!(pn, p1)
false

# Cannot remove Place p1
julia> remove_place!(pn, p0)
true

# Empty Petri net
julia> places(pn)
Place[]
```
"""
function remove_place!(pn::PetriNet, id::Int)
    ccall((:petri_remove_place, libtpne), Bool, (Clonglong, Clonglong), pn.handle, id) ||
    error("Invalid Petri net handle or invalid Place identifier")
end

"""
    count_places

Return the number of places in the Petri net.
Throw an exception if the Petri net handle is invalid.

# Examples
```julia-repl
julia> pn = petri_net()
PetriNet(0)

julia> count_places(pn)
0
```
"""
function count_places(pn::PetriNet)
    count = ccall((:petri_count_places, libtpne), Clonglong, (Clonglong,), pn.handle)
    if (count < 0) throw_error() end
    return count
end

"""
    places

Return the list (column vector) of places in the Petri net.
Throw an exception if the Petri net handle is invalid.

# Examples
```julia-repl
julia> pn = petri_net()
PetriNet(0)

julia> add_place!(pn, 3.15, 4.15, 5)
0

julia> places(pn)
1-element Vector{Place}:
 Place(3.15, 4.15, 5)
```
"""
function places(pn::PetriNet)
    size = count_places(pn)
    if (size <= 0)
        return Vector{Place}(undef, 0)
    end

    list = Vector{Place}(undef, size)
    ccall((:petri_get_places, libtpne), Bool, (Clonglong, Ptr{Place}),
          pn.handle, list)
    list
end

"""
    tokens

Return the number of tokens in the given Place.
Throw an exception if the Petri net handle or the place identifier are invalid.

# Examples
```julia-repl
julia> pn = petri_net()
PetriNet(0)

julia> p0 = add_place!(pn, 3.15, 4.15, 5)
0

julia> tokens(pn, p0)
5
```
"""
function tokens(pn::PetriNet, place::Int)
    count = ccall((:petri_get_tokens, libtpne), Clonglong, (Clonglong, Clonglong), pn.handle, Clonglong(place))
    (count < 0) && error("Invalid Petri net handle or invalid Place identifier")
    return count
end

# TODO retourner les marquages https://youtu.be/F0tImMHObv0
# TODO retourner successeurs https://youtu.be/mN8XWiXyHyk

"""
    tokens!

Set the number of tokens in the given Place.
Throw an exception if the Petri net handle or the place identifier are invalid.

# Examples
```julia-repl
julia> pn = petri_net()
PetriNet(0)

julia> p0 = add_place!(pn, 3.15, 4.15, 5)
0

julia> tokens(pn, p0)
5

julia> tokens!(pn, p0, 10)
true

julia> tokens(pn, p0)
10
```
"""
function tokens!(pn::PetriNet, place::Int, tokens::Int)
    (tokens < 0) && error("Number of tokens shall be >= 0")
    ccall((:petri_set_tokens, libtpne), Bool, (Clonglong, Clonglong, Clonglong),
          pn.handle, Clonglong(place), Clonglong(tokens)) || throw_error()
end

"""
    add_transition!

Add a new Place at coordinate X-Y and the number of tokens.
Throw an exception if the Petri net handle is invalid.
Return the identifier of the place.

# Examples
```julia-repl
julia> pn = petri_net()
PetriNet(0)

julia> t0 = add_transition!(pn, 3.15, 4.15)
0

julia> t1 = add_transition!(pn, 4.0, 5.0)
1

julia> transitions(pn)
2-element Vector{Transition}:
 Transition(3.15, 4.15)
 Transition(4.0, 5.0)
```
"""
function add_transition!(pn::PetriNet, x::Float64, y::Float64)
    id = ccall((:petri_add_transition, libtpne), Clonglong, (Clonglong, Cfloat, Cfloat),
                pn.handle, Cfloat(x), Cfloat(y))
    (id < 0) && throw_error()
    return id
end

"""
    remove_transition!

Remove a transition refered by its unique identifier.
Throw an exception if the Petri net handle or the transition identifier are invalid.

# Examples
```julia-repl
julia> pn = petri_net()
PetriNet(0)

# Petri with two transitions
julia> t0 = add_transition!(pn, 3.15, 4.15)
0

julia> t1 = add_transition!(pn, 4.0, 5.0)
1

julia> transitions(pn)
2-element Vector{Transition}:
 Transition(3.15, 4.15)
 Transition(4.0, 5.0)

# Remove transition t0
julia> remove_transition!(pn, t0)
true

# Place t1 is now transition t0
julia> places(pn)
1-element Vector{Transition}:
 Transition(4.0, 5.0)

# Cannot remove transition t1 since its now t0!
julia> remove_transition!(pn, t1)
false

# Cannot remove transition t1
julia> remove_transition!(pn, t0)
true

# Empty Petri net
julia> transitions(pn)
Transition[]
```
"""
function remove_transition!(pn::PetriNet, id::Int)
    ccall((:petri_remove_transition, libtpne), Bool, (Clonglong, Clonglong), pn.handle, id) ||
    error("Invalid Petri net handle")
end

"""
    count_transitions

Return the number of transitions in the Petri net.
Throw an exception if the Petri net handle is invalid.

# Examples
```julia-repl
julia> pn = petri_net()
PetriNet(0)

julia> count_transitions(pn)
0
```
"""
function count_transitions(pn::PetriNet)
    count = ccall((:petri_count_transitions, libtpne), Clonglong, (Clonglong,), pn.handle)
    (count < 0) && error("Invalid Petri net handle")
    return count
end

"""
    transitions

Return the list of transitions in the Petri net.
Throw an exception if the Petri net handle is invalid.

# Examples
```julia-repl
julia> pn = petri_net()
PetriNet(0)

julia> t0 = add_transition!(pn, 4.0, 5.0)
0

julia> transitions(pn)
1-element Vector{Transition}:
 Transition(4.0, 5.0)
```
"""
function transitions(pn::PetriNet)
    size = count_transitions(pn)
    if (size <= 0)
        return Vector{Transition}(undef, 0)
    end

    list = Vector{Transition}(undef, size)
    ccall((:petri_get_transitions, libtpne), Bool, (Clonglong, Ptr{Transition}),
          pn.handle, list)
    list
end

# TODO arc

"""
    save

Save the Petri net to a json file.
Throw an exception if the Petri net handle is invalid or a failure occured during
the saving.

# Examples
```julia-repl
julia> pn = petri_net()
PetriNet(0)

julia> save(pn, "/home/qq/petri.json")
true
```
"""
function save(pn::PetriNet, file::String)
    ccall((:petri_save, libtpne), Bool, (Clonglong, Cstring), pn.handle, file) ||
    error("Invalid Petri net handle or failed saving in file")
end

"""
    load

Load the Petri net from a json file.
Throw an exception if the Petri net handle is invalid or a failure occured during
the loading.

# Examples
```julia-repl
julia> pn = petri_net()
PetriNet(0)

julia> load!(pn, "/home/qq/petri.json")
true
```
"""
function load!(pn::PetriNet, file::String)
    ccall((:petri_load, libtpne), Bool, (Clonglong, Cstring), pn.handle, file) ||
    error("Invalid Petri net handle or failed loading petri net from file")
end

"""
    load

Load the Petri net from a json file.
Throw an exception if the Petri net handle is invalid or a failure occured during
the loading.

# Examples
```julia-repl
julia> pn = load("/home/qq/petri.json")
PetriNet(0)
```
"""
function load(file::String)
    pn = petri_net()
    ccall((:petri_load, libtpne), Bool, (Clonglong, Cstring), pn.handle, file) ||
    error("Failed loading Petri net from file")
    pn
end

"""
    is_event_graph

Check if the Petri net is an event graph: if all places have a single input arc
and a single output arc.
Throw an exception if the Petri net handle is invalid.

# Examples
```julia-repl
julia> pn = petri_net()
PetriNet(0)

julia> is_event_graph(pn)
false
```
"""
function is_event_graph(pn::PetriNet)
    event_graph = Ref{Bool}(false)
    ccall((:petri_is_event_graph, libtpne), Bool, (Clonglong, Ref{Bool}),
          pn.handle, event_graph) || throw_error()
    return event_graph[]
end

"""
    canonic

If and only if the Petri net is an event graph, you can create its canonical form
meaning places will have at most one token.

# Examples
```julia-repl
julia> pn1 = petri_net("TimedPetriNetEditor/examples/Howard2.json")
PetriNet(0)

julia> is_event_graph(pn)
true

julia> pn2 = canonic(pn1)
PetriNet(1)
```
"""
function canonic(pn::PetriNet)
    id = ccall((:petri_to_canonical, libtpne), Clonglong, (Clonglong,), pn.handle)
    (id < 0) && throw_error()
    PetriNet(id)
end

# Helper struct for loading my C sparse matrix (data are Max-Plus values)
mutable struct CSparse
    i::Ptr{Csize_t}
    j::Ptr{Csize_t}
    d::Ptr{MP}
    size::Csize_t
    N::Csize_t
    M::Csize_t
    CSparse() = new(Ptr{Csize_t}(), Ptr{Csize_t}(), Ptr{Cdouble}(), 0, 0, 0)
end

# Helper Copy a C array to a Julia vector
function from_c_arr(p::Ref{T}, s::Csize_t) where T
   V = Vector{T}(undef, s)
   for i in 1:s
      V[i] = unsafe_load(p, i)
   end
   return V
end

"""
    to_graph

If and only if the Petri net is an event graph then generate two adjacency max-plus
matrices representing the net: the first matrix holds durations and the second holds
tokens. Values are type MP (for Max-Plus. See package `Max-Plus.jl`). These matrices
can be used and displayed with the package `SimpleGraphs.jl`.
Throw an exception if the Petri net handle is invalid or if the net is not an event
graph.

# Examples
```julia-repl
julia> pn = petri_net("examples/Howard2.json")
PetriNet(0)

julia> is_event_graph(pn)
true

julia> (N,T) = to_graph(pn);

julia> full(N)
4×4 Max-Plus dense matrix:
  .   .   1   .
  0   .   .   .
  .   0   .   0
  0   .   .   .

julia> full(T)
4×4 Max-Plus dense matrix:
  .   .   5   .
  5   .   .   .
  .   3   .   1
  1   .   .   .
```
"""
function to_graph(pn::PetriNet)
    N = Ref(CSparse())
    T = Ref(CSparse())

    ccall((:petri_to_adjacency_matrices, libtpne), Bool, (Clonglong, Ptr{CSparse}, Ptr{CSparse}),
           pn.handle, N, T) || error("Invalid Petri net handle")

    ns = N[].size
    ts = T[].size
    return (
        sparse(from_c_arr(N[].i, ns), from_c_arr(N[].j, ns), from_c_arr(N[].d, ns), N[].N, N[].M),
        sparse(from_c_arr(T[].i, ts), from_c_arr(N[].j, ts), from_c_arr(T[].d, ts), N[].N, N[].M)
   )
end

"""
    to_graph

If and only if the Petri net is an event graph then generate the implicit dynamic linear Max-Plus system.
State space representation:
    X(n) = D.X(n) ⨁ A.X(n-1) ⨁ B.U(n),
    Y(n) = C.X(n)
"""
function to_syslin(pn::PetriNet)
    A = Ref(CSparse())
    B = Ref(CSparse())
    C = Ref(CSparse())
    D = Ref(CSparse())

    ccall((:petri_to_sys_lin, libtpne), Bool, (Clonglong, Ptr{CSparse}, Ptr{CSparse}, Ptr{CSparse}, Ptr{CSparse}),
           pn.handle, D, A, B, C) || error("Invalid Petri net handle")

    as = A[].size; bs = B[].size; cs = C[].size; ds = D[].size;
    MPSysLin(
        sparse(from_c_arr(A[].i, as), from_c_arr(A[].j, as), from_c_arr(A[].d, as), A[].N, A[].M),
        sparse(from_c_arr(B[].i, bs), from_c_arr(B[].j, bs), from_c_arr(B[].d, bs), B[].N, B[].M),
        sparse(from_c_arr(C[].i, cs), from_c_arr(C[].j, cs), from_c_arr(C[].d, cs), C[].N, C[].M),
        sparse(from_c_arr(D[].i, ds), from_c_arr(D[].j, ds), from_c_arr(D[].d, ds), D[].N, D[].M)
    )
end

function dater(pn::PetriNet)
    ccall((:petri_dater_form, libtpne), Bool, (Clonglong,), pn.handle) || error("Invalid Petri net handle")
end

function counter(pn::PetriNet)
    ccall((:petri_counter_form, libtpne), Bool, (Clonglong,), pn.handle) || error("Invalid Petri net handle")
end

#end # TimedPetriNetEditor module
