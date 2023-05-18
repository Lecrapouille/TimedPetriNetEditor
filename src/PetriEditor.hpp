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

#ifndef PETRIEDITOR_HPP
#  define PETRIEDITOR_HPP

#  include "Renderer/Application.hpp"
#  include "Renderer/EntryBox.hpp"
#  include "utils/Grid.hpp"
#  include "utils/MessageBar.hpp"
#  include "utils/AnimatedTokens.hpp"
#  include "MQTT/MQTT.hpp"
#  include "PetriNet.hpp"
#  include <functional>

// *****************************************************************************
//! \brief Graphical representation and manipulation of the Petri net using the
//! SFML library for the rendering.
// *****************************************************************************
class PetriEditor: public Application::GUI, public MQTT
{
private:

    // *************************************************************************
    //! \brief State machine for the Petri net simulation.
    // *************************************************************************
    enum States
    {
        STATE_IDLE,     //! Waiting the user request to start the simulation.
        STATE_STARTING, //! Init states before the simulation.
        STATE_ENDING,   //! Restore states after the simulation.
        STATE_ANIMATING //! Simulation on-going: animate tokens.
    };

    // *************************************************************************
    //! \brief Structure holding information for exports Petri net to another
    //! application input format.
    // *************************************************************************
    struct Export
    {
        //--------------------------------------------------------------------------
        //! \brief Function doing the exportation of the Petri net into the given
        //! file.
        //--------------------------------------------------------------------------
        using ExportFun = std::function<bool(PetriNet const& pn, std::string const& file)>;

        //--------------------------------------------------------------------------
        //! \brief Default constructor
        //! \param[in] application the name of the application we are exporting to.
        //! \param[in] title the caption for the menu.
        //! \param[in] extensions the list of file extensions accepted by the Application.
        //! \param[in] export_function the function to call for doing the export.
        //--------------------------------------------------------------------------
        Export(std::string const& application, std::string const& title, std::initializer_list<std::string>
               const& extensions, ExportFun export_function)
            : m_application(application), m_title(title), m_extensions(extensions),
              m_export_function(export_function)
        {}

        //--------------------------------------------------------------------------
        //! \brief Return the title for Dear IM Gui menu.
        //--------------------------------------------------------------------------
        inline std::string const& title() const { return m_title; }

        //--------------------------------------------------------------------------
        //! \brief Call the exportation function to the given Petri net.
        //! \param[in] net: the Petri net to export.
        //! \param[in] application the name of the application we are exporting to.
        //! \param[out] message the error or information returned during the exportation.
        //! \return true in case of success.
        //--------------------------------------------------------------------------
        bool exports(PetriNet const& net, std::string const& application, std::string& message) const;

        //--------------------------------------------------------------------------
        //! \brief Dummy constructor do not use call it directly.
        //--------------------------------------------------------------------------
        Export() = default;

    private:

        //! \brief Name to which application we are exporting the Petri net to.
        std::string m_application;
        //! \brief Menu title
        std::string m_title;
        //! \brief List of file extensions.
        std::vector<std::string> m_extensions;
        //! \brief Function doing the exportation of the Petri net into the given
        //! file.
        ExportFun m_export_function;
    };

public:

    //--------------------------------------------------------------------------
    //! \brief Default constructor. Graphical editor with dummy net.
    //! \param[in] net Petri net (usually dummy but not necessary) to edit.
    //--------------------------------------------------------------------------
    PetriEditor(Application& application, PetriNet& net);

    //--------------------------------------------------------------------------
    //! \brief Default constructor. Graphical editor with net to load given the
    //! path of the file.
    //! \param[in] net Petri net to edit.
    //! \param[in] file the path of the Petri JSON file to load. Current net is
    //! reset before the loading. In case of loading failure (file not found or
    //! bad parsing) the net is dummy.
    //--------------------------------------------------------------------------
    PetriEditor(Application& application, PetriNet& net, std::string const& file);

    //--------------------------------------------------------------------------
    //! \brief Halt the application. Before closing the application, open the
    //! file manage before for saving the Petri net if modified.
    //--------------------------------------------------------------------------
    void close();

    bool screenshot();
    bool load();
    bool changeTypeOfNet(PetriNet::Type const mode)
    {
        if (m_simulating)
            return false;
        if (m_petri_net.type(mode, m_marked_arcs))
            return true;

        m_message_bar.setError(m_petri_net.message());
        return false;
    }
    std::string const& getError() const { return m_message_bar.getText(); }
    std::vector<MessageBar::TimedMessage> const& getLogs() const { return m_message_bar.getBuffer(); }
    void clearLogs() { m_message_bar.clear(); }

    bool findCriticalCycle()
    {
        return m_petri_net.findCriticalCycle(m_marked_arcs);
    }

    inline std::map<std::string, PetriEditor::Export>const& exporters() const
    {
        return m_exporters;
    }

