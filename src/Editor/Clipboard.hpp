// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef CLIPBOARD_HPP
#  define CLIPBOARD_HPP

#  include "PetriNet/PetriNet.hpp"
#  include <vector>
#  include <map>

namespace tpne {

// *****************************************************************************
//! \brief Clipboard for copy-paste of Petri net subgraphs.
//! Stores copies of places, transitions and arcs that can be pasted elsewhere.
// *****************************************************************************
class Clipboard
{
public:

    //--------------------------------------------------------------------------
    //! \brief Copy a Place to the clipboard.
    //! \param[in] place The place to copy.
    //--------------------------------------------------------------------------
    void addPlace(Place const& place);

    //--------------------------------------------------------------------------
    //! \brief Copy a Transition to the clipboard.
    //! \param[in] transition The transition to copy.
    //--------------------------------------------------------------------------
    void addTransition(Transition const& transition);

    //--------------------------------------------------------------------------
    //! \brief Copy an Arc to the clipboard.
    //! \param[in] arc The arc to copy.
    //--------------------------------------------------------------------------
    void addArc(Arc const& arc);

    //--------------------------------------------------------------------------
    //! \brief Paste clipboard contents into a net at the given position.
    //! \param[in,out] target_net The net to paste into.
    //! \param[in] x Target X position for paste.
    //! \param[in] y Target Y position for paste.
    //! \param[out] created_nodes Output vector of newly created nodes.
    //--------------------------------------------------------------------------
    void paste(Net& target_net, float x, float y,
               std::vector<Node*>& created_nodes);

    //--------------------------------------------------------------------------
    //! \brief Clear all clipboard contents.
    //--------------------------------------------------------------------------
    void clear();

    //--------------------------------------------------------------------------
    //! \brief Check if the clipboard is empty.
    //! \return true if no places and no transitions are stored.
    //--------------------------------------------------------------------------
    bool empty() const;

    //--------------------------------------------------------------------------
    //! \brief Set the center point for calculating paste offsets.
    //! \param[in] x Center X coordinate.
    //! \param[in] y Center Y coordinate.
    //--------------------------------------------------------------------------
    void setCenter(float x, float y);

private:

    //! \brief Stored place data
    struct PlaceData
    {
        size_t id;
        std::string caption;
        float x;
        float y;
        size_t tokens;
    };

    //! \brief Stored transition data
    struct TransitionData
    {
        size_t id;
        std::string caption;
        float x;
        float y;
    };

    //! \brief Stored arc data
    struct ArcData
    {
        size_t from_id;
        bool from_is_place;
        size_t to_id;
        bool to_is_place;
        float duration;
    };

    std::vector<PlaceData> m_places;
    std::vector<TransitionData> m_transitions;
    std::vector<ArcData> m_arcs;
    float m_center_x = 0.0f;
    float m_center_y = 0.0f;
};

} // namespace tpne

#endif // CLIPBOARD_HPP
