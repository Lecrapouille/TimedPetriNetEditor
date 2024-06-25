//=============================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2023 Quentin Quadrat <lecrapouille@gmail.com>
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
//=============================================================================

#include "Julia.hpp"
#include "TimedPetriNetEditor/PetriNet.hpp"
#include "TimedPetriNetEditor/Algorithms.hpp"
#include "TimedPetriNetEditor/SparseMatrix.hpp"
#include "TimedPetriNetEditor/TropicalAlgebra.hpp"
#include "Editor.hpp" // Selected by Makefile
#include <iostream>
#include <deque>
#include <memory>

//------------------------------------------------------------------------------
//! \brief List of Petri nets.
//------------------------------------------------------------------------------
static std::deque<std::unique_ptr<tpne::Net>> g_petri_nets;

//------------------------------------------------------------------------------
//! \brief Check the validity of the Petri net handle (pn) and return an
//! error code in case of failure.
//------------------------------------------------------------------------------
#define CHECK_VALID_PETRI_HANDLE(pn, err_code)                                 \
    if ((pn < 0) || (size_t(pn) >= g_petri_nets.size()))                       \
    {                                                                          \
        std::cerr << "Unkown Petri net handle " << pn << std::endl;            \
        return err_code;                                                       \
    }

//------------------------------------------------------------------------------
//! \brief Check the is the Petri net is an event graph. In case of error print
//! erroneous arcs and return an error code.
//------------------------------------------------------------------------------
#define CHECK_IS_EVENT_GRAPH(pn, err_code)                                     \
    if (!isEventGraph(*g_petri_nets[size_t(pn)]))                              \
    {                                                                          \
        return err_code;                                                       \
    }

//------------------------------------------------------------------------------
int64_t petri_create()
{
    g_petri_nets.push_back(std::make_unique<tpne::Net>(tpne::TypeOfNet::TimedPetriNet));
    return int64_t(g_petri_nets.size() - 1u);
}

//------------------------------------------------------------------------------
int64_t petri_copy(int64_t const pn)
{
    CHECK_VALID_PETRI_HANDLE(pn, -1);

    g_petri_nets.push_back(std::make_unique<tpne::Net>(*g_petri_nets[size_t(pn)]));
    return int64_t(g_petri_nets.size() - 1u);
}

//------------------------------------------------------------------------------
bool petri_reset(int64_t const pn)
{
    CHECK_VALID_PETRI_HANDLE(pn, false);

    g_petri_nets[size_t(pn)]->clear();
    return true;
}

//------------------------------------------------------------------------------
bool petri_is_empty(int64_t const pn, bool* empty)
{
    CHECK_VALID_PETRI_HANDLE(pn, false);

    if (empty == nullptr)
        return false;

    *empty = g_petri_nets[size_t(pn)]->isEmpty();
    return true;
}

