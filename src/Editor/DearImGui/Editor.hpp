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

#ifndef DEAR_IMGUI_PETRI_NET_EDITOR_HPP
#  define DEAR_IMGUI_PETRI_NET_EDITOR_HPP

#  include "Application.hpp" // Selected by Makefile
#  include "TimedPetriNetEditor/PetriEditor.hpp"
#  include "Document.hpp"
#  include "Net/Simulation.hpp"
#  include "Net/Exports/Exports.hpp"
#  include "Net/Imports/Imports.hpp"
#  include "Utils/ForceDirected.hpp"
#  include "Utils/History.hpp"
#  include "Utils/Path.hpp"
#  ifdef WITH_MQTT
#    include "MQTT/MQTT.hpp"
#  endif
#  include <vector>
#  include <memory>

namespace tpne {

// ****************************************************************************
//! \brief Graphical User interface for manipulating and simulating Petri net.
// ****************************************************************************
class Editor: public PetriNetEditor, public Application
{
public:

    //-------------------------------------------------------------------------
    //! \brief Constructor. No additional actions are made here.
    //-------------------------------------------------------------------------
    Editor(size_t const width, size_t const height, std::string const& title);

    //-------------------------------------------------------------------------
    //! \brief Starts the Petri net editor up, load the Petri file if given not
    //! empty. Then call the infinite loop of the GUI.
    //! \param[in] petri_file the path of the Petri net fil to load. Pass dummy
    //! string if you do not want to load a Petri net file.
    //-------------------------------------------------------------------------
    virtual void run(std::string const& petri_file) override;
    virtual void run(Net const& net) override;

    // Document management
    Document& activeDocument();
    Document const& activeDocument() const;
    Net& net();
    Net const& net() const;
    Simulation& simulation();
    Simulation const& simulation() const;

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
    inline ImVec2 viewSize() const { return m_view.size(); }

private: // Show results from Petri algorithms

    void showCriticalCycles();// const;
    void showDynamicLinearSystem() const;
    void showCounterOrDaterEquation() const;
    void showAdjacencyMatrices() const;

private: // Document management

    void newDocument();
    Document& createDocument();
    void closeDocument(Document* doc);
    void setActiveDocument(size_t index);
    size_t documentCount() const { return m_documents.size(); }
    void addNetToActiveDocument(TypeOfNet type, std::string const& name);
    void createGEMMADocument();

private: // Petri net services

    Node* getNode(ImVec2 const& position);
    void exportNetTo(Exporter const& exporter);
    void importNetFrom(Importer const& importer);
    void loadNetFile();
    void loadDocumentFromFile(std::string const& filepath);
    void saveNetAs();
    void saveDocumentAs();
    void closeNet();
    void toogleStartSimulation();
    void toogleStartAllSimulations();
    void updateSimulationFramerate();
    void takeScreenshot();
    void clearNet();
    void undo();
    void redo();
    void springify();
    bool initMQTT();

private:

    Place* getPlace(ImVec2 const& position);
    Transition* getTransition(ImVec2 const& position);
    bool switchOfNet(TypeOfNet const type);
    Transition& addTransition(float const x, float const y);
    void addPlace(float const x, float const y);
    void removeNode(Node& node);
    void removeArc(Arc& arc);
    Node& addOppositeNode(Node::Type const type, float const x, float const y,
        size_t const tokens = 0u);
    void addArc(Node& from, Node& to, float const duration = 0.0f);

private: // Error logs

    void setSavePath(std::string const& filepath);
    std::string getError() const;  // FIXME: by copy
    std::vector<Messages::TimedMessage> const& getLogs() const;
    void clearLogs();
    void showStyleSelector();

private:

    // ************************************************************************
    //! \brief Hold X and Y data for plotting.
    // ************************************************************************
    class PlotData
    {
    public:

        inline void add(float const x, float const y) { m_x_data.push_back(x); m_y_data.push_back(y); }
        inline void reset() { m_x_data.clear(); m_y_data.clear(); }
        inline bool isEmpty() const { return m_x_data.empty(); }
        inline bool length() const { return m_x_data.size(); }
        inline std::vector<float> const& x() const { return m_x_data; }
        inline std::vector<float> const& y() const { return m_y_data; }

    private:
        std::vector<float> m_x_data;
        std::vector<float> m_y_data;
    };

