//=====================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2022 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of TimedPetriNetEditor.
//
// TimedPetriNetEditor is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GNU Emacs.  If not, see <http://www.gnu.org/licenses/>.
//=====================================================================

#include "Julia.hpp"
#include <iostream>
#include <memory>

//------------------------------------------------------------------------------
//! \brief List of Petri nets.
//------------------------------------------------------------------------------
static std::deque<std::unique_ptr<PetriNet>> g_petri_nets;

//------------------------------------------------------------------------------
//! \brief Check the validity of the Petri net handle (handle) and return v in
//! case of failure.
//------------------------------------------------------------------------------
#define SANITY_HANDLE(handle, v)                                        \
    if ((handle < 0) || (size_t(handle) >= g_petri_nets.size()))        \
    {                                                                   \
        std::cerr << "Unkown Petri net handle " << handle << std::endl; \
        return v;                                                       \
    }

//------------------------------------------------------------------------------
int petri_create()
{
    g_petri_nets.push_back(std::make_unique<PetriNet>());
    return int(g_petri_nets.size() - 1u);
}

//------------------------------------------------------------------------------
int petri_copy(int const handle)
{
    SANITY_HANDLE(handle, -1);

    g_petri_nets.push_back(std::make_unique<PetriNet>(*g_petri_nets[size_t(handle)]));
    return int(g_petri_nets.size() - 1u);
}

//------------------------------------------------------------------------------
bool petri_reset(int const handle)
{
    SANITY_HANDLE(handle, false);
    g_petri_nets[size_t(handle)]->reset();
    return true;
}

//------------------------------------------------------------------------------
bool petri_is_empty(int const handle, bool* empty)
{
    SANITY_HANDLE(handle, false);
    if (empty == nullptr)
        return false;

    *empty = g_petri_nets[size_t(handle)]->isEmpty();
    return true;
}

