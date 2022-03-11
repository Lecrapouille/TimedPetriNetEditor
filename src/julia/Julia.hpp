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
    double x; double y; int tokens;
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
extern "C" int petri_create();

// ****************************************************************************
//! \brief Duplicate the given Petri net.
//! \return the handle of the Petri net.
// ****************************************************************************
extern "C" int petri_copy(int const handle);

// ****************************************************************************
//! \brief Clear the petri net.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \return false if the handle is invalid or return true.
// ****************************************************************************
extern "C" bool petri_reset(int const handle);

// ****************************************************************************
//! \brief Is the Petri net empty ?
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \return true if the net is empty, else return false.
// ****************************************************************************
extern "C" bool petri_is_empty(int const handle, bool* empty);

// ****************************************************************************
//! \brief Call the GUI for editing the given Petri net.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \return false if the handle is invalid or the GUI cannot be started else
//! return true.
// ****************************************************************************
extern "C" bool petri_editor(int const handle);

// ****************************************************************************
//! \brief Add a new Place node at position (X,Y) and holding N tokens.
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \param[in] x: the X coordinate in the window.
//! \param[in] y: the Y coordinate in the window.
//! \param[in] tokens: the number of tokens hold by the place.
//! \return -1 if the handle is invalid or return the unique identifier of the
//! newly created place.
// ****************************************************************************
extern "C" int petri_add_place(int const handle, float const x, float const y, size_t const tokens);

// ****************************************************************************
//! \brief Remove the place refered by its unique identifier.
// ****************************************************************************
extern "C" bool petri_remove_place(int const handle, int const place);

// ****************************************************************************
//! \brief Return the number of places.
// ****************************************************************************
extern "C" int petri_count_places(int const handle);

// ****************************************************************************
//! \brief Return the list of places.
//! \param[out] list: a pre-allocated memory to store places. The size shall
//! have be set with \c petri_count_places().
// ****************************************************************************
extern "C" bool petri_get_places(int const handle, CPlace_t* list);

// ****************************************************************************
//! \brief Add a new Transition node at position (X,Y).
//! \param[in] pn: the handle of the petri net created by create_petri_net().
//! \param[in] x: the X coordinate in the window.
//! \param[in] y: the Y coordinate in the window.
//! \return -1 if the handle is invalid or return the unique identifier of the
//! newly created place.
// ****************************************************************************
extern "C" int petri_add_transition(int const handle, float const x, float const y);

// ****************************************************************************
//! \brief Return the number of transitions.
// ****************************************************************************
extern "C" int petri_count_transitions(int const handle);

// ****************************************************************************
//! \brief Return the list of transitions.
//! \param[out] list: a pre-allocated memory to store transitions. The size shall
//! have be set with \c petri_count_transitions().
// ****************************************************************************
extern "C" bool petri_get_transitions(int const handle, CTransition_t* transitions);

// ****************************************************************************
//! \brief Remove the transition refered by its unique identifier.
// ****************************************************************************
extern "C" bool petri_remove_transition(int const handle, int const transition);

// ****************************************************************************
//! \brief
// ****************************************************************************
extern "C" int petri_add_arc(int const handle, const char* from, const char* to);

// ****************************************************************************
//! \brief
// ****************************************************************************
extern "C" bool petri_remove_arc(int const handle, const char* from, const char* to);

// ****************************************************************************
//! \brief Get the number of tokens from the desired places.
// ****************************************************************************
extern "C" int petri_get_tokens(int const handle, int const id);

// ****************************************************************************
//! \brief Set the given number of tokens in the desired places.
// ****************************************************************************
extern "C" bool petri_set_tokens(int const handle, int const id, size_t const tokens);

// ****************************************************************************
//! \brief Save the petri net.
//! \return false if the handle is invalid or failed to save, else return true.
// ****************************************************************************
extern "C" bool petri_save(int const handle, const char* filepath);

// ****************************************************************************
//! \brief Load the petri net.
//! \return false if the handle is invalid or failed to load, else return true
// ****************************************************************************
extern "C" bool petri_load(int const handle, const char* filepath);

// ****************************************************************************
//! \brief
// ****************************************************************************
extern "C" bool petri_is_event_graph(int const handle, bool* res);

// ****************************************************************************
//! \brief
//! \return the handle of the newly created Petri net.
// ****************************************************************************
extern "C" int petri_to_canonical(int const handle);

// ****************************************************************************
//! \brief
// ****************************************************************************
extern "C" bool petri_dater_form(int const handle);
extern "C" bool petri_counter_form(int const handle);

// ****************************************************************************
//! \brief
// ****************************************************************************
//extern "C" bool showCriticalCycle(int const handle);

// ****************************************************************************
//! \brief
// ****************************************************************************
extern "C" bool petri_to_adjacency_matrices(int const handle,
                                            CSparseMatrix_t* N,
                                            CSparseMatrix_t* T);

// ****************************************************************************
//! \brief
// ****************************************************************************
extern "C" bool petri_to_sys_lin(int const handle, CSparseMatrix_t* D,
                                 CSparseMatrix_t* A, CSparseMatrix_t* B,
                                 CSparseMatrix_t* C);

#endif