    // ************************************************************************
    //! \brief Since we are using immediate mode GUI we need to memorize states
    //! controling which widgets/modal windows to show.
    // ************************************************************************
    struct States
    {
        bool do_counter_or_dater = false;
        bool do_find_critical_cycle = false;
        bool do_syslin = false;
        bool do_adjency = false;
        bool do_load = false;
        bool do_save_as = false;
        bool do_screenshot = false;
        Exporter const* do_export_to = nullptr;
        Importer const* do_import_from = nullptr;
        bool show_about = false;
        bool show_help = false;
        bool show_theme = false;
        bool show_place_captions = true;
        bool show_transition_captions = true;
        ImVec2 viewport_center;
        std::string title;
        bool request_quitting = false;
        bool request_new = false;
        bool request_vertical_split = false;
        bool show_unsaved_dialog = false;
        PlotData plot;
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
            float step = 50.0f;
            bool  show = true;
            bool  menu = true;
            bool  snap = true;  // Actif par defaut

            float snapValue(float v) const
            {
                return snap ? roundf(v / step) * step : v;
            }
        } grid;

        PetriView(Editor& editor);
        ImVec2 reshape();
        void onHandleInput(Net& net, Simulation& simulation);
        void drawPetriNet(Net& net, Simulation& simulation,
                          bool interactive = true);
        inline ImVec2 const& origin() const { return m_canvas.origin; };
        inline ImVec2 const& size() const { return m_canvas.size; };
        void loadViewState(Document::ViewState const& state);
        void saveViewState(Document::ViewState& state) const;

    private:

        bool isMouseClicked(ImGuiMouseButton& key);
        bool isMouseReleased(ImGuiMouseButton& key);
        bool isMouseDraggingView(ImGuiMouseButton const& key);
        void handleAddNode(ImGuiMouseButton button);
        void handleArcOrigin();
        void handleMoveNode();
        void handleArcDestination();
        void handleSelection(ImGuiMouseButton button);
        void drawGrid(ImDrawList* draw_list, bool const running);
        void drawRubberBand(ImDrawList* draw_list);
        bool isNodeSelected(Node* node) const;
        void selectNode(Node* node, bool add_to_selection);
        void deselectNode(Node* node);
        void clearSelection();
        void selectNodesInRect(ImVec2 const& min, ImVec2 const& max);
        Arc* getArc(ImVec2 const& position);
        bool isArcSelected(Arc* arc) const;
        void selectArc(Arc* arc, bool add_to_selection);
        void deselectArc(Arc* arc);
        void clearArcSelection();
        void copySelection();
        void pasteClipboard();

    private:

        Editor& m_editor;
        //! \brief Current net being drawn/edited (set by drawPetriNet)
        Net* m_current_net = nullptr;
        //! \brief Current simulation being used (set by drawPetriNet)
        Simulation* m_current_simulation = nullptr;

        // ********************************************************************
        //! \brief
        // ********************************************************************
        struct Canvas
        {
            ImVec2 corners[2] = {{0.0f, 0.0f}, {0.0f, 0.0f}};
            ImVec2 size{800.0f, 600.0f}; // FIXME: size is undefined while not
                                         // rendered once but when importing nets
                                         // we need to know th windows size which
                                         // is 0x0
            ImVec2 origin{0.0f, 0.0f};
            ImVec2 scrolling{0.0f, 0.0f};
            float zoom = 1.0f;
            static constexpr float ZOOM_MIN = 0.1f;
            static constexpr float ZOOM_MAX = 5.0f;
            ImDrawList* draw_list;

            ImVec2 getMousePosition();
            ImVec2 reshape();
            void push();
            void pop();
        } m_canvas;

