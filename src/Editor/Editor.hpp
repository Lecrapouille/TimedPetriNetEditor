// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef DEAR_IMGUI_PETRI_NET_EDITOR_HPP
#  define DEAR_IMGUI_PETRI_NET_EDITOR_HPP

#  include "Application.hpp" // Selected by Makefile

#  include "Editor/PetriView.hpp"
#  include "Editor/Document.hpp"
#  include "Editor/Clipboard.hpp"
#  include "Editor/EditorState.hpp"
#  include "Editor/Path.hpp"

#  include "IO/IO.hpp"
#  include "Editor/ForceDirected.hpp"
#  include "Editor/JournalView.hpp"
#  include "Editor/NodeMarks.hpp"
#  include "MaxPlus/Algorithms.hpp"

#  include <deque>
#  include <vector>
#  include <filesystem>
#  include <map>
#  include <memory>

namespace tpne { class IRemoteControl; }

namespace tpne {

// ****************************************************************************
//! \brief Graphical User interface for manipulating and simulating Petri net.
// ****************************************************************************
class Editor: public Application
{
    friend class PetriView;
#  if TPNE_USE_ZEROMQ
    friend class ZeroMQRemote;
#  endif

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
    void run(std::string const& petri_file);
    void run(Net const& net, bool show_critical_cycle = false);

    //-------------------------------------------------------------------------
    //! \brief Save a screenshot after the next rendered frame.
    //! \param[in] exit_after if true, close the editor right after the capture.
    //! \param[in] whole_window the whole window, else the canvas of the net.
    //-------------------------------------------------------------------------
    void setAutoScreenshot(std::string const& path, bool exit_after = false,
                           bool whole_window = false);

    //-------------------------------------------------------------------------
    //! \brief Take the screenshot of setAutoScreenshot() after these seconds
    //! (a simulation started at launch has run).
    //-------------------------------------------------------------------------
    void screenshotDelay(double const seconds) { m_screenshot_delay = seconds; }

    //-------------------------------------------------------------------------
    //! \brief Bring this window in front once the layout is built (the tab of
    //! a dock: "Journal", "Analysis" ...).
    //-------------------------------------------------------------------------
    void focusWindowAtLaunch(std::string const& name) { m_focus_window = name; }

    //-------------------------------------------------------------------------
    //! \brief Start the simulation of the document once it is loaded.
    //-------------------------------------------------------------------------
    void startSimulationAtLaunch() { m_start_simulation = true; }

    //-------------------------------------------------------------------------
    //! \brief Whether the last automatic screenshot succeeded.
    //-------------------------------------------------------------------------
    bool lastScreenshotSucceeded() const { return m_last_screenshot_ok; }

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
    void onFrameEnd() override;
    bool isAnimating() const override;
    void close();

private: // Widgets

    void menu();
    void menuFile();
    void menuEdit();
    void menuView();
    void menuSimulation();
    void menuGraphEvents();
    void menuHelp();
    void toolbar();
    void shortcuts();
    void statusBar();
    void setupDefaultLayout(unsigned int dockspace_id);
    //! \brief Folder of the settings of the user (layout, theme), created.
    static std::filesystem::path settingsFolder();
    void handleMenuActions();
    void showUnsavedChangesDialog();
    void saveCurrentDocument();
    void about() const;
    void help() const;
    void openHelp(int tab) { m_states.show_help = true; m_states.help_tab = tab; }
    void console();
    void notifications();
    void inspector();
    void view();
    void keepNetProportions();
    inline ImVec2 viewSize() const { return m_view.size(); }

private: // GRAFCET control panel and description of the document (EditorPanel.cpp)

    void controlPanel();
    void outputsPanel();
    //! \brief Timing diagram of the steps, inputs and outputs (EditorTiming.cpp).
    void timingDiagram();
    void documentPanel();
    void inputWidget(GrafcetInput& input, bool running);
    void selectorWidget(std::vector<GrafcetInput*> const& positions, bool running);
    void setPanelInput(std::string const& name, int value, bool running);
    //! \brief Inputs and outputs read and written by the GRAFCET nets of the
    //! active document, described (added to the document info if missing).
    void collectGrafcetIo(std::vector<GrafcetInput*>& inputs, std::vector<GrafcetOutput*>& outputs);

private: // Show results from Petri algorithms