    //--------------------------------------------------------------------------
    //! \brief Save the Petri net in its current JSON file. If the net was not
    //! loaded from a file, or if the \c force argument is set to true, a file
    //! manager is called to select the destination file.
    //!
    //! \param[in] force set to true when the application is closing to force
    //! saving the net: this opens a file manager to select the destination
    //! file. If the user cancel the file manager then the net is saved in a
    //! temporary file anyway.
    //!
    //! \return true if successfully saved else return false.
    //--------------------------------------------------------------------------
    bool save(bool const force = false);

    //--------------------------------------------------------------------------
    //! \brief Align nodes on a "magnetic" grid.
    //--------------------------------------------------------------------------
    void align();

    void clear();

    //--------------------------------------------------------------------------
    //! \brief Return the help as stringstream.
    //--------------------------------------------------------------------------
    static std::stringstream help();

    //--------------------------------------------------------------------------
    //! \brief Export the Petri net into application file format.
    //--------------------------------------------------------------------------
    bool exports(std::string const& format);

private: // Derived from MQTT

void onDrawIMGui();
void renderScene(sf::RenderTexture&);

    //--------------------------------------------------------------------------
    //! \brief Do MQTT subscriptions when this class (MQTT client) is conencted
    //! to the broker.
    //--------------------------------------------------------------------------
    virtual void onConnected(int rc) override;

    //--------------------------------------------------------------------------
    //! \brief React to the received message. In our case, we want to simulate
    //! mouse click on transitions to fire them or increment the number of tokens
    //! in a place.
    //--------------------------------------------------------------------------
    virtual void onMessageReceived(const struct mosquitto_message& message) override;

private: // Derived from Application::GUI

    //--------------------------------------------------------------------------
    //! \brief Inherit from GUI class. Draw the Petri net.
    //--------------------------------------------------------------------------
    virtual void onDraw() override;

    //--------------------------------------------------------------------------
    //! \brief Inherit from GUI class. Update GUI and Petri net.
    //--------------------------------------------------------------------------
    virtual void onUpdate(const float dt) override;

    //--------------------------------------------------------------------------
    //! \brief Inherit from GUI class. Manage mouse and keyboard events.
    //--------------------------------------------------------------------------
    virtual void onHandleInput(sf::Event const& event) override;

    //-------------------------------------------------------------------------
    //! \brief Inherit from GUI class. The GUI has been activated.
    //-------------------------------------------------------------------------
    virtual void onActivate() override
    {
        // Do nothing
    }

    //-------------------------------------------------------------------------
    //! \briefInherit from GUI class. The GUI has been deactivated.
    //-------------------------------------------------------------------------
    virtual void onDeactivate() override
    {
        // Do nothing
    }

    //-------------------------------------------------------------------------
    //! \brief Create the GUI event.
    //-------------------------------------------------------------------------
    virtual void onCreate() override
    {
        // Do nothing
    }

    //-------------------------------------------------------------------------
    //! \brief Release the GUI event.
    //-------------------------------------------------------------------------
    virtual void onRelease() override
    {
        // Do nothing
    }

private:

    //--------------------------------------------------------------------------
    //! \brief Load a new Petri net from JSON file. Current net is reset before
    //! the loading. In case of loading failure (file not found or bad parsing)
    //! the net is dummy.
    //! \return true if successfully loaded else return false.
    //--------------------------------------------------------------------------
    bool load(std::string const& file);

    //--------------------------------------------------------------------------
    //! \brief Draw a Petri Place (as circle), its caption (text) and its tokens
    //! (as back dots or as a number).
    //! \param[in] place the reference of the Petri place to render.
    //! \param[in] alpha transparency channel (0 .. 255) for the fading effect.
    //--------------------------------------------------------------------------
    void draw(Place const& place, uint8_t alpha);

    //--------------------------------------------------------------------------
    //! \brief Draw a transition as rectangle and its caption.
    //! \param[in] transition the reference of the Petri transition to render.
    //! \param[in] alpha transparency channel (0 .. 255) for the fading effect.
    //--------------------------------------------------------------------------
    void draw(Transition const& transition, uint8_t alpha);

    //--------------------------------------------------------------------------
    //! \brief Draw a Petri arc as arrow and its duration (text).
    //! \param[in] arc the reference of the Petri arc to render.
    //! \param[in] alpha transparency channel (0 .. 255) for the fading effect.
    //--------------------------------------------------------------------------
    void draw(Arc const& arc, uint8_t alpha);

    //--------------------------------------------------------------------------
    //! \brief Draw a string centered on x, y coordiates.
    //! \param[in] txt the SFML holding font and other information for the
    //! rendering.
    //! \param[in] str the text to render.
    //! \param[in] x: X-axis coordinate of the beginning of the text.
    //! \param[in] y: Y-axis coordinate of the center of the text height.
    //--------------------------------------------------------------------------
    void draw(sf::Text& txt, std::string const& str, float const x, float const y);

    //--------------------------------------------------------------------------
    //! \brief Draw a unsigned integer centered on x, y coordiates.
    //! \param[in] txt the SFML holding font and other information for the
    //! rendering.
    //! \param[in] number the integer value to render.
    //! \param[in] x: X-axis coordinate of the beginning of the text.
    //! \param[in] y: Y-axis coordinate of the center of the text height.
    //--------------------------------------------------------------------------
    void draw(sf::Text& txt, size_t const number, float const x, float const y);

