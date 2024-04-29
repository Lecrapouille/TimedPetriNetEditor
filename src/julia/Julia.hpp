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

#ifndef PETRI_NET_EDITOR_JULIA_API_HPP
#  define PETRI_NET_EDITOR_JULIA_API_HPP

#  include <cstdint>
#  include <cstddef>

namespace tpne { class MaxPlus; }

// ****************************************************************************
//! \file Export C functions into shared library for Julia.
// ****************************************************************************

typedef struct CPlace
{
    double x; double y; int64_t tokens;
} CPlace_t;

typedef struct CTransition
{
    double x; double y;
} CTransition_t;

typedef struct CSparseMatrix
{
    size_t* i;
    size_t* j;
    tpne::MaxPlus* d;
    size_t size;
    size_t N;
    size_t M;
} CSparseMatrix_t;

// ****************************************************************************
//! \brief Create a new empty timed Petri net.
//! \return the handle of the Petri net needed by other functions.
// ****************************************************************************
extern "C" int64_t petri_create();

// ****************************************************************************
//! \brief Duplicate the given Petri net.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \return the handle of the new Petri net.
// ****************************************************************************
extern "C" int64_t petri_copy(int64_t const pn);

// ****************************************************************************
//! \brief Clear the petri net (remove all places, transitions and arcs).
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \return false if the Petri net handle is invalid or return true.
// ****************************************************************************
extern "C" bool petri_reset(int64_t const pn);

// ****************************************************************************
//! \brief Is the Petri net empty ?
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \param[out] empty: true if the net is empty, else return false.
//! \return false if the Petri net handle is invalid or return true.
// ****************************************************************************
extern "C" bool petri_is_empty(int64_t const pn, bool* empty);

// ****************************************************************************
//! \brief Call the GUI for editing the given Petri net.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \return false if the handle is invalid or the GUI cannot be started else
//! return true.
// ****************************************************************************
extern "C" bool petri_editor(int64_t const pn);

// ****************************************************************************
//! \brief Set the number of tokens for each places.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \param[in] tokens the list of tokens for each places (#P0, #P1 .. #Pn).
//! \note the size of list of tokens is not checked and shall be made by the
//! caller function.
//! \return false if the Petri net handle is invalid or return true.
// ****************************************************************************
extern "C" bool petri_set_marks(int64_t const pn, int64_t const* tokens);

// ****************************************************************************
//! \brief Get the number of tokens for each places.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \param[out] tokens the list of tokens for each places (#P0, #P1 .. #Pn).
//! \note the size of list of tokens is not checked and shall be made by the
//! caller function.
//! \return false if the Petri net handle is invalid or return true.
// ****************************************************************************
extern "C" bool petri_get_marks(int64_t const pn, int64_t* tokens);

// ****************************************************************************
//! \brief Add a new Place node at position (X,Y) and holding N tokens.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \param[in] x: the X coordinate in the window.
//! \param[in] y: the Y coordinate in the window.
//! \param[in] tokens: the number of tokens hold by the place.
//! \return -1 if the handle is invalid or return the unique identifier of the
//! newly created place.
// ****************************************************************************
extern "C" int64_t petri_add_place(int64_t const pn, double const x,
                                   double const y, int64_t const tokens);

// ****************************************************************************
//! \brief Remove the place refered by its unique identifier.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \param[in] place: the place unique identifier.
//! \return false if the Petri net handle or the place id are invalid else
//! return true.
// ****************************************************************************
extern "C" bool petri_remove_place(int64_t const pn, int64_t const place);

// ****************************************************************************
//! \brief Return the number of places.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \return -1 if the Petri net handle is invalid or return the number of places.
// ****************************************************************************
extern "C" int64_t petri_count_places(int64_t const pn);

// ****************************************************************************
//! \brief Return the list of places.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \param[out] list: a pre-allocated memory to store places. The size shall
//! have be set with \c petri_count_places().
//! \return false if the Petri net handle is invalid (\c list is undefined) else
//! return true.
// ****************************************************************************
extern "C" bool petri_get_places(int64_t const pn, CPlace_t* list);

// ****************************************************************************
//! \brief Return the ith place.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \param[in] id: unique place identifier.
//! \param[out] places: a pre-allocated memory to store places.
//! \return false if the Petri net handle is invalid (\c place is undefined)
//! else return true.
// ****************************************************************************
extern "C" bool petri_get_place(int64_t const pn, int64_t const id,
                                CPlace_t* places);

// ****************************************************************************
//! \brief Add a new Transition node at position (X,Y).
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \param[in] x: the X coordinate in the window.
//! \param[in] y: the Y coordinate in the window.
//! \return -1 if the handle is invalid or return the unique identifier of the
//! newly created place.
// ****************************************************************************
extern "C" int64_t petri_add_transition(int64_t const pn, double const x,
                                        double const y);

// ****************************************************************************
//! \brief Return the number of transitions.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \return -1 if the Petri net handle is invalid or return the number of
//! transitions.
// ****************************************************************************
extern "C" int64_t petri_count_transitions(int64_t const pn);

// ****************************************************************************
//! \brief Return the list of transitions.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \param[out] transitions: a pre-allocated memory to store transitions. The
//! size shall have be set with \c petri_count_transitions().
//! \return false if the Petri net handle is invalid (\c transitions is undefined)
//! else return true.
// ****************************************************************************
extern "C" bool petri_get_transitions(int64_t const pn, CTransition_t* transitions);

