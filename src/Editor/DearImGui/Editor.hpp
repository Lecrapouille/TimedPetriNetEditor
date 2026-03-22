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
#  include "PetriView.hpp"
#  include "Document.hpp"
#  include "Net/Simulation.hpp"
#  include "Net/Exports/Exports.hpp"
#  include "Net/Imports/Imports.hpp"
#  include "Utils/ForceDirected.hpp"
#  include "Utils/History.hpp"
#  include "Utils/Path.hpp"
#  include <vector>
#  include <memory>

namespace tpne { class IRemoteControl; }

namespace tpne {

// ****************************************************************************
//! \brief Graphical User interface for manipulating and simulating Petri net.
// ****************************************************************************
class Editor: public PetriNetEditor, public Application
{
    friend class PetriView;
    friend class ZeroMQRemote;

public:

    //-------------------------------------------------------------------------
    //! \brief Constructor. No additional actions are made here.
    //-------------------------------------------------------------------------
    Editor(size_t const width, size_t const height, std::string const& title);

    //-------------------------------------------------------------------------
    //! \brief Destructor. Defined in .cpp to allow unique_ptr with forward
    //! declared types to work properly.
    //-------------------------------------------------------------------------
    ~Editor();

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
    void menuFile();
    void menuView();
    void menuActions();
    void menuGraphEvents();
    void menuHelp();
    void handleMenuActions();
    void showUnsavedChangesDialog();
    void saveCurrentDocument();
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
    void initRemoteControl();
    void toggleRemoteControl();

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

protected: // Methods for PetriView access

    //! \brief Get caption display state for places.
    bool showPlaceCaptions() const { return m_states.show_place_captions; }
    //! \brief Get caption display state for transitions.
    bool showTransitionCaptions() const { return m_states.show_transition_captions; }
    //! \brief Get marked arcs (critical cycle, errors).
    std::vector<Arc*>& markedArcs() { return m_marked_arcs; }
    //! \brief Clear marked arcs.
    void clearMarkedArcs() { m_marked_arcs.clear(); }

    // Clipboard operations
    void clearClipboard() { m_clipboard.clear(); }
    bool isClipboardEmpty() const { return m_clipboard.empty(); }
    void setClipboardCenter(float x, float y) { m_clipboard.center = ImVec2(x, y); }
    void addToClipboard(Place* p);
    void addToClipboard(Transition* t);
    void addArcToClipboard(Arc const& arc);
    void pasteFromClipboard(Net& net, ImVec2 const& position,
                            std::vector<Node*>& selected_nodes);

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
    //! \brief Remote control for external debugging/control.
    std::unique_ptr<IRemoteControl> m_remote;
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