// -----------------------------------------------------------------------------
// Equivalent to the main() but separated to allow to export function and create
// shared library.
// -----------------------------------------------------------------------------
bool petri_editor(int64_t const pn)
{
    CHECK_VALID_PETRI_HANDLE(pn, false);

    tpne::Editor editor(1024, 768, "Petri Net Editor");

    try
    {
        editor.run(*g_petri_nets[size_t(pn)]);
        *g_petri_nets[size_t(pn)] = editor.net();
    }
    catch (std::string const& msg)
    {
        std::cerr << "Fatal: " << msg << std::endl;
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
int64_t petri_count_places(int64_t const pn)
{
    CHECK_VALID_PETRI_HANDLE(pn, -1);

    return int64_t(g_petri_nets[size_t(pn)]->places().size());
}

//------------------------------------------------------------------------------
bool petri_get_places(int64_t const pn, CPlace_t* places)
{
    CHECK_VALID_PETRI_HANDLE(pn, false);

    tpne::Net::Places const& p = g_petri_nets[size_t(pn)]->places();
    size_t i = 0;
    for (auto const& it: p)
    {
        places[i].x = it.x;
        places[i].y = it.y;
        places[i].tokens = int64_t(it.tokens);
        i += 1;
    }

    return true;
}

//------------------------------------------------------------------------------
bool petri_get_place(int64_t const pn, int64_t const i, CPlace_t* place)
{
    CHECK_VALID_PETRI_HANDLE(pn, false);

    tpne::Net::Places const& p = g_petri_nets[size_t(pn)]->places();
    if ((i < 0) || (size_t(i) >= p.size()))
    {
        std::cerr << "Unkown Place " << i << std::endl;
        return false;
    }

    place->x = p[i].x;
    place->y = p[i].y;
    place->tokens = int64_t(p[i].tokens);

    return true;
}

//------------------------------------------------------------------------------
int64_t petri_add_place(int64_t const pn, double const x, double const y,
                        int64_t const tokens)
{
    CHECK_VALID_PETRI_HANDLE(pn, -1);

    tpne::Place& p = g_petri_nets[size_t(pn)]->addPlace(float(x), float(y),
                                                      size_t(tokens));
    return int64_t(p.id);
}

//------------------------------------------------------------------------------
int64_t petri_add_transition(int64_t const pn, double const x, double const y)
{
    CHECK_VALID_PETRI_HANDLE(pn, -1);

    tpne::Transition& t = g_petri_nets[size_t(pn)]->addTransition(float(x), float(y));
    return int64_t(t.id);
}

//------------------------------------------------------------------------------
int64_t petri_count_transitions(int64_t const pn)
{
    CHECK_VALID_PETRI_HANDLE(pn, -1);

    return int64_t(g_petri_nets[size_t(pn)]->transitions().size());
}

//------------------------------------------------------------------------------
// FIXME: to be replaced by tpne::Net::setMarks()
bool petri_set_marks(int64_t const pn, int64_t const* tokens)
{
    CHECK_VALID_PETRI_HANDLE(pn, false);
    // FIXME unknown array size
    // tokens.size() == places.size()

    tpne::Net::Places& places = g_petri_nets[size_t(pn)]->places();
    size_t i = places.size();
    while (i--)
    {
        places[i].tokens = size_t(tokens[i]);
    }
    return true;
}

//------------------------------------------------------------------------------
bool petri_get_marks(int64_t const pn, int64_t* tokens)
{
    CHECK_VALID_PETRI_HANDLE(pn, false);

    tpne::Net::Places const& places = g_petri_nets[size_t(pn)]->places();
    size_t i = places.size();
    while (i--)
    {
        tokens[i] = size_t(places[i].tokens);
    }
    return true;
}

//------------------------------------------------------------------------------
bool petri_get_transitions(int64_t const pn, CTransition_t* transitions)
{
    CHECK_VALID_PETRI_HANDLE(pn, false);

    tpne::Net::Transitions const& t = g_petri_nets[size_t(pn)]->transitions();
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
bool petri_get_transition(int64_t const pn, int64_t const i, CTransition_t* transition)
{
    CHECK_VALID_PETRI_HANDLE(pn, false);

    tpne::Net::Transitions const& t = g_petri_nets[size_t(pn)]->transitions();
    if ((i < 0) || (size_t(i) >= t.size()))
    {
        std::cerr << "Unkown Transition " << i << std::endl;
        return false;
    }

    transition->x = t[i].x;
    transition->y = t[i].y;

    return true;
}

//------------------------------------------------------------------------------
bool petri_remove_place(int64_t const pn, int64_t const id)
{
    CHECK_VALID_PETRI_HANDLE(pn, false);

    if ((id < 0) || (size_t(id) >= g_petri_nets[size_t(pn)]->places().size()))
        return false;

    std::string strid = tpne::Place::to_str(size_t(id));
    tpne::Node* node = g_petri_nets[size_t(pn)]->findNode(strid);
    if (node == nullptr)
        return false;

    g_petri_nets[size_t(pn)]->removeNode(*node);
    return true;
}

//------------------------------------------------------------------------------
bool petri_remove_transition(int64_t const pn, int64_t const id)
{
    CHECK_VALID_PETRI_HANDLE(pn, false);

    if ((id < 0) || (size_t(id) >= g_petri_nets[size_t(pn)]->transitions().size()))
        return false;

    std::string strid = tpne::Transition::to_str(size_t(id));
    tpne::Node* node = g_petri_nets[size_t(pn)]->findNode(strid);
    if (node == nullptr)
        return false;

    g_petri_nets[size_t(pn)]->removeNode(*node);
    return true;
}

//------------------------------------------------------------------------------
int64_t petri_add_arc(int64_t const pn,const char* from, const char* to,
                      double const duration)
{
    CHECK_VALID_PETRI_HANDLE(pn, -1);

    tpne::Node* node_from = g_petri_nets[size_t(pn)]->findNode(from);
    if (node_from == nullptr)
        return -1;

    tpne::Node* node_to = g_petri_nets[size_t(pn)]->findNode(to);
    if (node_to == nullptr)
        return -1;

    if (!g_petri_nets[size_t(pn)]->addArc(*node_from, *node_to,
                                              float(duration)))
        return -1;

    return int64_t(g_petri_nets[size_t(pn)]->arcs().size() - 1u);
}

//------------------------------------------------------------------------------
bool petri_remove_arc(int64_t const pn, const char* from, const char* to)
{
    CHECK_VALID_PETRI_HANDLE(pn, false);

    tpne::Node* node_from = g_petri_nets[size_t(pn)]->findNode(from);
    if (node_from == nullptr)
        return false;

    tpne::Node* node_to = g_petri_nets[size_t(pn)]->findNode(to);
    if (node_to == nullptr)
        return false;

    return g_petri_nets[size_t(pn)]->removeArc(*node_from, *node_to);
}

//------------------------------------------------------------------------------
int64_t petri_get_tokens(int64_t const pn, int64_t const id)
{
    CHECK_VALID_PETRI_HANDLE(pn, -1);

    auto const& places = g_petri_nets[size_t(pn)]->places();
    if ((id < 0) || (id > int64_t(places.size())))
        return -1;
    return int64_t(places[size_t(id)].tokens);
}

//------------------------------------------------------------------------------
bool petri_set_tokens(int64_t const pn, int64_t const id, int64_t const tokens)
{
    CHECK_VALID_PETRI_HANDLE(pn, false);

    auto& places = g_petri_nets[size_t(pn)]->places();
    if ((id < 0) || (id > int64_t(places.size())))
        return -1;

    places[size_t(id)].tokens = size_t(tokens);
    return true;
}

//------------------------------------------------------------------------------
bool petri_save(int64_t const pn, const char* filepath)
{
    CHECK_VALID_PETRI_HANDLE(pn, false);

    std::string err = tpne::saveToFile(*g_petri_nets[size_t(pn)], filepath);
    if (err.empty())
        return true;
    std::cerr << "Failed saving net to " << filepath << "Reason is '"
        << err << "'" << std::endl;
    return false;
}

//------------------------------------------------------------------------------
bool petri_load(int64_t const pn, const char* filepath)
{
    CHECK_VALID_PETRI_HANDLE(pn, false);

    bool springify = false;
    std::string err = tpne::loadFromFile(*g_petri_nets[size_t(pn)], filepath, springify);
    if (err.empty())
        return true;
    std::cerr << "Failed loading net from " << filepath << "Reason is '"
        << err << "'" << std::endl;
    return false;
}

//------------------------------------------------------------------------------
bool petri_is_event_graph(int64_t const pn, bool* res)
{
    CHECK_VALID_PETRI_HANDLE(pn, false);
    if (res == nullptr)
    {
        std::cerr << "Sanity check: NULL param" << std::endl;
        return false;
    }

    std::vector<tpne::Arc*> erroneous_arcs;
    std::string error;
    *res = isEventGraph(*g_petri_nets[size_t(pn)], error, erroneous_arcs);
    if (!error.empty())
        std::cerr << error << std::endl;
    return true;
}

//------------------------------------------------------------------------------
int64_t petri_to_canonical(int64_t const pn)
{
    CHECK_VALID_PETRI_HANDLE(pn, -1);
    CHECK_IS_EVENT_GRAPH(pn, -1);

    int64_t handle = petri_create();
    toCanonicalForm(*g_petri_nets[size_t(pn)], *g_petri_nets[size_t(handle)]);
    return handle;
}

//------------------------------------------------------------------------------
// Ok! Ok! Ugly but works since org are static variables
static void convert(tpne::SparseMatrix<tpne::MaxPlus>& org, CSparseMatrix_t* dst)
{
    dst->i = org.i.data();
    dst->j = org.j.data();
    dst->d = org.d.data();
    dst->size = org.d.size();
    dst->N = org.M;
    dst->M = org.N;
}

//------------------------------------------------------------------------------
bool petri_to_adjacency_matrices(int64_t const pn, CSparseMatrix_t* pN, CSparseMatrix_t* pT)
{
    static tpne::SparseMatrix<tpne::MaxPlus> N;
    static tpne::SparseMatrix<tpne::MaxPlus> T;

    if ((pN == nullptr) || (pT == nullptr))
    {
        std::cerr << "Sanity check: NULL param" << std::endl;
        return false;
    }
    CHECK_VALID_PETRI_HANDLE(pn, false);
    CHECK_IS_EVENT_GRAPH(pn, false);

    if (tpne::toAdjacencyMatrices(*g_petri_nets[size_t(pn)], N, T))
    convert(N, pN);
    convert(T, pT);
    return true;
}

//------------------------------------------------------------------------------
bool petri_to_sys_lin(int64_t const pn, CSparseMatrix_t* pD, CSparseMatrix_t* pA,
                      CSparseMatrix_t* pB, CSparseMatrix_t* pC)
{
    static tpne::SparseMatrix<tpne::MaxPlus> D;
    static tpne::SparseMatrix<tpne::MaxPlus> A;
    static tpne::SparseMatrix<tpne::MaxPlus> B;
    static tpne::SparseMatrix<tpne::MaxPlus> C;

    CHECK_VALID_PETRI_HANDLE(pn, false);
    CHECK_IS_EVENT_GRAPH(pn, false);

    D.clear(); A.clear();
    tpne::toSysLin(*g_petri_nets[size_t(pn)], D, A, B, C);
    convert(D, pD);
    convert(A, pA);
    convert(B, pB);
    convert(C, pC);

    return true;
}

//------------------------------------------------------------------------------
bool petri_dater_equation(int64_t const pn, bool use_caption, bool maxplus_notation)
{
    CHECK_VALID_PETRI_HANDLE(pn, false);
    CHECK_IS_EVENT_GRAPH(pn, false);

    std::cout
        << tpne::showDaterEquation(*g_petri_nets[size_t(pn)], "", use_caption, maxplus_notation).str()
        << std::endl;
    return true;
}

//------------------------------------------------------------------------------
bool petri_counter_equation(int64_t const pn, bool use_caption, bool minplus_notation)
{
    CHECK_VALID_PETRI_HANDLE(pn, false);
    CHECK_IS_EVENT_GRAPH(pn, false);

    std::cout
        << tpne::showCounterEquation(*g_petri_nets[size_t(pn)], "", use_caption, minplus_notation).str()
        << std::endl;
    return true;
}

// TODO
//------------------------------------------------------------------------------
//bool petri_semihoward(int64_t const pn)
//{}
