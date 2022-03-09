//=====================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 Quentin Quadrat <lecrapouille@gmail.com>
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

#ifndef PETRIEDITOR_HPP
#  define PETRIEDITOR_HPP

#  include "utils/GUI.hpp"
#  include "utils/MessageBar.hpp"
#  include "utils/Animation.hpp"
#  include "PetriNet.hpp"

// *****************************************************************************
//! \brief Graphic representation of the Petri net using the SFML library.
// *****************************************************************************
class PetriEditor: public GUIStates
{
public:

    PetriEditor(sf::RenderWindow& render, PetriNet& net);
    ~PetriEditor();

private: // Derived from GUI

    //--------------------------------------------------------------------------
    //! \brief Inherit from GUI class. Draw the chessboard and pieces.
    //--------------------------------------------------------------------------
    virtual void draw() override;

    //--------------------------------------------------------------------------
    //! \brief Inherit from GUI class. Update GUI.
    //--------------------------------------------------------------------------
    virtual void update(const float dt) override;

    //--------------------------------------------------------------------------
    //! \brief Inherit from GUI class. Manage mouse and keyboard events.
    //--------------------------------------------------------------------------
    virtual void handleInput() override;

    //--------------------------------------------------------------------------
    //! \brief Inherit from GUI class. Return true if GUI is alive.
    //--------------------------------------------------------------------------
    virtual bool isRunning() override
    {
        return m_running;
    }

    //--------------------------------------------------------------------------
    //! \brief Called when the GUI has been enabled.
    //--------------------------------------------------------------------------
    virtual void activate() override
    {
        // Do nothing
    }

    //--------------------------------------------------------------------------
    //! \brief Called when the GUI has been disabled.
    //--------------------------------------------------------------------------
    virtual void deactivate() override
    {
        // Do nothing
    }

private:

    //--------------------------------------------------------------------------
    //! \brief Draw a Petri Place (as circle), its caption (text) and its tokens
    //! (as back dots or as a number).
    //--------------------------------------------------------------------------
    void draw(Place const& place, uint8_t alpha);

    //--------------------------------------------------------------------------
    //! \brief Draw a transition as rectangle and its caption.
    //--------------------------------------------------------------------------
    void draw(Transition const& transition, uint8_t alpha);

    //--------------------------------------------------------------------------
    //! \brief Draw a Petri arc as arrow and its duration (text).
    //--------------------------------------------------------------------------
    void draw(Arc const& arc, uint8_t alpha);

    //--------------------------------------------------------------------------
    //! \brief Draw a string centered on x, y coordiates.
    //--------------------------------------------------------------------------
    void draw(sf::Text&, std::string const& str, float const x, float const y);

    //--------------------------------------------------------------------------
    //! \brief Draw a unsigned integer centered on x, y coordiates.
    //--------------------------------------------------------------------------
    void draw(sf::Text&, size_t const number, float const x, float const y);

    //--------------------------------------------------------------------------
    //! \brief Draw a float value centered on x, y coordiates.
    //--------------------------------------------------------------------------
    void draw(sf::Text&, float const number, float const x, float const y);

    //--------------------------------------------------------------------------
    //! \brief Search and return if a place or a transition is present at the
    //! given coordinates.
    //! \param[in] x: X-axis coordinate of the mouse cursor.
    //! \param[in] y: Y-axis coordinate of the mouse cursor.
    //! \return the address of the place or the transition if present, else
    //! return nullptr.
    //--------------------------------------------------------------------------
    Node* getNode(float const x, float const y);

    //--------------------------------------------------------------------------
    //! \brief Handle the origin node of the arc when the user is clicking on
    //! the window.
    //--------------------------------------------------------------------------
    void handleArcOrigin();

    //--------------------------------------------------------------------------
    //! \brief Handle the destination node of the arc when the user is clicking
    //! on the window.
    //--------------------------------------------------------------------------
    void handleArcDestination();

    //--------------------------------------------------------------------------
    //! \brief Handle the keyboard press event.
    //--------------------------------------------------------------------------
    void handleKeyPressed(sf::Event const& event);

    //--------------------------------------------------------------------------
    //! \brief Handle the mouse click event.
    //--------------------------------------------------------------------------
    void handleMouseButton(sf::Event const& event);

private:

    //! \brief State machine for the Petri net simulation.
    enum States
    {
        STATE_IDLE, //! Waiting the user request to start the simulation.
        STATE_STARTING, //! Init states before the simulation.
        STATE_ENDING, //! Restore states after the simulation.
        STATE_ANIMATING //! Simulation on-going: animate tokens.
    };

    //! \brief The Petri net.
    PetriNet& m_petri_net;
    //! \brief Set true if the thread of the application shall stay alive.
    //! Set false to quit the application.
    std::atomic<bool> m_running{true};
    //! \brief Set true for starting the simulation the Petri net and maintain
    //! it alive. Set false to halt the simulation.
    std::atomic<bool> m_simulating{false};
    //! \brief State machine for the simulation.
    std::atomic<States> m_state{STATE_IDLE};
    //! \brief Set true when the user is pressing the Control key.
    std::atomic<bool> m_ctrl{false};
    //! \brief SFML circle shape needed to draw a Petri Place.
    sf::CircleShape m_figure_place;
    //! \brief SFML circle shape needed to draw a Petri Token.
    sf::CircleShape m_figure_token;
    //! \brief SFML rectangle shape needed to draw a Petri Transition.
    sf::RectangleShape m_figure_trans;
    //! \brief SFML loaded font from a TTF file.
    sf::Font m_font;
    //! \brief SFML structure for rendering node captions.
    sf::Text m_text_caption;
    //! \brief SFML structure for rendering the number of tokens in Places.
    sf::Text m_text_token;
    //!
    MessageBar m_message_bar;
    //! \brief Selected origin node (place or transition) by the user when
    //! adding an arc.
    Node* m_node_from = nullptr;
    //! \brief Selected destination node (place or transition) by the user when
    //! adding an arc.
    Node* m_node_to = nullptr;
    //! \brief The user has select a node to be displaced.
    std::vector<Node*> m_selected_modes;
    // Ugly stuffs needed when trying to determine which node the user wants to
    // create.
    float m_x = 0.0f; float m_y = 0.0f; bool m_arc_from_unknown_node = false;
    //! \brief Mouse cursor position.
    sf::Vector2f m_mouse;
    //! \brief Animation of tokens when transitioning from Transitions to Places.
    std::vector<AnimatedToken> m_animations;
};

#endif