// ****************************************************************************
//! \brief Return the ith transition.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \param[in] id: unique transition identifier.
//! \param[out] transition: a pre-allocated memory to store the transition.
//! \return false if the Petri net handle is invalid (\c transition is undefined)
//! else return true.
// ****************************************************************************
extern "C" bool petri_get_transition(int64_t const pn, int64_t const id,
                                     CTransition_t* transition);

// ****************************************************************************
//! \brief Remove the transition refered by its unique identifier.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \param[in] transition: the place unique transition.
//! \return false if the Petri net handle is invalid or transition id is invalid
//! else return true.
//! \return false if the Petri net handle is invalid of invalid transition id.
// ****************************************************************************
extern "C" bool petri_remove_transition(int64_t const pn, int64_t const transition);

// ****************************************************************************
//! \brief Add a new arc linking either a place to a transition or a trnasition
//! to a place.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \param[in] from: name of the place or name of the transition as source node
//! (i.e. "P4" or "T10").
//! \param[in] from: name of the place or name of the transition as destination
//! node (i.e. "P4" or "T10").
//! \return -1 if Petri net handle is invalid or if one node name is invalid else
//! return the arc identifier.
// ****************************************************************************
extern "C" int64_t petri_add_arc(int64_t const pn, const char* from, const char* to);

// ****************************************************************************
//! \brief Remove an arc given its source and destination node names.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \param[in] from: name of the place or name of the transition as source node
//! (i.e. "P4" or "T10").
//! \param[in] from: name of the place or name of the transition as destination
//! node (i.e. "P4" or "T10").
//! \return false if the Petri net handle is invalid or invalid node name else
//! return true.
// ****************************************************************************
extern "C" bool petri_remove_arc(int64_t const pn, const char* from, const char* to);

// ****************************************************************************
//! \brief Get the number of tokens from the desired places.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \param[in] id: place identifier.
//! \return the number of tokens in the place.
// ****************************************************************************
extern "C" int64_t petri_get_tokens(int64_t const pn, int64_t const id);

// ****************************************************************************
//! \brief Set the given number of tokens in the desired places.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \param[in] id: place identifier.
//! \param[in] tokens: number of tokens.
//! \return false if the Petri net handle is invalid or return true.
// ****************************************************************************
extern "C" bool petri_set_tokens(int64_t const pn, int64_t const id,
                                 int64_t const tokens);

// ****************************************************************************
//! \brief Save the petri net.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \param[in] filepath: the file path to save.
//! \return false if the handle is invalid or failed to save, else return true.
// ****************************************************************************
extern "C" bool petri_save(int64_t const pn, const char* filepath);

// TODO petri_export_latex, petri_export_graphivz ...

// ****************************************************************************
//! \brief Load the petri net.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \param[in] filepath: the file path to load.
//! \return false if the handle is invalid or failed to load, else return true
// ****************************************************************************
extern "C" bool petri_load(int64_t const pn, const char* filepath);

// ****************************************************************************
//! \brief Is the Petri net is an event graph ?
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \param[out] res: true if the Petri net is an event graph, else false.
//! \return false if the Petri net handle is invalid or return true.
// ****************************************************************************
extern "C" bool petri_is_event_graph(int64_t const pn, bool* res);

// ****************************************************************************
//! \brief Transform the event graph as canonical (only one tokens by places).
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \return -1 is the Petri net is not an event graph else return the handle of
//! the newly created Petri net.
// ****************************************************************************
extern "C" int64_t petri_to_canonical(int64_t const pn);

// ****************************************************************************
//! \brief Display the event graph into its dater equation.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \param[in] use_caption: display node captions instead of node keys.
//! \param[in] maxplus_notation: display (max,+) notation instead of classical
//! algebra.
//! \return false if the Petri net handle is invalid or return true.
// ****************************************************************************
extern "C" bool petri_dater_equation(int64_t const pn, bool use_caption,
    bool maxplus_notation);

// ****************************************************************************
//! \brief Display the event graph into its counter equation.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \param[in] use_caption: display node captions instead of node keys.
//! \param[in] minplus_notation: display (min,+) notation instead of classical
//! algebra.
//! \return false if the Petri net handle is invalid or return true.
// ****************************************************************************
extern "C" bool petri_counter_equation(int64_t const pn, bool use_caption,
    bool minplus_notation);

// ****************************************************************************
//! \brief Show the critical cycle in the graph event.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \return false if the Petri net handle is invalid or return true.
// ****************************************************************************
//extern "C" bool showCriticalCycle(int64_t const pn);
//TODO get petri_get_critical_cycle(int64_t const pn);

// ****************************************************************************
//! \brief
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \return false if the Petri net handle is invalid or return true.
// ****************************************************************************
extern "C" bool petri_to_adjacency_matrices(int64_t const pn,
                                            CSparseMatrix_t* N,
                                            CSparseMatrix_t* T);

// ****************************************************************************
//! \brief Save the Graph event into a Max-Plus linear system (Ax+Bu, y=Cx).
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \return false if the Petri net handle is invalid or return true.
// ****************************************************************************
extern "C" bool petri_to_sys_lin(int64_t const pn, CSparseMatrix_t* D,
                                 CSparseMatrix_t* A, CSparseMatrix_t* B,
                                 CSparseMatrix_t* C);

#endif