        // ********************************************************************
        //! \brief
        // ********************************************************************
        class MouseState
        {
        public:
            //! \brief Memorize the mouse cursor position when the user has moved it.
            ImVec2 position;
            //! \brief Memorize the mouse cursor position when the user has clicked.
            ImVec2 clicked_at;
            //! \brief The user is dragging the view
            bool is_dragging_view = false;
            //! \brief Selected origin node (place or transition) by the user when
            //! adding an arc.
            Node* from = nullptr;
            //! \brief The user is creating an arc ?
            bool handling_arc = false;
            //! \brief Selected destination node (place or transition) by the user when
            //! adding an arc.
            Node* to = nullptr;
            //! \brief The user has select a node to be displaced (legacy mode with `;` key).
            std::vector<Node*> selection;
            //! \brief Persistent selection of nodes (for copy, delete, drag).
            std::vector<Node*> selected_nodes;
            //! \brief Persistent selection of arcs (for delete).
            std::vector<Arc*> selected_arcs;
            //! \brief True when user is dragging to create a rubber band selection.
            bool is_rubber_band = false;
            //! \brief Start position of rubber band selection.
            ImVec2 rubber_band_start;
            //! \brief True when user is dragging selected nodes.
            bool is_dragging_nodes = false;
            //! \brief Offset from mouse position to each selected node (for drag).
            std::vector<ImVec2> drag_offsets;
            //! \brief Node currently being edited (name edit mode).
            Node* editing_node = nullptr;
            //! \brief Buffer for editing node name.
            char edit_buffer[256] = {0};
            //! \brief Flag to set focus on first frame.
            bool edit_focus_requested = false;
            //! \brief Node under mouse cursor (for hover effect).
            Node* hovered_node = nullptr;
            //! \brief Arc under mouse cursor (for hover effect).
            Arc* hovered_arc = nullptr;
            //! \brief Node for context menu.
            Node* context_menu_node = nullptr;
            //! \brief Arc for context menu.
            Arc* context_menu_arc = nullptr;
        } m_mouse;
    }; // class PetriView

    // ************************************************************************
    //! \brief Quick and dirty net memorization for performing undo/redo.
    //! \fixme this is memory usage consumption by saving two nets. It's better
    //! to memorize only command. but the remove command makes nodes ID change
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
            m_editor.net() = m_before;
            return true;
        }

        virtual bool redo() override
        {
            m_editor.net() = m_after;
            return true;
        }

    private:

        Editor& m_editor;
        Net m_before;
        Net m_after;
    };

    // ************************************************************************
    //! \brief Clipboard for copy-paste of subgraphs
    // ************************************************************************
    struct Clipboard
    {
        struct PlaceData { size_t id; std::string caption; float x; float y; size_t tokens; };
        struct TransitionData { size_t id; std::string caption; float x; float y; int angle; };
        struct ArcData { size_t from_id; bool from_is_place; size_t to_id; bool to_is_place; float duration; };

        std::vector<PlaceData> places;
        std::vector<TransitionData> transitions;
        std::vector<ArcData> arcs;
        ImVec2 center{0.0f, 0.0f};

        bool empty() const { return places.empty() && transitions.empty(); }
        void clear() { places.clear(); transitions.clear(); arcs.clear(); }
    };

private:

    //! \brief Helper instance to find files like Linux $PATH environment variable.
    //! Used for example for loading font files.
    Path m_path;
    //! \brief All open documents (each can contain multiple nets)
    std::vector<std::unique_ptr<Document>> m_documents;
    //! \brief Index of the currently active document
    size_t m_active_document_index = 0;
    //! Apply spring positive/negative forces on arcs/nodes to unfold imported
    //! Petri nets that do not have positions on their nodes.
    ForceDirected m_spring;
    //! \brief History of modifications of the net.
    History m_history;
#ifdef WITH_MQTT
    //! \brief Allow to control the net from network.
    mqtt::Client m_mqtt;
    mqtt::Topic TOPIC_LOAD{"tpne/load"};
    mqtt::Topic TOPIC_START{"tpne/start"};
    mqtt::Topic TOPIC_STOP{"tpne/stop"};
    mqtt::Topic TOPIC_FIRE{"tpne/fire"};
#endif
    //! \brief Critical cycle found by Howard algorithm. Also used to show
    //! where are erroneous arcs making the Petri net not be a graph event.
    std::vector<Arc*> m_marked_arcs;
    //! \brief Clipboard for copy-paste
    Clipboard m_clipboard;
    //! \brief Visualize the net and do the interaction with the user.
    //! \note Now each Document::NetEntry will have its own PetriView-like state
    PetriView m_view;
    //! \brief Messages to be displayed on the GUI.
    Messages m_messages;
    //! \brief States controlling the GUI
    mutable States m_states;
    //! \brief Cache the path to save the loaded Petri file.
    std::string m_path_to_save;
};

} // namespace tpne

#endif