    //--------------------------------------------------------------------------
    //! \brief Draw a float value centered on x, y coordiates.
    //! \param[in] txt the SFML holding font and other information for the
    //! rendering.
    //! \param[in] number the float value to render.
    //! \param[in] x: X-axis coordinate of the beginning of the text.
    //! \param[in] y: Y-axis coordinate of the center of the text height.
    //--------------------------------------------------------------------------
    void draw(sf::Text& txt, float const number, float const x, float const y);

    //--------------------------------------------------------------------------
    //! \brief Search and return of the first place or a transition if its shape
    //! contains the given coordinates (usually the mouse).
    //! \param[in] x: X-axis coordinate of the mouse cursor.
    //! \param[in] y: Y-axis coordinate of the mouse cursor.
    //! \return the address of the place or the transition if present, else
    //! return nullptr.
    //--------------------------------------------------------------------------
    Node* getNode(float const x, float const y);

    //--------------------------------------------------------------------------
    //! \brief Search and return of the first transition if its shape contains
    //! the given coordinates (usually the mouse).
    //! \param[in] x: X-axis coordinate of the mouse cursor.
    //! \param[in] y: Y-axis coordinate of the mouse cursor.
    //! \return the address of the transition if present, else return nullptr.
    //--------------------------------------------------------------------------
    Transition* getTransition(float const x, float const y);

    //--------------------------------------------------------------------------
    //! \brief Search and return of the first place if its shape contains
    //! the given coordinates (usually the mouse).
    //! \param[in] x: X-axis coordinate of the mouse cursor.
    //! \param[in] y: Y-axis coordinate of the mouse cursor.
    //! \return the address of the place if present, else return nullptr.
    //--------------------------------------------------------------------------
    Place* getPlace(float const x, float const y);

    //--------------------------------------------------------------------------
    //! \brief Search and return of the first arc if its shape contains
    //! the given coordinates (usually the mouse).
    //! \param[in] x: X-axis coordinate of the mouse cursor.
    //! \param[in] y: Y-axis coordinate of the mouse cursor.
    //! \return the address of the arc if present, else return nullptr.
    //--------------------------------------------------------------------------
    Arc* getArc(float const x, float const y);

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

    //--------------------------------------------------------------------------
    //! \brief Check if the user has clicked on the caption of a node.
    //! Side effect: the m_entry_box holds the caption.
    //--------------------------------------------------------------------------
    bool clickedOnCaption();

    //--------------------------------------------------------------------------
    //! \brief Zoom/unzoom.
    //--------------------------------------------------------------------------
    void applyZoom(float const delta);

private:

sf::RenderTexture m_render_texture;

    //! \brief The Petri net.
public:    PetriNet& m_petri_net;

//private:
    //! \brief List of format the editor can export.
    std::map<std::string, PetriEditor::Export> m_exporters;
    //! \brief Memorize initial number of tokens in places.
    std::vector<size_t> m_marks;
    //! \brief Critical cycle found by Howard algorithm. Also used to show
    //! where are erroneous arcs making the Petri net not be a graph event.
    std::vector<Arc*> m_marked_arcs;
    //! \brief Color of m_marked_arcs.
    sf::Color m_marked_arcs_color;
    //! \brief Path of the Petri net file: not empty when the net was loaded
    //! from file, else empty when created from scratch.
    std::string m_petri_filename;
    //! \brief Set true for starting the simulation the Petri net and to
    //! maintain the simulation running. Set false to halt the simulation.
    std::atomic<bool> m_simulating{false};
    //! \brief State machine for the simulation.
    std::atomic<States> m_state{STATE_IDLE};
    //! \brief SFML loaded font from a TTF file.
    sf::Font m_font;
    //! \brief Set true when the user is pressing the Control key.
    std::atomic<bool> m_ctrl{false};
    //! \brief Set true when the user is pressing the Shift key.
    std::atomic<bool> m_shift{false};
    //! \brief Cache the SFML circle shape needed to draw a Petri Place.
    sf::CircleShape m_shape_place;
    //! \brief Cache the SFML circle shape needed to draw a Petri Token.
    sf::CircleShape m_shape_token;
    //! \brief Cache the SFML rectangle shape needed to draw a Petri Transition.
    sf::RectangleShape m_shape_transition;
    //! \brief Cache the SFML structure for rendering node captions.
    sf::Text m_text_caption;
    //! \brief Cache the SFML structure for rendering the number of tokens in
    //! Places.
    sf::Text m_text_token;
    //! \brief Custom widget rendering a message with fading out effect.
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
    std::vector<AnimatedToken> m_animated_tokens;
    //! \brief Widget editing text such as Node caption and values.
    EntryBox m_entry_box;
    //! \brief Show the grid
    Grid m_grid;
    sf::View m_view;
    //! \brief Camera zoom
    float m_zoom_level = 1.0f;
    //! \brief Subscription to MQTT topic for receiving commands for manipulating
    //! the net
    std::string m_mqtt_topic;

    bool m_is_hovered = false;
};

#endif
