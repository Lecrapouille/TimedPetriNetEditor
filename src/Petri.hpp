//=====================================================================
// PetriEditor: A petri net editor.
// Copyright 2021 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of PetriEditor.
//
// PetriEditor is free software: you can redistribute it and/or modify it
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

#ifndef PETRI_HPP
#  define PETRI_HPP

#  include "GUI.hpp"
#  include <atomic>

class Petri: public GUI
{
public:

    Petri(Application& application);
    ~Petri();

private:

    //! \brief Inherit from GUI class. Draw the chessboard and pieces.
    virtual void draw(const float dt) override;

    //! \brief Inherit from GUI class. Update GUI.
    virtual void update(const float dt) override;

    //! \brief Inherit from GUI class. Manage mouse and keyboard events.
    virtual void handleInput() override;

    //! \brief Inherit from GUI class. Return true if GUI is alive.
    virtual bool isRunning() override;

    //! \brief Called when the GUI has been enabled.
    virtual void activate() override {}

    //! \brief Called when the GUI has been disabled.
    virtual void deactivate() override {}

    sf::CircleShape* m_body;
    sf::RectangleShape* m_arm;
    std::atomic<bool> m_running{true};
};

#endif
