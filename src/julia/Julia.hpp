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

#ifndef API_HPP
#  define API_HPP

// ****************************************************************************
//! \file Export C functions into shared library for Julia.
// ****************************************************************************

#  include "PetriEditor.hpp"

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
    double* d;
    size_t size;
    size_t N;
    size_t M;
} CSparseMatrix_t;

// ****************************************************************************
//! \brief Create a new empty timed Petri net.
//! \return the handle of the Petri net.
// ****************************************************************************
extern "C" int64_t petri_create();

// ****************************************************************************
//! \brief Duplicate the given Petri net.
//! \return the handle of the Petri net.
// ****************************************************************************
extern "C" int64_t petri_copy(int64_t const handle);

// ****************************************************************************
//! \brief Clear the petri net.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \return false if the handle is invalid or return true.
// ****************************************************************************
extern "C" bool petri_reset(int64_t const handle);

// ****************************************************************************
//! \brief Is the Petri net empty ?
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \return true if the net is empty, else return false.
// ****************************************************************************
extern "C" bool petri_is_empty(int64_t const handle, bool* empty);

// ****************************************************************************
//! \brief Call the GUI for editing the given Petri net.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \return false if the handle is invalid or the GUI cannot be started else
//! return true.
// ****************************************************************************
extern "C" bool petri_editor(int64_t const handle);

// ****************************************************************************
//! \brief Set the number of tokens for each places.
//! \param[in] tokens the list of tokens for each places (#P0, #P1 .. #Pn).
//! \note the size of list of tokens is not checked and shall be made by the
//! caller function.
//! \return true if the net is valid.
// ****************************************************************************
extern "C" bool petri_set_marks(int64_t const handle, int64_t const* tokens);

// ****************************************************************************
//! \brief Get the number of tokens for each places.
//! \param[out] tokens the list of tokens for each places (#P0, #P1 .. #Pn).
//! \note the size of list of tokens is not checked and shall be made by the
//! caller function.
//! \return true if the net is valid.
// ****************************************************************************
extern "C" bool petri_get_marks(int64_t const handle, int64_t* tokens);

// ****************************************************************************
//! \brief Add a new Place node at position (X,Y) and holding N tokens.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \param[in] x: the X coordinate in the window.
//! \param[in] y: the Y coordinate in the window.
//! \param[in] tokens: the number of tokens hold by the place.
//! \return -1 if the handle is invalid or return the unique identifier of the
//! newly created place.
// ****************************************************************************
extern "C" int64_t petri_add_place(int64_t const handle, double const x,
                                   double const y, int64_t const tokens);

// ****************************************************************************
//! \brief Remove the place refered by its unique identifier.
// ****************************************************************************
extern "C" bool petri_remove_place(int64_t const handle, int64_t const place);

// ****************************************************************************
//! \brief Return the number of places.
// ****************************************************************************
extern "C" int64_t petri_count_places(int64_t const handle);

// ****************************************************************************
//! \brief Return the list of places.
//! \param[out] list: a pre-allocated memory to store places. The size shall
//! have be set with \c petri_count_places().
// ****************************************************************************
extern "C" bool petri_get_places(int64_t const handle, CPlace_t* list);

// ****************************************************************************
//! \brief Return the ith transition.
//! \param[out] transition: a pre-allocated memory to store the transition.
// ****************************************************************************
extern "C" bool petri_get_place(int64_t const handle, int64_t const i,
                                CPlace_t* place);

// ****************************************************************************
//! \brief Add a new Transition node at position (X,Y).
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \param[in] x: the X coordinate in the window.
//! \param[in] y: the Y coordinate in the window.
//! \return -1 if the handle is invalid or return the unique identifier of the
//! newly created place.
// ****************************************************************************
extern "C" int64_t petri_add_transition(int64_t const handle, double const x,
                                        double const y);

// ****************************************************************************
//! \brief Return the number of transitions.
// ****************************************************************************
extern "C" int64_t petri_count_transitions(int64_t const handle);

// ****************************************************************************
//! \brief Return the list of transitions.
//! \param[out] list: a pre-allocated memory to store transitions. The size shall
//! have be set with \c petri_count_transitions().
// ****************************************************************************
extern "C" bool petri_get_transitions(int64_t const handle, CTransition_t* transitions);

// ****************************************************************************
//! \brief Return the ith transition.
//! \param[out] transition: a pre-allocated memory to store the transition.
// ****************************************************************************
extern "C" bool petri_get_transition(int64_t const handle, int64_t const i,
                                     CTransition_t* transition);

// ****************************************************************************
//! \brief Remove the transition refered by its unique identifier.
// ****************************************************************************
extern "C" bool petri_remove_transition(int64_t const handle, int64_t const transition);

// ****************************************************************************
//! \brief
// ****************************************************************************
extern "C" int64_t petri_add_arc(int64_t const handle, const char* from, const char* to);

// ****************************************************************************
//! \brief
// ****************************************************************************
extern "C" bool petri_remove_arc(int64_t const handle, const char* from, const char* to);

// ****************************************************************************
//! \brief Get the number of tokens from the desired places.
// ****************************************************************************
extern "C" int64_t petri_get_tokens(int64_t const handle, int64_t const id);

// ****************************************************************************
//! \brief Set the given number of tokens in the desired places.
// ****************************************************************************
extern "C" bool petri_set_tokens(int64_t const handle, int64_t const id,
                                 int64_t const tokens);

// ****************************************************************************
//! \brief Save the petri net.
//! \return false if the handle is invalid or failed to save, else return true.
// ****************************************************************************
extern "C" bool petri_save(int64_t const handle, const char* filepath);

// ****************************************************************************
//! \brief Load the petri net.
//! \return false if the handle is invalid or failed to load, else return true
// ****************************************************************************
extern "C" bool petri_load(int64_t const handle, const char* filepath);

// ****************************************************************************
//! \brief
// ****************************************************************************
extern "C" bool petri_is_event_graph(int64_t const handle, bool* res);

// ****************************************************************************
//! \brief
//! \return the handle of the newly created Petri net.
// ****************************************************************************
extern "C" int64_t petri_to_canonical(int64_t const handle);

// ****************************************************************************
//! \brief
// ****************************************************************************
extern "C" bool petri_dater_form(int64_t const handle);
extern "C" bool petri_counter_form(int64_t const handle);

// ****************************************************************************
//! \brief
// ****************************************************************************
//extern "C" bool showCriticalCycle(int64_t const handle);

// ****************************************************************************
//! \brief
// ****************************************************************************
extern "C" bool petri_to_adjacency_matrices(int64_t const handle,
                                            CSparseMatrix_t* N,
                                            CSparseMatrix_t* T);

// ****************************************************************************
//! \brief
// ****************************************************************************
extern "C" bool petri_to_sys_lin(int64_t const handle, CSparseMatrix_t* D,
                                 CSparseMatrix_t* A, CSparseMatrix_t* B,
                                 CSparseMatrix_t* C);

#endif