// -----------------------------------------------------------------------------
// Equivalent to the main() but separated to allow to export function and create
// shared library.
// -----------------------------------------------------------------------------
bool petri_editor(int const handle)
{
    SANITY_HANDLE(handle, false);

    Application application(800, 600, "Timed Petri Net Editor");
    PetriEditor editor(application.renderer(), *g_petri_nets[size_t(handle)]);
    editor.bgColor = sf::Color(255,255,255,255);

    try
    {
        application.push(editor);
        application.loop();
    }
    catch (std::string const& msg)
    {
        std::cerr << "Fatal: " << msg << std::endl;
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
int petri_count_places(int const handle)
{
    SANITY_HANDLE(handle, -1);
    return int(g_petri_nets[size_t(handle)]->places().size());
}

//------------------------------------------------------------------------------
bool petri_get_places(int const handle, CPlace_t* places)
{
    SANITY_HANDLE(handle, false);

    std::deque<Place> const& p = g_petri_nets[size_t(handle)]->places();
    size_t i = 0;
    for (auto const& it: p)
    {
        places[i].x = it.x;
        places[i].y = it.y;
        places[i].tokens = int(it.tokens);
        i += 1;
    }

    return true;
}

//------------------------------------------------------------------------------
int petri_add_place(int const handle, float const x, float const y, size_t const tokens)
{
    SANITY_HANDLE(handle, -1);

    Place& p = g_petri_nets[size_t(handle)]->addPlace(x, y, tokens);
    return int(p.id);
}

//------------------------------------------------------------------------------
int petri_add_transition(int const handle, float const x, float const y)
{
    SANITY_HANDLE(handle, -1);

    Transition& t = g_petri_nets[size_t(handle)]->addTransition(x, y);
    return int(t.id);
}

//------------------------------------------------------------------------------
int petri_count_transitions(int const handle)
{
    SANITY_HANDLE(handle, -1);
    return int(g_petri_nets[size_t(handle)]->transitions().size());
}

//------------------------------------------------------------------------------
bool petri_get_transitions(int const handle, CTransition_t* transitions)
{
    SANITY_HANDLE(handle, false);

    std::deque<Transition> const& t = g_petri_nets[size_t(handle)]->transitions();
    size_t i = 0;
    for (auto const& it: t)
    {
        transitions[i].x = it.x;
        transitions[i].y = it.y;
        i += 1;
    }

    return true;
}

//------------------------------------------------------------------------------
bool petri_remove_place(int const handle, int const id)
{
    SANITY_HANDLE(handle, false);

    if ((id < 0) || (size_t(id) >= g_petri_nets[size_t(handle)]->places().size()))
        return false;

    std::string strid = Place::to_str(size_t(id));
    Node* node = g_petri_nets[size_t(handle)]->findNode(strid);
    if (node == nullptr)
        return false;

    g_petri_nets[size_t(handle)]->removeNode(*node);
    return true;
}

//------------------------------------------------------------------------------
bool petri_remove_transition(int const handle, int const id)
{
    SANITY_HANDLE(handle, false);

    if ((id < 0) || (size_t(id) >= g_petri_nets[size_t(handle)]->transitions().size()))
        return false;

    std::string strid = Transition::to_str(size_t(id));
    Node* node = g_petri_nets[size_t(handle)]->findNode(strid);
    if (node == nullptr)
        return false;

    g_petri_nets[size_t(handle)]->removeNode(*node);
    return true;
}

//------------------------------------------------------------------------------
int petri_add_arc(int const handle,const char* from, const char* to, float const duration)
{
    SANITY_HANDLE(handle, -1);

    Node* node_from = g_petri_nets[size_t(handle)]->findNode(from);
    if (node_from == nullptr)
        return -1;

    Node* node_to = g_petri_nets[size_t(handle)]->findNode(to);
    if (node_to == nullptr)
        return -1;

    if (!g_petri_nets[size_t(handle)]->addArc(*node_from, *node_to, duration))
        return -1;

    return int(g_petri_nets[size_t(handle)]->arcs().size() - 1u);
}

//------------------------------------------------------------------------------
bool petri_remove_arc(int const handle, const char* from, const char* to)
{
    SANITY_HANDLE(handle, false);

    Node* node_from = g_petri_nets[size_t(handle)]->findNode(from);
    if (node_from == nullptr)
        return false;

    Node* node_to = g_petri_nets[size_t(handle)]->findNode(to);
    if (node_to == nullptr)
        return false;

    return g_petri_nets[size_t(handle)]->removeArc(*node_from, *node_to);
}

//------------------------------------------------------------------------------
int petri_get_tokens(int const handle, int const id)
{
    SANITY_HANDLE(handle, -1);

    auto const& places = g_petri_nets[size_t(handle)]->places();
    if ((id < 0) || (id > int(places.size())))
        return -1;
    return int(places[size_t(id)].tokens);
}

//------------------------------------------------------------------------------
bool petri_set_tokens(int const handle, int const id, size_t const tokens)
{
    SANITY_HANDLE(handle, false);

    auto& places = g_petri_nets[size_t(handle)]->places();
    if ((id < 0) || (id > int(places.size())))
        return -1;

    places[size_t(id)].tokens = tokens;
    return true;
}

//------------------------------------------------------------------------------
bool petri_save(int const handle, const char* filepath)
{
    SANITY_HANDLE(handle, false);

    return g_petri_nets[size_t(handle)]->save(filepath);
}

//------------------------------------------------------------------------------
bool petri_load(int const handle, const char* filepath)
{
    SANITY_HANDLE(handle, false);

    return g_petri_nets[size_t(handle)]->load(filepath);
}

//------------------------------------------------------------------------------
bool petri_is_event_graph(int const handle, bool* res)
{
    SANITY_HANDLE(handle, false);
    if (res == nullptr)
        return false;

    *res = g_petri_nets[size_t(handle)]->isEventGraph();
    return true;
}

//------------------------------------------------------------------------------
int petri_to_canonical(int const handle)
{
    SANITY_HANDLE(handle, -1);
    if (!g_petri_nets[size_t(handle)]->isEventGraph())
        return -1;

    int pn = petri_create();
    g_petri_nets[size_t(handle)]->toCanonicalForm(*g_petri_nets[size_t(pn)]);
    return pn;
}

//------------------------------------------------------------------------------
static void reference(SparseMatrix& org, CSparseMatrix_t* dst)
{
    dst->i = org.i.data();
    dst->j = org.j.data();
    dst->d = org.d.data();
    dst->size = org.d.size();
    dst->N = org.N;
    dst->M = org.M;
}

//------------------------------------------------------------------------------
bool petri_to_adjacency_matrices(int const handle, CSparseMatrix_t* pN, CSparseMatrix_t* pT)
{
    static SparseMatrix N;
    static SparseMatrix T;

    SANITY_HANDLE(handle, false);
    if ((pN == nullptr) || (pT == nullptr))
    {
        std::cerr << "Sanity check: NULL param" << std::endl;
        return false;
    }

    if (!g_petri_nets[size_t(handle)]->isEventGraph())
        return false;

    g_petri_nets[size_t(handle)]->toAdjacencyMatrices(N, T);
    reference(N, pN);
    reference(T, pT);
    return true;
}

//------------------------------------------------------------------------------
bool petri_to_sys_lin(int const handle, CSparseMatrix_t* pD, CSparseMatrix_t* pA,
                      CSparseMatrix_t* pB, CSparseMatrix_t* pC)
{
    static SparseMatrix D; static SparseMatrix A;
    static SparseMatrix B; static SparseMatrix C;

    SANITY_HANDLE(handle, false);
    if (!g_petri_nets[size_t(handle)]->isEventGraph())
        return false;

    g_petri_nets[size_t(handle)]->toSysLin(D, A, B, C);
    reference(D, pD);
    reference(A, pA);
    reference(B, pB);
    reference(C, pC);

    return true;
}

//------------------------------------------------------------------------------
bool petri_dater_form(int const handle)
{
    SANITY_HANDLE(handle, false);
    if (!g_petri_nets[size_t(handle)]->isEventGraph())
        return false;

    std::cout << g_petri_nets[size_t(handle)]->showDaterForm("").str() << std::endl;
    return true;
}

//------------------------------------------------------------------------------
bool petri_counter_form(int const handle)
{
    SANITY_HANDLE(handle, false);
    if (!g_petri_nets[size_t(handle)]->isEventGraph())
        return false;

    std::cout << g_petri_nets[size_t(handle)]->showCounterForm("").str() << std::endl;
    return true;
}
