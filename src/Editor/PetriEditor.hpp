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

#  include "Application.hpp" // Selected by Makefile
#  include "Net/Simulation.hpp"
#  include "Net/Exports/Exports.hpp"
#  include "Net/Imports/Imports.hpp"
#  include "Utils/History.hpp"
#  include "Utils/Path.hpp"

namespace tpne {

// ****************************************************************************
//! \brief Graphical User interface for manipulating and simulating Petri net.
// ****************************************************************************
class Editor: public Application
{
public:

    //-------------------------------------------------------------------------
    //! \brief Constructor. No additional actions are made here.
    //-------------------------------------------------------------------------
    Editor(size_t const width, size_t const height, std::string const& title);

    //-------------------------------------------------------------------------
    //! \brief starts up the Petri net editor and call the infinite loop.
    //! \param[in] petri_file the path of the Petri net fil to load. Let it
    //! dummy if you do not want to open a file.
    //-------------------------------------------------------------------------
    void startUp(std::string const& petri_file);

private: // Inheritance from Application class

    virtual void onUpdate(float const dt) override;
    virtual void onDraw() override;
    void close();

private: // Widgets

    void menu();
    void about() const;
    void help() const;
    void console();
    void messagebox();
    void inspector();
    void view();

private: // Show results from Petri algorithms

    void showCriticalCycles() const;
    void showDynamicLinearSystem() const;
    void showCounterOrDaterequation() const;
    void showAdjacencyMatrices() const;

private: // Petri net services

    Node* getNode(ImVec2 const& position);
    Place* getPlace(ImVec2 const& position);
    Transition* getTransition(ImVec2 const& position);
    bool switchOfNet(TypeOfNet const type);
    void exportNetTo(Exporter const& exporter);
    void importNetTo(Importer const& importer);
    void loadNetFile();
    void saveNetAs();
    void closeNet();
    void toogleStartSimulation();
    void takeScreenshot();
    void clearNet();
    void undo();
    void redo();

private: // Error logs

    std::string getError() const;  // FIXME: by copy
    std::vector<Messages::TimedMessage> const& getLogs() const;
    void clearLogs();

private:

    // ************************************************************************
    //! \brief Since we are using immediate mode GUI we need to memorize states
    //! controling which widgets/modal windows to show.
    // ************************************************************************
    struct States
    {
        bool do_dater = false;
        bool do_counter = false;
        bool do_find_critical_cycle = false;
        bool do_syslin = false;
        bool do_adjency = false;
        bool do_load = false;
        bool do_save_as = false;
        bool do_screenshot = false;
        Exporter const* do_export_to = nullptr;
        Importer const* do_import_to = nullptr;
        bool show_about = false;
        bool show_help = false;
        bool show_place_captions = true;
        bool show_transition_captions = true;
        ImVec2 viewport_center;
        std::string title;
        bool request_quitting = false;
    };

    // ************************************************************************
    //! \brief Graphical representation of the Petri net using and its
    //! interaction with the user.
    // ************************************************************************
    class PetriView
    {
    public:

        // ********************************************************************
        //! \brief
        // ********************************************************************
        struct GridLayout
        {
            float step = 64.0f;
            bool  show = true;
            bool  menu = true;
        } grid;

        PetriView(Editor& editor);
        void reshape();
        void onHandleInput();
        void drawPetriNet(Net& net, Simulation& simulation);
        inline ImVec2 const& origin() const { return m_canvas.origin; };

    private:

        bool isMouseClicked(ImGuiMouseButton& key, bool& dragging);
        bool isMouseReleased(ImGuiMouseButton& key);
        void handleAddNode(ImGuiMouseButton button);
        void handleArcOrigin();
        void handleMoveNode();
        void handleArcDestination();
        void drawGrid(ImDrawList* draw_list, bool const running);

    private:

        Editor& m_editor;

        // ********************************************************************
        //! \brief
        // ********************************************************************
        struct Canvas
        {
            ImVec2 corners[2];
            ImVec2 size;
            ImVec2 origin;
            ImVec2 scrolling{0.0f, 0.0f};
            ImDrawList* draw_list;

            ImVec2 getMousePosition();
            void reshape();
            void push();
            void pop();
        } m_canvas;

        // ********************************************************************
        //! \brief
        // ********************************************************************
        class MouseSelection
        {
        public:
            //! \brief Mouse cursor position.
            ImVec2 position;
            //! \brief
            bool is_dragging = false;
            bool disable_dragging = false; // FIXME
            //! \brief Selected origin node (place or transition) by the user when
            //! adding an arc.
            Node* from = nullptr;
            //! \brief Selected destination node (place or transition) by the user when
            //! adding an arc.
            Node* to = nullptr;
            //! \brief The user has select a node to be displaced.
            std::vector<Node*> selection;
            // Ugly stuffs needed when trying to determine which node the user wants to
            // create.
            ImVec2 click_position; bool arc_from_unknown_node = false;
        } m_mouse;
    }; // class PetriView

    // ************************************************************************
    //! \brief Quick and dirty net memorization for performing undo/redo.
    //! \fixme this is memory usage consuption by saving two nets. It's better
    //! to memorize only command. but the remove command make change nodes ID
    //! so history will become false.
    // ************************************************************************
    class NetModifaction : public History::Action
    {
    public:
        NetModifaction(Editor& editor) : m_editor(editor) {}
        void before(Net& net) { m_before = net; }
        void after(Net& net) { m_after = net; }

        virtual bool undo() override
        {
            m_editor.m_net = m_before;
            return true;
        }

        virtual bool redo() override
        {
            m_editor.m_net = m_after;
            return true;
        }

    private:

        Editor& m_editor;
        Net m_before;
        Net m_after;
    };

private:

    //! \brief Heper instance to find files like Linux $PATH environment variable.
    //! Used for example for loading font files.
    Path m_path;
    //! \brief Single Petri net the editor can edit.
    //! \fixme Manage several nets (like done with GEMMA).
    Net m_net;
    //! \brief History of modifications of the net.
    History m_history;
    //! \brief Instance allowing to do timed simulation.
    Simulation m_simulation;
    //! \brief Visualize the net and do the interaction with the user.
    PetriView m_view;
    //! \brief Messages to be displayed on the GUI.
    Messages m_messages;
    //! \brief States controling the GUI
    mutable States m_states;
    //! \brief Path of the loaded Petri file.
    std::string m_filepath;
};

} // namespace tpne

#endif
