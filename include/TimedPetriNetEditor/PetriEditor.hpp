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

#ifndef PETRI_NET_EDITOR_HPP
#  define PETRI_NET_EDITOR_HPP

#  include <string>

namespace tpne {

// ****************************************************************************
//! \brief Graphical User interface for manipulating and simulating Petri net.
// ****************************************************************************
class PetriNetEditor
{
public:

    virtual ~PetriNetEditor() = default;

    //-------------------------------------------------------------------------
    //! \brief Starts the Petri net editor up, load the Petri file if given not
    //! empty. Then call the infinite loop of the GUI.
    //! \param[in] petri_file the path of the Petri net fil to load. Pass dummy
    //! string if you do not want to load a Petri net file.
    //-------------------------------------------------------------------------
    virtual void run(std::string const& petri_file) = 0;
};

} // namespace tpne

#endif