    void analysisWindow();
    void analysisCritical();
    void analysisSyslin();
    void analysisEquations();
    void analysisAdjacency();
    void analysisGantt();
    void analysisPlot();
    void openAnalysis(int tab) { m_states.show_analysis = true; m_states.analysis_tab = tab; }

private: // Document management

    void newDocument();
    Document& createDocument();
    void closeDocument(Document const* doc);
    void setActiveDocument(size_t index);
    size_t documentCount() const { return m_documents.size(); }
    void addNetToActiveDocument(TypeOfNet type, std::string const& name);
    void createGEMMADocument();

private: // Petri net services

    Node* getNode(ImVec2 const& position);
    void exportNetTo(Exporter const& exporter);
    void importNetFrom(Importer const& importer);
    //! \brief Stop the simulation before a file operation. Return true if it
    //! was running: the operation is then done at the next frame.
    bool stopSimulationFor(char const* what);
    void loadNetFile();
    void loadDocumentFromFile(std::string const& filepath);
    void saveNetAs();
    void saveDocumentAs();
    //! \brief Save all the nets of the active document and its description
    //! in a JSON file. Return the error, or empty.
    std::string writeDocument(std::string const& path);
    void closeNet();
    void toogleStartSimulation();
    void toogleStartAllSimulations();
    void updateSimulationFramerate();
    //! \brief Compute the cycle time λ of the net before its simulation.
    void computeCycleTime();
    void takeScreenshot();
    bool saveScreenshot(std::string const& path);
    void clearNet();
    bool hasUnsavedChanges() const;
    void discardUnsavedChanges();
    void fulfillPendingDocumentRequest();
    void undo();
    void redo();
    void springify();
    void initRemoteControl();
    void toggleRemoteControl();

private:

    Place* getPlace(ImVec2 const& position);
    Transition* getTransition(ImVec2 const& position);
    //! \brief Convert the active net to the type; compact: to a timed event
    //! graph drawn with its transitions only.
    bool switchOfNet(TypeOfNet const type, bool const compact = false);
    Transition& addTransition(float const x, float const y);
    void addPlace(float const x, float const y);
    void removeNode(Node& node);
    void removeArc(Arc const& arc);
    //! \brief Change the kind and the weight of an arc (undoable).
    void setArc(Arc& arc, ArcKind kind, size_t weight);
    //! \brief Change the duration, the weight of an arc and, for a timed
    //! event graph, the tokens of its place (undoable).
    void setArc(Arc& arc, float duration, size_t weight, size_t tokens);
    //! \brief Change the tokens and the capacity of a place (undoable).
    void setPlace(Place& place, size_t tokens, size_t capacity);
    //! \brief Change the color of the tokens of an arc, by its name: added to
    //! the colors of the net if new; empty: any color (undoable).
    void setArcColor(Arc& arc, std::string const& color);
    Node& addOppositeNode(Node::Type const type, float const x, float const y,
        size_t const tokens = 0u);
    void addArc(Node& from, Node& to, float const duration = 0.0f);

private: // Error logs

    void setSavePath(std::string const& filepath);
    std::vector<Messages::TimedMessage> const& getLogs() const;
    void clearLogs();
    void showStyleSelector();

private: // Theme management

    void loadTheme();
    void saveTheme();
    void applyTheme();

protected: // Methods for PetriView access

    //! \brief Draw the identifiers of places (P0, step numbers).
    bool showPlaceIdentifiers() const { return m_states.show_place_identifiers; }
    //! \brief Draw the identifiers of transitions (T0, "(0)" for GRAFCET).
    bool showTransitionIdentifiers() const { return m_states.show_transition_identifiers; }
    //! \brief GRAFCET: draw the boxes of the actions, else a small tag.
    bool showActionBoxes() const { return m_states.show_action_boxes; }
    //! \brief Mark arcs of the net (critical cycle, errors), by index in
    //! Net::arcs().
    void markArcs(Net const& net, std::vector<size_t> const& arcs)
    {
        m_marked_net = &net;
        m_marked_arcs_count = net.arcs().size();
        m_marked_arcs = arcs;
    }
    //! \brief Marked arcs of the net: none when they were marked on another
    //! net or when arcs have been added or removed since.
    std::vector<size_t> const& markedArcs(Net const& net) const
    {
        static std::vector<size_t> const none;
        return ((&net == m_marked_net) && (net.arcs().size() == m_marked_arcs_count))
            ? m_marked_arcs : none;
    }
    //! \brief Clear marked arcs.
    void clearMarkedArcs() { m_marked_arcs.clear(); }

