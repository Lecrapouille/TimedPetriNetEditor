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

#ifndef PETRIEDITOR_HPP
#  define PETRIEDITOR_HPP

#  include "Application.hpp" // Path defined in the Makefile
#  include "TimedPetriNetEditor/PetriNet.hpp"
#  include "Net/Simulation.hpp"
#  include "Editor/Messages.hpp"

namespace tpne {

struct Exporter;

// *****************************************************************************
//! \brief Graphical representation and manipulation of the Petri net using the
//! SFML library for the rendering.
// *****************************************************************************
class Editor: public Application
{
public:

    //--------------------------------------------------------------------------
    //! \brief Constructor.
    //! \param[in] project_path the path of the Petri net fil to load. Let it
    //! dummy if you do not want to open a file.
    //--------------------------------------------------------------------------
    Editor(size_t const width, size_t const height, std::string const& title,
           std::string const& project_path = {});

private: // Inheritnace from Application class

    virtual void onStartUp() override;
    virtual void onUpdate(float const dt) override;
    virtual void onDraw() override;

private: // Show results from Petri algorithms

    void showCriticalCycles() const;
    void showDynamicLinearSystem() const;
    void showCounterOrDaterequation() const;
    void showAdjacencyMatrices() const;

private: // Widgets

    void menu();
    void about() const;
    void help() const;
    void console();
    void messagebox();
    void inspector();

private:

    Node* getNode(float const x, float const y);
    Place* getPlace(float const x, float const y);
    Transition* getTransition(float const x, float const y);
    bool switchOfNet(TypeOfNet const type);
    void loadNetFile();
    void exportNetTo(Exporter const& exporter);
    void saveNetAs();
    void closeNet();
    void toogleStartSimulation();
    void takeScreenshot();
    void clearNet();

private: // Error logs

    std::string getError() const;  // FIXME: by copy
    std::vector<Messages::TimedMessage> const& getLogs() const;
    void clearLogs();

private:

    struct GridLayout
    {

    };

    class View
    {
    public:

        void reshape();
        void drawGrill();
        void drawArc(Arc const& arc);
        void drawPlace(Place const& place);
        void drawTransition(Transition const& transition);
        void drawPetriNet();
        void drawToken(float const x, float const y);

    private:
        Layout m_layout;
        ImVec2 scrolling{0.0f, 0.0f};
        ImVec2 canvas_p0;
        ImVec2 canvas_p1;
        ImVec2 canvas_sz;
        ImVec2 origin;
    };

    struct MouseSelection
    {
        //! \brief Mouse cursor position.
        ImVec2 m_mouse;
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
        ImVec2 m_click_position; bool m_arc_from_unknown_node = false;
    };

    struct States
    {
        bool is_dragging = false;
        bool disable_dragging = false;
        bool do_dater = false;
        bool do_counter = false;
        bool do_find_critical_cycle = false;
        bool do_syslin = false;
        bool do_adjency = false;
        bool do_load = false;
        bool do_save_as = false;
        bool do_screenshot = false;
        Exporter const* do_export_to = nullptr;
        bool show_about = false;
        bool show_help = false;
        bool show_place_captions = false;
        bool show_transition_captions = false;
        ImVec2 viewport_center;
    };

    //! \brief The single Petri net the editor can edit
    Net m_net; // TODO faire plusieurs pour le GRAFCET
    //! \brief
    Simulation m_simulation;
    //! \brief
    std::vector<Exporter> m_exporters;
    //! \brief Path of the Petri net file: not empty when the net was loaded
    //! from file, else empty when created from scratch.
    std::string m_filename;
    //! \brief
    Messages m_messages;
    //! \brief
    mutable States m_states;
    //! \brief
    ImDrawList* draw_list;






    // ************************************************************************
    //! \brief Look and fell of the editor.
    // ************************************************************************
    struct LayoutConfig
    {
        struct Grid
        {
            float step = 64.0f;
            bool  enable = true;
            bool  menu = true;
        };

        struct Color
        {
            // selected
        };

        Grid grid;
    };

public: //private:



    void onHandleInput();
    void onDragged(ImVec2 const& mouse_delta); // Grid
    ImVec2 getMousePosition();
    void handleArcOrigin();
    void handleArcDestination();
    void handleAddNode(ImGuiMouseButton button);
    void handleMoveNode();
    bool IsMouseClicked(ImGuiMouseButton& button, bool& dragging);
    bool IsMouseReleased(ImGuiMouseButton& key);









    int node_hovered_in_list = -1;
    int node_hovered_in_scene = -1;

private: // gestion de la souris


private:

    //! \brief Some algorithms indicate arcs (i.e. critical cycles, or
    //! if a Petri net is an event graph).
    std::vector<Arc*> m_selected_arcs;



public: // FIXME


};

// FIXME export mandatory because of friendship
//void menu(Editor& editor);

} // namespace tpne

#endif