    // Clipboard operations (delegate to m_clipboard)
    void clearClipboard() { m_clipboard.clear(); }
    bool isClipboardEmpty() const { return m_clipboard.empty(); }
    void setClipboardCenter(float x, float y) { m_clipboard.setCenter(x, y); }
    void addToClipboard(Place const& p) { m_clipboard.addPlace(p, net().position(p)); }
    void addToClipboard(Transition const& t) { m_clipboard.addTransition(t, net().position(t)); }
    void addArcToClipboard(Arc const& arc) { m_clipboard.addArc(arc); }
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
    //! \brief Text computed from a net (matrices, equations), kept until the
    //! net changes (Net::modified.revision()) or the display options change.
    //! Avoids recomputing analysis results at every frame of a dialog.
    // ************************************************************************
    struct TextCache
    {
        template<class Compute>
        std::string const& get(Net const& net_, unsigned const options_,
                               Compute&& compute)
        {
            if ((net != &net_) || (revision != net_.modified.revision()) ||
                (options != options_))
            {
                text = compute();
                net = &net_;
                revision = net_.modified.revision();
                options = options_;
            }
            return text;
        }

        void clear() { net = nullptr; }

        Net const* net = nullptr;
        size_t revision = 0u;
        unsigned options = 0u;
        std::string text;
    };

public:

    //! \brief One bar of the Gantt chart: an operation during one cycle.
    struct GanttBarData { int row; int color; double start; double end; std::string label; };

private:

    // ************************************************************************
    //! \brief Since we are using immediate mode GUI we need to memorize states
    //! controling which widgets/modal windows to show.
    // ************************************************************************
    struct States
    {
        //! \brief Enum for mutually exclusive file dialogs
        enum class FileDialog { None, Load, SaveAs, Export, Import, Screenshot };

        //! \brief Analysis window and the tab to bring in front (-1: none).
        bool show_analysis = false;
        int analysis_tab = -1;
        int gantt_cycles = 3;

        //! \brief File operation dialog state (mutually exclusive)
        FileDialog file_dialog = FileDialog::None;
        Exporter const* pending_exporter = nullptr;
        Importer const* pending_importer = nullptr;

        //! \brief Modal info dialogs
        bool show_about = false;
        bool show_help = false;
        //! \brief Tab of the help to bring in front (-1: none).
        int help_tab = -1;
        bool show_theme = false;
        bool show_unsaved_dialog = false;

        //! \brief Display options
        bool show_place_identifiers = true;
        bool show_transition_identifiers = true;
        //! \brief GRAFCET: the boxes of the actions (else a small tag).
        bool show_action_boxes = true;

        //! \brief Console: levels shown (Messages::Level), text filter.
        bool console_levels[Messages::LEVELS] = { true, true, true, true };
        std::string console_filter;

        //! \brief Simulation: time goes this times faster than the wall
        //! clock; paused: the simulation does not evolve.
        float time_scale = 1.0f;
        bool paused = false;

        //! \brief Timing diagram: what to show, time span shown (s), end of
        //! the span when it does not follow the simulation.
        bool timing_steps = true;
        bool timing_inputs = true;
        bool timing_outputs = true;
        bool timing_used_only = true;
        float timing_span = 30.0f;
        bool timing_follow = true;
        float timing_end = 30.0f;

        //! \brief Control panel: changes of inputs applied together (F7).
        bool batch_inputs = false;
        std::map<std::string, int> pending_inputs;
        //! \brief Control panel: editing how the inputs and outputs are drawn.
        bool edit_io = false;
        //! \brief Control panel and outputs: smaller to fit their window.
        bool fit_panel = true;

        //! \brief Widths of the windows of the nets, as fractions of their
        //! total, kept when the main window changes size.
        std::vector<float> net_ratios;
        float nets_width = 0.0f;

        //! \brief Viewport state
        ImVec2 viewport_center;
        std::string title;

        //! \brief Pending requests
        bool request_quitting = false;
        bool request_new = false;
        //! \brief Frame at which to save the document (0: none), after the
        //! simulation stopped.
        int request_save_frame = 0;
        //! \brief A document load (Open) has been requested but is still pending
        //! the resolution of unsaved changes in the current document.
        bool request_load = false;
        bool request_vertical_split = false;
        //! \brief Rebuild the default layout of the windows.
        bool reset_layout = false;

        //! \brief Plot data for simulation visualization
        PlotData plot;
        //! \brief Transitions shown by the firing dates plot, per net.
        std::map<Net const*, std::vector<bool>> plot_selection;
        bool plot_subtract_drift = true;
        //! \brief (max,+) cycle time λ of the net when its simulation
        //! started (timed event graphs), to subtract the drift λ·k.
        double plot_lambda = 0.0;
        bool plot_only_outputs = true;

        //! \brief Options and cached results of the analysis dialogs.
        bool dense_matrices = false;
        bool equations_with_captions = false;
        bool equations_min_plus = false;
        TextCache adjacency[2];  // durations, tokens
        TextCache equations[2];  // counter, dater
        TextCache syslin[4];     // D, A, B, C
        //! \brief Bars of the Gantt chart, for the net of the given revision.
        std::vector<GanttBarData> gantt_bars;
        std::vector<std::string> gantt_rows;
        std::string gantt_error;
        Net const* gantt_net = nullptr;
        size_t gantt_revision = 0u;
        int gantt_computed_cycles = 0;
        //! \brief Critical cycle of the net of the given revision.
        CriticalCycleResult critical_cycle;
        Net const* critical_cycle_net = nullptr;
        size_t critical_cycle_revision = 0u;
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
    ForceDirected m_force_directed;
    //! \brief History of modifications of the net.
    History m_history;
    //! \brief Remote control for external debugging/control.
    std::unique_ptr<IRemoteControl> m_remote;
    //! \brief Critical cycle found by Howard algorithm. Also used to show
    //! where are erroneous arcs making the Petri net not be a graph event.
    std::vector<size_t> m_marked_arcs;
    //! \brief Net of the marked arcs and its number of arcs when marked.
    Net const* m_marked_net = nullptr;
    size_t m_marked_arcs_count = 0u;
    //! \brief Clipboard for copy-paste
    Clipboard m_clipboard;
    //! \brief Visualize the net and do the interaction with the user.
    //! \note Now each Document::NetEntry will have its own PetriView-like state
    PetriView m_view;
    //! \brief Messages to be displayed on the GUI.
    Messages m_messages;
    //! \brief Journal of the firings (simulations, files, runtimes).
    JournalView m_journal;
    //! \brief Nodes drawn with a halo (journal, runtime, analysis).
    NodeMarks m_marks;
    //! \brief States controlling the GUI
    mutable States m_states;
    //! \brief Cache the path to save the loaded Petri file.
    std::string m_path_to_save;
    //! \brief Optional screenshot requested before run() (Julia API, batch export).
    std::string m_auto_screenshot_path;
    bool m_auto_screenshot_pending = false;
    bool m_exit_after_screenshot = false;
    bool m_last_screenshot_ok = false;
    bool m_screenshot_window = false;
    bool m_start_simulation = false;
    int m_screenshot_defer_frames = 0;
    double m_screenshot_delay = 0.0;
    std::string m_focus_window;
    //! \brief Frames to draw before sleeping (docking and centering settle).
    static constexpr int BUSY_FRAMES = 20;
    int m_busy_frames = BUSY_FRAMES;
    //! \brief GRAFCET inputs and outputs not described by the document, with
    //! a default description for the frame (see collectGrafcetIo()).
    std::deque<GrafcetInput> m_undescribed_inputs;
    std::deque<GrafcetOutput> m_undescribed_outputs;
        //! \brief Number of nets the layout of the windows was made for.
    size_t m_layout_nets = 0u;
};

} // namespace tpne

#endif
