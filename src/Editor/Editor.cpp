// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "PetriNet/PetriNet.hpp"
#include "MaxPlus/Algorithms.hpp"
#include "MaxPlus/SparseMatrix.hpp"
#include "Editor/Editor.hpp"
#include "Editor/Drawable.hpp"
#include "Editor/FileDialogHelper.hpp"
#include "Editor/KeyBindings.hpp"
#include "Editor/Remote/IRemoteControl.hpp"
#  if TPNE_USE_ZEROMQ
#    include "Editor/Remote/ZeroMQRemote.hpp"
#  else
#    include "Editor/Remote/NullRemoteControl.hpp"
#  endif
#include "imgui/imgui_internal.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <fstream>

namespace tpne {

//! \brief path of the file storing dear imgui widgets. Cannot be placed
//! as member variable of Editor class.
static std::string g_ini_filename = "imgui.ini";
//! \brief path of the file storing theme settings.
static std::string g_theme_filename = "theme.ini";

//--------------------------------------------------------------------------
Editor::Editor(size_t const width, size_t const height,
               std::string const& title)
    : Application(width, height, title),
      m_path(GET_DATA_PATH),
      m_view(*this)
{
    registerMaxPlusFormats();
    m_messages.onFired = [this](Net const& fired, size_t const transition, double const date) {
        m_journal.record(fired, transition, date);
    };
#ifdef __EMSCRIPTEN__
 constexpr float FONT_SIZE = 18.0f;
#else
 constexpr float FONT_SIZE = 13.0f;
#endif

    m_states.title = title;

    // Create initial empty document
    createDocument();

    // Layout of the windows and theme: in the folder of the settings of the
    // user, not in the data folder of the application (which may be read only
    // and is shared). A first start builds the default layout.
    ImGuiIO &io = ImGui::GetIO();
    std::filesystem::path const settings = settingsFolder();
    // The name changes with the windows of the default layout: a new layout
    // is built instead of reusing one made for other windows.
    g_ini_filename = (settings / "layout-2.ini").string();
    g_theme_filename = (settings / "theme.ini").string();
    io.IniFilename = g_ini_filename.c_str();
    m_states.reset_layout = !std::filesystem::exists(g_ini_filename);

    // Setup fonts. The text font has no mathematical symbols: merge the
    // ones of math.ttf (a subset of DejaVu Sans: ⨁ ⊗ ε ↑ ↓ ≤ ≥ ...) else they
    // are drawn as '?'.
    io.Fonts->AddFontFromFileTTF(m_path.expand("font.ttf").c_str(), FONT_SIZE);
    std::string const math_font = m_path.expand("math.ttf");
    if (std::ifstream(math_font).good())
    {
        ImFontConfig config;
        config.MergeMode = true;
        io.Fonts->AddFontFromFileTTF(math_font.c_str(), FONT_SIZE, &config);
    }
    reloadFonts();

    // Load and apply theme
    loadTheme();
    applyTheme();

    // Initialize remote control
    initRemoteControl();
}

//------------------------------------------------------------------------------
Editor::~Editor()
{
    saveTheme();
}

//------------------------------------------------------------------------------
void Editor::loadTheme()
{
    std::string const& theme_path = g_theme_filename;
    std::ifstream file(theme_path);
    if (file.is_open())
    {
        int theme_id = 0;
        if (file >> theme_id)
        {
            if (theme_id >= 0 && theme_id <= 2)
            {
                theme() = static_cast<ThemeId>(theme_id);
            }
        }
    }
}

//------------------------------------------------------------------------------
void Editor::saveTheme()
{
    std::string const& theme_path = g_theme_filename;
    std::ofstream file(theme_path);
    if (file.is_open())
    {
        file << static_cast<int>(theme());
    }
}

//------------------------------------------------------------------------------
void Editor::applyTheme()
{
    switch (theme())
    {
    case ThemeId::Dark:
        ImGui::StyleColorsDark();
        break;
    case ThemeId::Light:
        ImGui::StyleColorsLight();
        break;
    case ThemeId::Calssic:
        ImGui::StyleColorsClassic();
        break;
    }

    // Softer shapes and more room than the Dear ImGui defaults.
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.PopupRounding = 4.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.ScrollbarSize = 12.0f;
    style.FramePadding = ImVec2(6.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 5.0f);
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;
    style.SeparatorTextBorderSize = 1.0f;
}

//------------------------------------------------------------------------------
std::filesystem::path Editor::settingsFolder()
{
    std::filesystem::path folder;
#if defined(_WIN32)
    if (char const* appdata = std::getenv("APPDATA"))
        folder = std::filesystem::path(appdata) / "TimedPetriNetEditor";
#elif defined(__APPLE__)
    if (char const* home = std::getenv("HOME"))
        folder = std::filesystem::path(home) / "Library" / "Application Support" / "TimedPetriNetEditor";
#else
    if (char const* xdg = std::getenv("XDG_CONFIG_HOME"); (xdg != nullptr) && (*xdg != '\0'))
        folder = std::filesystem::path(xdg) / "TimedPetriNetEditor";
    else if (char const* home = std::getenv("HOME"))
        folder = std::filesystem::path(home) / ".config" / "TimedPetriNetEditor";
#endif
    if (folder.empty())
        folder = std::filesystem::temp_directory_path() / "TimedPetriNetEditor";
    std::error_code error;
    std::filesystem::create_directories(folder, error);
    return folder;
}

//------------------------------------------------------------------------------
void Editor::setupDefaultLayout(unsigned int const id)
{
    // Nets in the middle, their inspector on the right, the GRAFCET control
    // panel and the analysis under it, the console and the GRAFCET outputs
    // at the bottom.
    ImGuiViewport const* viewport = ImGui::GetMainViewport();
    ImGui::DockBuilderRemoveNode(id);
    ImGui::DockBuilderAddNode(id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(id, viewport->WorkSize);
    ImGuiID center = id;
    ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.28f, nullptr, &center);
    ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.22f, nullptr, &center);
    ImGuiID bottom_right = ImGui::DockBuilderSplitNode(bottom, ImGuiDir_Right, 0.40f, nullptr, &bottom);
    ImGuiID right_bottom = ImGui::DockBuilderSplitNode(right, ImGuiDir_Down, 0.52f, nullptr, &right);
    for (char const* name : { "Document", "Places###Places", "Transitions", "Arcs", "Colors", "Actions" })
        ImGui::DockBuilderDockWindow(name, right);
    for (char const* name : { "Control panel", "Analysis" })
        ImGui::DockBuilderDockWindow(name, right_bottom);
    ImGui::DockBuilderDockWindow("Console", bottom);
    ImGui::DockBuilderDockWindow("Journal", bottom);
    ImGui::DockBuilderDockWindow("Program", right_bottom);
    ImGui::DockBuilderDockWindow("Timing diagram", bottom);
    ImGui::DockBuilderDockWindow("Outputs", bottom_right);
    for (int i = 0; i < 16; ++i)
        ImGui::DockBuilderDockWindow(("###Net" + std::to_string(i)).c_str(), center);
    ImGui::DockBuilderFinish(id);
}

//------------------------------------------------------------------------------
void Editor::statusBar()
{
    ImGuiWindowFlags const flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_MenuBar;
    float const height = ImGui::GetFrameHeight();
    if (ImGui::BeginViewportSideBar("##StatusBar", ImGui::GetMainViewport(), ImGuiDir_Down, height, flags))
    {
        if (ImGui::BeginMenuBar())
        {
            Net const& current = net();
            // Left: what the mouse would do. Right: counts, position, zoom.
            ImGui::TextUnformatted(m_view.hint().c_str());
            char right[256];
            size_t arcs = current.arcs().size();
            if (current.layout().compact)
                arcs /= 2u;
            // ImGui gives -FLT_MAX (finite) when the mouse is out of the window.
            ImVec2 const mouse = m_view.mouse();
            bool const inside = ImGui::IsMousePosValid();
            std::snprintf(right, sizeof(right), "%s%zu P   %zu T   %zu arcs   |   x %.0f  y %.0f   |   %d %%",
                          (m_view.selectionSize() > 0u)
                              ? (std::to_string(m_view.selectionSize()) + " selected   |   ").c_str() : "",
                          current.layout().compact ? size_t(0) : current.places().size(),
                          current.transitions().size(), arcs, inside ? double(mouse.x) : 0.0,
                          inside ? double(mouse.y) : 0.0, int(std::round(m_view.zoom() * 100.0f)));
            float const width = ImGui::CalcTextSize(right).x;
            ImGui::SameLine(std::max(ImGui::GetCursorPosX() + 20.0f,
                                     ImGui::GetWindowWidth() - width - 12.0f));
            ImGui::TextDisabled("%s", right);
            ImGui::EndMenuBar();
        }
    }
    ImGui::End();
}

//------------------------------------------------------------------------------
Document& Editor::activeDocument()
{
    if (m_documents.empty())
    {
        createDocument();
    }
    return *m_documents[m_active_document_index];
}

//------------------------------------------------------------------------------
Document const& Editor::activeDocument() const
{
    return *m_documents[m_active_document_index];
}

//------------------------------------------------------------------------------
Net& Editor::net()
{
    return activeDocument().activeNet().net;
}

//------------------------------------------------------------------------------
Net const& Editor::net() const
{
    return activeDocument().activeNet().net;
}

//------------------------------------------------------------------------------
Simulation& Editor::simulation()
{
    return *activeDocument().activeNet().simulation;
}

//------------------------------------------------------------------------------
Simulation const& Editor::simulation() const
{
    return *activeDocument().activeNet().simulation;
}

//------------------------------------------------------------------------------
void Editor::newDocument()
{
    // The journal is the one of the net of the document shown.
    m_journal.clear();
    createDocument();
    m_active_document_index = m_documents.size() - 1;
}

//------------------------------------------------------------------------------
Document& Editor::createDocument()
{
    m_documents.push_back(std::make_unique<Document>(m_messages));
    auto& doc = *m_documents.back();
    doc.addNet(TypeOfNet::PetriNet, "Net");
    doc.registerNets();
    doc.setModified(false);
    doc.activeNet().net.modified = false;
    return doc;
}

//------------------------------------------------------------------------------
void Editor::closeDocument(Document const* doc)
{
    if (doc == nullptr)
        return;
    m_journal.clear();

    for (size_t i = 0; i < m_documents.size(); ++i)
    {
        if (m_documents[i].get() == doc)
        {
            m_documents.erase(m_documents.begin() + static_cast<ptrdiff_t>(i));
            if (m_active_document_index >= m_documents.size() && !m_documents.empty())
            {
                m_active_document_index = m_documents.size() - 1;
            }
            if (m_documents.empty())
            {
                createDocument();
            }
            break;
        }
    }
}

//------------------------------------------------------------------------------
void Editor::setActiveDocument(size_t index)
{
    if ((index < m_documents.size()) && (index != m_active_document_index))
    {
        m_journal.clear();
        m_active_document_index = index;
    }
}

//------------------------------------------------------------------------------
void Editor::addNetToActiveDocument(TypeOfNet type, std::string const& name)
{
    activeDocument().addNet(type, name);
}

//------------------------------------------------------------------------------
void Editor::createGEMMADocument()
{
    // Create a new document or clear current one
    auto& doc = activeDocument();

    // Unregister old nets
    m_journal.clear();
    doc.unregisterNets();
    doc.nets().clear();

    // Create the 3 standard GEMMA graphs
    doc.addNet(TypeOfNet::GRAFCET, "Securite");
    doc.addNet(TypeOfNet::GRAFCET, "Production");
    doc.addNet(TypeOfNet::GRAFCET, "Controle");

    // Reset document state
    doc.setFilepath("");
    doc.setModified(true);
    doc.setActiveNetIndex(0);

    // Request vertical split layout
    m_states.request_vertical_split = true;

    m_messages.setInfo("Created GEMMA document with 3 graphs: Securite, Production, Controle");
}

//------------------------------------------------------------------------------
void Editor::initRemoteControl()
{
#  if TPNE_USE_ZEROMQ
    m_remote = std::make_unique<ZeroMQRemote>(*this);
    if (m_remote->start("tcp://*:5555"))
    {
        m_messages.setInfo("Remote control listening on tcp://*:5555");
    }
    else
    {
        m_messages.setError("Failed to start remote: " + m_remote->error());
    }
#  else
    m_remote = std::make_unique<NullRemoteControl>();
#  endif
}

//------------------------------------------------------------------------------
void Editor::toggleRemoteControl()
{
    if (m_remote->isRunning())
    {
        m_remote->stop();
        m_messages.setInfo("Remote control stopped");
    }
    else
    {
        if (m_remote->start("tcp://*:5555"))
        {
            m_messages.setInfo("Remote control listening on tcp://*:5555");
        }
        else
        {
            m_messages.setError("Failed to start remote: " + m_remote->error());
        }
    }
}

//------------------------------------------------------------------------------
void Editor::setSavePath(std::string const& filepath)
{
    // If we do not have exporters for the imported file format, use
    // the default file format: json.
#if 0
    if (getExporter(Path::extension(filepath)) == nullptr)
        m_path_to_save = Path::baseName(filepath) + ".json";
    else
        m_path_to_save = filepath;
#else
    m_path_to_save = Path::baseName(filepath) + ".json";
#endif
}

//------------------------------------------------------------------------------
void Editor::run(Net const& net_to_load, bool show_critical_cycle)
{
    this->net() = net_to_load;

    if (show_critical_cycle)
    {
        openAnalysis(0);
        CriticalCycleResult const cc = findCriticalCycle(this->net());
        if (cc.success)
            markArcs(this->net(), cc.arcs);
    }

    // Start the infinite loop
    Application::run();
}

//------------------------------------------------------------------------------
void Editor::run(std::string const& filepath)
{
    if (!filepath.empty())
    {
        loadDocumentFromFile(filepath);
    }
    if (m_start_simulation)
        toogleStartSimulation();

    Application::run();
}

//------------------------------------------------------------------------------
void Editor::onUpdate(float const dt)
{
    if (m_busy_frames > 0)
        --m_busy_frames;

    auto& entry = activeDocument().activeNet();
    if (entry.net.modified)
    {
        activeDocument().setModified(true);
        title(m_states.title + " -- " + entry.net.name + " **");
    }
    else
    {
        title(m_states.title + " -- " + entry.net.name);
    }

    // Check GRAFCET receptivities once per change of the net, not per frame,
    // and give the GRAFCET nets (their inputs ...) to the document engine.
    if (entry.checked_revision != entry.net.modified.revision())
    {
        if (entry.net.type() == TypeOfNet::GRAFCET)
        {
            entry.simulation->validateReceptivities();
        }
        activeDocument().registerNets();
        entry.checked_revision = entry.net.modified.revision();
    }

    // Step all simulations in the active document, at the chosen speed. A
    // pause freezes them (a stop is never paused: see toogleStartSimulation).
    if (!activeDocument().isAnySimulationRunning())
        m_states.paused = false;
    if (!m_states.paused)
        activeDocument().stepAllSimulations(dt * m_states.time_scale);
    m_force_directed.update();

    // Poll remote control for commands
    if (m_remote && m_remote->isRunning())
    {
        m_remote->poll();
    }
}

//------------------------------------------------------------------------------
bool Editor::isAnimating() const
{
    // Docking and centering need a few frames to settle after a start, a
    // load or a new layout: draw them before sleeping.
    if (m_busy_frames > 0)
        return true;
    for (auto const& entry : activeDocument().nets())
    {
        if (entry->view_state.center_request > 0)
            return true;
    }
    return activeDocument().isAnySimulationRunning() || m_monitor.isConnected() ||
           m_journal.isReplaying() ||
           m_force_directed.isRunning() ||
           (m_remote && m_remote->isRunning()) ||
           m_auto_screenshot_pending || (m_screenshot_defer_frames > 0);
}

//------------------------------------------------------------------------------
void Editor::onDraw()
{
    // The status bar first: the dock space takes the space left.
    statusBar();
    ImGuiID dockspace_id = ImGui::DockSpaceOverViewport();
    // A document with another number of nets: rebuild the layout (the nets
    // side by side), else empty panes of the previous document remain.
    if (activeDocument().netCount() != m_layout_nets)
    {
        m_layout_nets = activeDocument().netCount();
        m_states.reset_layout = true;
        m_states.request_vertical_split = (m_layout_nets > 1u);
    }
    bool layout_built = false;
    if (m_states.reset_layout)
    {
        for (auto const& entry : activeDocument().nets())
            entry->view_state.center_request = Document::ViewState::CENTER_FRAMES;
        setupDefaultLayout(dockspace_id);
        m_states.reset_layout = false;
        m_busy_frames = BUSY_FRAMES;
        layout_built = true;
    }

    // Split the nets side by side once the central node has its size (not in
    // the frame of a new layout: the sizes are not computed yet).
    ImGuiDockNode const* central = ImGui::DockBuilderGetCentralNode(dockspace_id);
    bool const sized = !layout_built && (central != nullptr) && (central->Size.x > 200.0f);
    if (m_states.request_vertical_split && activeDocument().netCount() > 1 && sized)
    {
        auto& doc = activeDocument();
        size_t count = doc.netCount();

        ImGuiDockNode const* central_node = central;
        ImGuiID target_id;

        if (central_node)
        {
            target_id = central_node->ID;
        }
        else
        {
            target_id = dockspace_id;
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id,
                ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id,
                ImGui::GetMainViewport()->Size);
        }

        std::vector<ImGuiID> dock_ids;
        ImGuiID remaining = target_id;
        for (size_t i = 0; i < count - 1; ++i)
        {
            ImGuiID dock_left;
            float ratio = 1.0f / static_cast<float>(count - i);
            ImGui::DockBuilderSplitNode(remaining, ImGuiDir_Left,
                ratio, &dock_left, &remaining);
            dock_ids.push_back(dock_left);
        }
        dock_ids.push_back(remaining);

        for (size_t i = 0; i < count; ++i)
        {
            std::string title = doc.getNet(i).net.name +
                "###Net" + std::to_string(i);
            ImGui::DockBuilderDockWindow(title.c_str(), dock_ids[i]);
        }

        ImGui::DockBuilderFinish(dockspace_id);
        m_states.request_vertical_split = false;
        // The windows changed size: fit each net again.
        for (auto const& entry : doc.nets())
            entry->view_state.center_request = Document::ViewState::CENTER_FRAMES;
        m_busy_frames = BUSY_FRAMES;
    }

    keepNetProportions();
    if (!m_connect_endpoint.empty() && (ImGui::GetFrameCount() > 2))
    {
        m_monitor_endpoint = m_connect_endpoint;
        m_connect_endpoint.clear();
        if (m_monitor.connect(m_monitor_endpoint))
            m_journal.live(m_monitor_endpoint, net());
        else
            m_messages.setError(m_monitor.error());
    }
    if (!m_focus_window.empty() && (ImGui::GetFrameCount() > 3))
    {
        // "Analysis": the properties of the net; a tab of the analysis by its
        // name: that tab ("Critical circuit" also marks the circuit).
        char const* tabs[] = { "Critical circuit", "Gantt", "Linear system", "Equations",
                               "Adjacency", "Firing dates", "Properties", "Analysis" };
        for (int i = 0; i < 8; ++i)
        {
            if (m_focus_window != tabs[i])
                continue;
            openAnalysis(std::min(i, 6));
            if (i == 0)
            {
                CriticalCycleResult const cc = findCriticalCycle(net());
                if (cc.success)
                    markArcs(net(), cc.arcs);
            }
            m_focus_window = "Analysis";
            break;
        }
        ImGui::SetWindowFocus(m_focus_window.c_str());
        m_focus_window.clear();
    }
    menu();
    console();
    m_monitor.poll(net(), m_journal, m_marks);
    m_journal.draw(net(), activeDocument().isAnySimulationRunning(), m_marks);
    programWindow();
    inspector();
    view();
    // A screenshot asked from the command line shows the windows, not the
    // messages of the moment (the console keeps them).
    if (!m_auto_screenshot_pending)
        notifications();
}

//------------------------------------------------------------------------------
void Editor::keepNetProportions()
{
    // Dear ImGui gives the change of size of the main window to the central
    // node only: the last net would shrink alone. Keep the share of each net
    // window instead (the user may change it with the splitters).
    auto& doc = activeDocument();
    size_t const count = doc.netCount();
    std::vector<ImGuiDockNode*> nodes;
    float total = 0.0f;
    for (size_t i = 0u; i < count; ++i)
    {
        ImGuiWindow* window = ImGui::FindWindowByName(("###Net" + std::to_string(i)).c_str());
        ImGuiDockNode* node = (window != nullptr) ? window->DockNode : nullptr;
        // Only nets side by side, one per dock node.
        if ((node == nullptr) || (node->ParentNode == nullptr) ||
            (node->ParentNode->SplitAxis != ImGuiAxis_X) ||
            (std::find(nodes.begin(), nodes.end(), node) != nodes.end()))
        {
            m_states.net_ratios.clear();
            m_states.nets_width = 0.0f;
            return;
        }
        nodes.push_back(node);
        total += node->Size.x;
    }
    if ((count < 2u) || (total < 1.0f))
    {
        m_states.net_ratios.clear();
        m_states.nets_width = 0.0f;
        return;
    }

    auto& ratios = m_states.net_ratios;
    bool const resized = (ratios.size() == count) && (m_states.nets_width > 0.0f) &&
                         (std::fabs(total - m_states.nets_width) >= 1.0f);
    if (resized)
    {
        // Applied by the docking at the next frame.
        for (size_t i = 0u; i < count; ++i)
            nodes[i]->SizeRef.x = std::max(1.0f, std::floor(ratios[i] * total));
        m_busy_frames = std::max(m_busy_frames, 2);
    }
    else
    {
        ratios.resize(count);
        for (size_t i = 0u; i < count; ++i)
            ratios[i] = nodes[i]->Size.x / total;
    }
    m_states.nets_width = total;
}

//------------------------------------------------------------------------------
void Editor::view()
{
    auto& doc = activeDocument();

    // A click in the window of a net makes it the active net at once: the
    // focus given by Dear ImGui comes at the end of the frame (and never for
    // the right button), so the first click would be lost.
    ImGuiWindow const* hovered = ImGui::GetCurrentContext()->HoveredWindow;
    bool const clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left) ||
                         ImGui::IsMouseClicked(ImGuiMouseButton_Right) ||
                         ImGui::IsMouseClicked(ImGuiMouseButton_Middle);
    if (clicked && (hovered != nullptr))
    {
        for (size_t i = 0; i < doc.netCount(); ++i)
        {
            ImGuiWindow* window = ImGui::FindWindowByName(("###Net" + std::to_string(i)).c_str());
            if ((window != nullptr) && (window == hovered) && (i != doc.activeNetIndex()))
            {
                doc.setActiveNetIndex(i);
                ImGui::FocusWindow(window);
            }
        }
    }

    for (size_t i = 0; i < doc.netCount(); ++i)
    {
        auto& entry = doc.getNet(i);
        if (!entry.visible)
            continue;

        std::string window_title = entry.net.name + "###Net" + std::to_string(i);
        bool* close_button = (doc.netCount() > 1) ? nullptr : &entry.visible;

        if (!ImGui::Begin(window_title.c_str(), close_button))
        {
            ImGui::End();
            continue;
        }

        if (ImGui::IsWindowFocused())
        {
            doc.setActiveNetIndex(i);
        }

        bool is_active_view = (i == doc.activeNetIndex());
        bool show_interactive = is_active_view && !entry.simulation->isRunning();
        Document::ViewState& state = entry.view_state;
        m_view.loadViewState(state);
        m_view.reshape();
        m_view.consumeUserMoved();

        // The window changed size (main window, splitters, a new layout): fit
        // the net again while the user has not moved the view, else keep the
        // point at its center.
        ImVec2 const size = m_view.size();
        if ((state.canvas_width > 0.0f) &&
            ((std::fabs(size.x - state.canvas_width) > 0.5f) ||
             (std::fabs(size.y - state.canvas_height) > 0.5f)))
        {
            if (state.auto_fit)
                state.center_request = std::max(state.center_request, 1);
            else
                m_view.keepCenter(ImVec2(state.canvas_width, state.canvas_height));
        }
        state.canvas_width = size.x;
        state.canvas_height = size.y;
        if (state.center_request > 0)
        {
            m_view.centerOnNet(entry.net);
            --state.center_request;
            state.auto_fit = true;
        }
        if (is_active_view && (state.zoom_request != 1.0f))
        {
            m_view.zoomBy(state.zoom_request);
            state.zoom_request = 1.0f;
        }

        if (is_active_view)
        {
            m_view.onHandleInput(entry.net, *entry.simulation);
        }
        m_view.drawPetriNet(entry.net, *entry.simulation, show_interactive, is_active_view);
        if (m_view.consumeUserMoved())
            state.auto_fit = false;
        m_view.saveViewState(state);

        ImGui::End();
    }

    if (doc.netCount() == 1 && !doc.getNet(0).visible)
    {
        doc.getNet(0).visible = true;
        if (hasUnsavedChanges())
            m_states.request_quitting = true;
        else
            m_states.request_new = true;
    }
}

//------------------------------------------------------------------------------
void Editor::close()
{
    simulation().stop();
    m_states.request_quitting = true;
}

//! \brief Color of the messages of a level, readable on the dark and light
//! themes.
static ImVec4 levelColor(Messages::Level const level)
{
    switch (level)
    {
    case Messages::Level::Info: return ImGui::GetStyleColorVec4(ImGuiCol_Text);
    case Messages::Level::Warning: return ImVec4(0.95f, 0.55f, 0.05f, 1.0f);
    case Messages::Level::Error: return ImVec4(0.92f, 0.22f, 0.22f, 1.0f);
    case Messages::Level::Trace: return ImVec4(0.30f, 0.60f, 0.95f, 1.0f);
    }
    return ImGui::GetStyleColorVec4(ImGuiCol_Text);
}

//! \brief Short name of a level.
static char const* levelName(Messages::Level const level)
{
    switch (level)
    {
    case Messages::Level::Info: return "info";
    case Messages::Level::Warning: return "warning";
    case Messages::Level::Error: return "error";
    case Messages::Level::Trace: return "simulation";
    }
    return "info";
}

//! \brief Message on one line: its line breaks become " | ".
static std::string oneLine(std::string const& text)
{
    std::string line;
    line.reserve(text.size());
    for (size_t i = 0u; i < text.size(); ++i)
    {
        if (text[i] == '\n')
        {
            while ((i + 1u < text.size()) && (text[i + 1u] == ' '))
                ++i;
            if (i + 1u < text.size())
                line += " | ";
        }
        else
            line += text[i];
    }
    return line;
}

//! \brief True if the text contains the filter, ignoring the case.
static bool containsNoCase(std::string const& text, std::string const& filter)
{
    if (filter.empty())
        return true;
    auto const it = std::search(text.begin(), text.end(), filter.begin(), filter.end(),
        [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) ==
                                    std::tolower(static_cast<unsigned char>(b)); });
    return it != text.end();
}

//------------------------------------------------------------------------------
void Editor::console()
{
    if (!ImGui::Begin("Console"))
    {
        ImGui::End();
        return;
    }

    // What to show: one toggle per level with its count, a text filter.
    static char const* NAMES[] = { "Info", "Warnings", "Errors", "Simulation" };
    for (size_t level = 0u; level < Messages::LEVELS; ++level)
    {
        bool& shown = m_states.console_levels[level];
        std::string const label = std::string(NAMES[level]) + " " +
            std::to_string(m_messages.count(Messages::Level(level))) + "##level" + std::to_string(level);
        ImVec4 color = levelColor(Messages::Level(level));
        if (!shown)
            color.w = 0.45f;
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::PushStyleColor(ImGuiCol_Button, shown ? ImGui::GetStyleColorVec4(ImGuiCol_FrameBgActive)
                                                     : ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
        if (ImGui::SmallButton(label.c_str()))
            shown ^= true;
        ImGui::PopStyleColor(2);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(shown ? "Click to hide these messages" : "Click to show these messages");
        ImGui::SameLine();
    }
    ImGui::SetNextItemWidth(std::max(80.0f, ImGui::GetContentRegionAvail().x - 120.0f));
    ImGui::InputTextWithHint("##console_filter", "Filter", &m_states.console_filter);
    ImGui::SameLine();
    bool const copy = ImGui::SmallButton("Copy");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Copy the shown messages");
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear"))
        clearLogs();
    ImGui::Separator();

    // Newest first. Only the visible lines are drawn (the log can be long).
    auto const& logs = getLogs();
    std::vector<size_t> shown;
    shown.reserve(logs.size());
    for (size_t i = logs.size(); i-- > 0u;)
    {
        auto const& log = logs[i];
        if (m_states.console_levels[size_t(log.level)] &&
            containsNoCase(log.message, m_states.console_filter))
            shown.push_back(i);
    }
    if (copy)
    {
        std::string text;
        for (size_t i : shown)
        {
            text += logs[i].time + "[" + levelName(logs[i].level) + "] " + logs[i].message;
            if (logs[i].count > 1u)
                text += " (x" + std::to_string(logs[i].count) + ")";
            text += "\n";
        }
        ImGui::SetClipboardText(text.c_str());
    }

    ImGui::BeginChild("##console_logs", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None,
                      ImGuiWindowFlags_HorizontalScrollbar);
    ImVec4 const dim = ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
    ImGuiListClipper clipper;
    clipper.Begin(int(shown.size()));
    while (clipper.Step())
    {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
        {
            auto const& log = logs[shown[size_t(row)]];
            ImGui::TextColored(dim, "%s", log.time.c_str());
            ImGui::SameLine(0.0f, 0.0f);
            ImVec4 const color = levelColor(log.level);
            if (log.level != Messages::Level::Info)
            {
                ImGui::TextColored(color, "%s:", levelName(log.level));
                ImGui::SameLine();
            }
            ImGui::TextColored(color, "%s", oneLine(log.message).c_str());
            if (log.count > 1u)
            {
                ImGui::SameLine();
                ImGui::TextColored(dim, "(x%zu)", log.count);
            }
            if ((log.message.find('\n') != std::string::npos) && ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", log.message.c_str());
        }
    }
    ImGui::EndChild();
    ImGui::End();
}

//------------------------------------------------------------------------------
void Editor::notifications()
{
    // The last messages (not the traces of the simulation) for a few seconds
    // in the bottom right corner, newest at the bottom; all are in the
    // console.
    static constexpr double LIFE_INFO = 3.0;
    static constexpr double LIFE_ALERT = 6.0;
    static constexpr int MAX_SHOWN = 3;
    double const now = Messages::now();
    auto const& logs = getLogs();
    // In the bottom right corner of the nets (where the user looks), above
    // the zoom of the last one.
    ImGuiViewport const* viewport = ImGui::GetMainViewport();
    ImVec2 corner = viewport->WorkPos + viewport->WorkSize - ImVec2(10.0f, 10.0f);
    float left = viewport->WorkPos.x;
    bool found = false;
    for (size_t i = 0u; i < activeDocument().netCount(); ++i)
    {
        ImGuiWindow* window = ImGui::FindWindowByName(("###Net" + std::to_string(i)).c_str());
        if ((window == nullptr) || !window->Active)
            continue;
        ImVec2 const end = window->Pos + window->Size - ImVec2(10.0f, 34.0f);
        corner = found ? ImVec2(std::max(corner.x, end.x), std::max(corner.y, end.y)) : end;
        left = found ? std::min(left, window->Pos.x) : window->Pos.x;
        found = true;
    }
    // Never out of the screen when the nets are narrow.
    corner.x = std::max(corner.x, std::min(left + 380.0f, viewport->WorkPos.x + viewport->WorkSize.x - 10.0f));
    float y = corner.y;
    float const x = corner.x;
    int shown = 0;
    for (size_t i = logs.size(); (i-- > 0u) && (shown < MAX_SHOWN);)
    {
        auto const& log = logs[i];
        if (log.level == Messages::Level::Trace)
            continue;
        double const age = now - log.stamp;
        if (age > LIFE_ALERT)
            continue;
        double const life = (log.level == Messages::Level::Info) ? LIFE_INFO : LIFE_ALERT;
        if (age > life)
            continue;
        float const alpha = float(std::clamp((life - age) / 0.5, 0.0, 1.0));

        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::SetNextWindowBgAlpha(0.94f * alpha);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
        ImGuiWindowFlags const flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoInputs;
        std::string const id = "##notification" + std::to_string(shown);
        if (ImGui::Begin(id.c_str(), nullptr, flags))
        {
            ImVec4 const color = levelColor(log.level);
            ImVec2 const p = ImGui::GetWindowPos();
            ImGui::GetWindowDrawList()->AddRectFilled(p, p + ImVec2(4.0f, ImGui::GetWindowHeight()),
                                                      ImGui::GetColorU32(color), 6.0f,
                                                      ImDrawFlags_RoundCornersLeft);
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 340.0f);
            if (log.level != Messages::Level::Info)
                ImGui::TextColored(color, "%s", (log.level == Messages::Level::Error) ? "Error" : "Warning");
            ImGui::TextUnformatted(log.message.c_str());
            if (log.count > 1u)
                ImGui::TextDisabled("(x%zu)", log.count);
            ImGui::PopTextWrapPos();
            y -= ImGui::GetWindowHeight() + 6.0f;
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
        ++shown;
    }
}

//------------------------------------------------------------------------------
void Editor::toogleStartSimulation()
{
    m_states.paused = false;
    // The GRAFCET graphs of a document run together (shared engine).
    if (net().type() == TypeOfNet::GRAFCET)
    {
        toogleStartAllSimulations();
        return;
    }

    if (simulation().isRunning())
        simulation().stop();
    else
    {
        // The replayed tokens are not the initial marking.
        m_journal.stopReplay();
        computeCycleTime();
        simulation().start();
    }
    if (simulation().isRunning())
    {
        m_view.clearAllSelections();
    }
    updateSimulationFramerate();
}

//------------------------------------------------------------------------------
void Editor::toogleStartAllSimulations()
{
    m_states.paused = false;
    // Toggle simulation for all nets in the document
    if (activeDocument().isAnySimulationRunning())
    {
        activeDocument().stopAllSimulations();
    }
    else
    {
        m_journal.stopReplay();
        computeCycleTime();
        activeDocument().startAllSimulations();
    }
    updateSimulationFramerate();
}

//------------------------------------------------------------------------------
void Editor::computeCycleTime()
{
    // Computed on the initial marking, for the timed event graphs only.
    m_states.plot_lambda = 0.0;
    if ((net().type() == TypeOfNet::TimedPetriNet) && isEventGraph(net()))
    {
        CriticalCycleResult const cc = findCriticalCycle(net());
        if (cc.success && !cc.durations.empty())
            m_states.plot_lambda = cc.durations.front();
    }
}

//------------------------------------------------------------------------------
void Editor::updateSimulationFramerate()
{
    // Set framerate based on whether any simulation is running
    // Lower framerate during simulation to have bigger time steps
    bool any_running = activeDocument().isAnySimulationRunning();
    framerate(any_running ? 30 : 60);
}

//------------------------------------------------------------------------------
bool Editor::switchOfNet(TypeOfNet const type, bool const compact)
{
    if (simulation().isRunning())
        return false;

    std::vector<size_t> arcs;
    std::string error_msg;
    bool const converted = compact ? convertToEventGraph(net(), error_msg, arcs)
                                   : convertTo(net(), type, error_msg, arcs);
    if (converted)
    {
        simulation().validateReceptivities();
        activeDocument().registerNets();
        return true;
    }

    m_messages.setError(error_msg);
    return false;
}

//------------------------------------------------------------------------------
Node* Editor::getNode(ImVec2 const& position)
{
    return nodeAt(net(), position, PLACE_RADIUS, TRANS_WIDTH);
}

//------------------------------------------------------------------------------
Place* Editor::getPlace(ImVec2 const& position)
{
    return placeAt(net(), position, PLACE_RADIUS);
}

//------------------------------------------------------------------------------
Transition* Editor::getTransition(ImVec2 const& position)
{
    return transitionAt(net(), position, TRANS_WIDTH);
}

//------------------------------------------------------------------------------
Transition& Editor::addTransition(float const x, float const y)
{
    auto action = std::make_unique<NetModificationAction>([this]() -> Net& { return net(); });
    action->before(net());
    Transition& transition = net().addTransition();
    net().position(transition) = { x, y };
    action->after(net());
    m_history.add(std::move(action));
    return transition;
}

//------------------------------------------------------------------------------
void Editor::addPlace(float const x, float const y)
{
    auto action = std::make_unique<NetModificationAction>([this]() -> Net& { return net(); });
    action->before(net());
    net().position(net().addPlace()) = { x, y };
    action->after(net());
    m_history.add(std::move(action));
}

//------------------------------------------------------------------------------
void Editor::removeNode(Node& node)
{
    auto action = std::make_unique<NetModificationAction>([this]() -> Net& { return net(); });
    action->before(net());
    // A removed step gives its number to the last step: the other graphs of
    // the document that read it ("Production:X3") follow (this graph is done
    // by the net itself).
    size_t const last = net().places().empty() ? 0u : net().places().back().id;
    bool const renumbered = (net().type() == TypeOfNet::GRAFCET) && (node.type == Node::Type::Place);
    size_t const removed = node.id;
    net().removeNode(node);
    if (renumbered)
    {
        for (auto const& entry : activeDocument().nets())
        {
            if (&entry->net != &net())
            {
                entry->net.renameStepOf(net().name, removed, last + 1u);
                entry->net.renameStepOf(net().name, last, removed);
            }
        }
    }
    action->after(net());
    m_history.add(std::move(action));
    net().modified = true;
}

//------------------------------------------------------------------------------
void Editor::setArc(Arc& arc, ArcKind const kind, size_t const weight)
{
    auto action = std::make_unique<NetModificationAction>([this]() -> Net& { return net(); });
    action->before(net());
    arc.kind = (arc.from.type == Node::Type::Place) ? kind : ArcKind::Normal;
    arc.weight = std::max(size_t(1u), weight);
    net().modified = true;
    action->after(net());
    m_history.add(std::move(action));
}

//------------------------------------------------------------------------------
void Editor::setArc(Arc& arc, float const duration, size_t const weight, size_t const tokens)
{
    auto action = std::make_unique<NetModificationAction>([this]() -> Net& { return net(); });
    action->before(net());
    if (arc.from.type == Node::Type::Transition)
        arc.duration = std::max(0.0f, duration);
    arc.weight = std::max(size_t(1u), weight);
    if (net().layout().compact)
        net().placeOf(arc).tokens = tokens;
    net().modified = true;
    action->after(net());
    m_history.add(std::move(action));
}

//------------------------------------------------------------------------------
void Editor::setPlace(Place& place, size_t const tokens, size_t const capacity)
{
    auto action = std::make_unique<NetModificationAction>([this]() -> Net& { return net(); });
    action->before(net());
    place.capacity = std::max(size_t(1u), capacity);
    place.tokens = std::min(tokens, place.capacity);
    net().modified = true;
    action->after(net());
    m_history.add(std::move(action));
}

//------------------------------------------------------------------------------
void Editor::setArcColor(Arc& arc, std::string const& color)
{
    auto action = std::make_unique<NetModificationAction>([this]() -> Net& { return net(); });
    action->before(net());
    arc.color = (color.empty() || (color == "any")) ? AnyColor : net().colors.add(color);
    net().modified = true;
    action->after(net());
    m_history.add(std::move(action));
}

//------------------------------------------------------------------------------
void Editor::removeArc(Arc const& arc)
{
    // In a timed event graph an arc between two transitions is made of a
    // hidden place and its two arcs: remove the place, which removes both.
    if (net().layout().compact &&
        (arc.to.type == Node::Type::Place))
    {
        removeNode(net().node(arc.to));
        return;
    }

    auto action = std::make_unique<NetModificationAction>([this]() -> Net& { return net(); });
    action->before(net());
    net().removeArc(arc);
    action->after(net());
    m_history.add(std::move(action));
    net().modified = true;
}

//------------------------------------------------------------------------------
Node& Editor::addOppositeNode(Node::Type const type, float const x,
    float const y, size_t const tokens)
{
    auto action = std::make_unique<NetModificationAction>([this]() -> Net& { return net(); });
    action->before(net());
    Node& node = net().addOppositeNode(type, tokens);
    net().position(node) = { x, y };
    action->after(net());
    m_history.add(std::move(action));
    return node;
}

//------------------------------------------------------------------------------
void Editor::addArc(Node& from, Node& to, float const duration)
{
    auto action = std::make_unique<NetModificationAction>([this]() -> Net& { return net(); });
    action->before(net());
    auto error = net().addArc(from, to, duration);
    if (!error.empty())
    {
        m_messages.setError(error);
        return;
    }
    action->after(net());
    m_history.add(std::move(action));
}

//------------------------------------------------------------------------------
bool Editor::stopSimulationFor(char const* what)
{
    // The simulation changes the marking: stop it first. The marking is
    // restored at the next update, so the file operation goes on at the next
    // frame (the caller keeps its request).
    if (!activeDocument().isAnySimulationRunning())
        return false;
    m_states.paused = false;
    activeDocument().stopAllSimulations();
    updateSimulationFramerate();
    m_messages.setInfo(std::string("Simulation stopped to ") + what);
    return true;
}

//------------------------------------------------------------------------------
void Editor::loadNetFile()
{
    if (stopSimulationFor("open a file"))
        return;

    FileDialogHelper::openLoad("LoadFileDlgKey",
        "Choose the Petri file to load", ".json");

    if (FileDialogHelper::display("LoadFileDlgKey",
        [this](std::string const& filepath) { loadDocumentFromFile(filepath); }))
    {
        m_states.file_dialog = States::FileDialog::None;
    }
}

//------------------------------------------------------------------------------
void Editor::loadDocumentFromFile(std::string const& filepath)
{
    // The firings of the previous nets mean nothing for the loaded ones.
    m_journal.clear();
    std::string ext = Path::extension(filepath);
    Importer const* importer = getImporter(ext);

    // JSON: multi-net format
    if (ext == ".json" || (importer == nullptr && ext.empty()))
    {
        std::vector<Net> nets;
        DocumentInfo info;
        std::string error = importAllNetsFromJSON(nets, filepath, &info);
        if (!error.empty())
        {
            m_messages.setError(error);
            return;
        }

        if (nets.empty())
        {
            m_messages.setError("No nets found in file '" + filepath + "'");
            return;
        }

        activeDocument().unregisterNets();
        auto& doc = activeDocument();
        doc.nets().clear();

        for (auto const& loaded_net : nets)
        {
            auto& entry = doc.addNet(loaded_net.type(), loaded_net.name);
            entry.net = std::move(loaded_net);
        }
        doc.info() = std::move(info);
        m_marked_arcs.clear();
        m_history.clear();

        doc.setFilepath(filepath);
        doc.setModified(false);
        setSavePath(filepath);
        doc.setActiveNetIndex(0);
        m_states.request_vertical_split = true;

        // Center view on each loaded net and validate GRAFCET receptivities
        m_busy_frames = BUSY_FRAMES;
        for (auto const& entry : doc.nets())
        {
            entry->view_state.center_request = Document::ViewState::CENTER_FRAMES;

            // Validate GRAFCET receptivities after loading
            if (entry->net.type() == TypeOfNet::GRAFCET)
            {
                entry->simulation->validateReceptivities();
            }
        }

        m_messages.setInfo("Loaded '" + filepath + "' with " +
            std::to_string(nets.size()) + " net(s)");
        return;
    }

    // Other formats: single net via importer
    if (importer == nullptr)
    {
        m_messages.setError("Cannot import '" + filepath + "'. Reason: 'unknown file extension'");
        return;
    }

    m_marked_arcs.clear();
    activeDocument().unregisterNets();
    auto& doc = activeDocument();
    doc.nets().clear();
    doc.info() = DocumentInfo{};
    auto& entry = doc.addNet(TypeOfNet::TimedPetriNet, "Imported");
    std::string error = importer->importFct(entry.net, filepath);

    if (!error.empty())
    {
        m_messages.setError(error);
        doc.nets().clear();
        doc.addNet(TypeOfNet::TimedPetriNet, "Empty");
        return;
    }

    doc.setFilepath(filepath);
    doc.setModified(false);
    setSavePath(filepath);
    doc.setActiveNetIndex(0);
    if (importer->springify)
        springify();

    // Center view on the loaded net
    entry.view_state.center_request = Document::ViewState::CENTER_FRAMES;
    m_busy_frames = BUSY_FRAMES;

    m_messages.setInfo("Loaded '" + filepath + "'");
}

//------------------------------------------------------------------------------
void Editor::importNetFrom(Importer const& importer)
{
    if (stopSimulationFor("import a file"))
        return;

    FileDialogHelper::openLoad("ImportDlgKey",
        "Choose the Petri file to load", importer.extensions.c_str());

    if (FileDialogHelper::display("ImportDlgKey", [this, &importer](std::string const& filepath)
    {
        m_journal.clear();
        m_marked_arcs.clear();
        net().clear();
        std::string error = importer.importFct(net(), filepath);
        if (error.empty())
        {
            if (m_states.file_dialog == States::FileDialog::Import)
                m_messages.setInfo("Imported with success from '" + filepath + "'");
            else
                m_messages.setInfo("Loaded with success '" + filepath + "'");
            setSavePath(filepath);
            activeDocument().setFilepath(filepath);
            net().modified = false;
            if (importer.springify)
            {
                springify();
            }
        }
        else
        {
            m_messages.setError(error);
            net().clear();
            net().modified = true;
            m_force_directed.reset();
        }
    }))
    {
        m_states.file_dialog = States::FileDialog::None;
        m_states.pending_importer = nullptr;
    }
}

//--------------------------------------------------------------------------
void Editor::springify()
{
    m_force_directed.reset(m_view.size().x, m_view.size().y, net());
}

//--------------------------------------------------------------------------
void Editor::saveNetAs()
{
    // Always the whole document: its nets and their description.
    saveDocumentAs();
}

//------------------------------------------------------------------------------
std::string Editor::writeDocument(std::string const& path)
{
    m_journal.stopReplay();
    std::vector<Net> nets;
    auto& doc = activeDocument();
    doc.pruneInfo();
    nets.reserve(doc.netCount());
    for (size_t i = 0; i < doc.netCount(); ++i)
        nets.push_back(doc.getNet(i).net);

    std::string const error = exportAllNetsToJSON(nets, path, &doc.info());
    if (error.empty())
    {
        setSavePath(path);
        doc.setFilepath(path);
        doc.setModified(false);
        for (size_t i = 0; i < doc.netCount(); ++i)
            doc.getNet(i).net.modified = false;
    }
    return error;
}

//------------------------------------------------------------------------------
void Editor::saveDocumentAs()
{
    if (stopSimulationFor("save"))
        return;

    FileDialogHelper::openSave("SaveDocDlgKey",
        "Choose the file to save", ".json");

    if (FileDialogHelper::display("SaveDocDlgKey", [this](std::string const& path)
    {
        std::string error = writeDocument(path);
        if (error.empty())
        {
            m_messages.setInfo("Saved with success '" + path + "'");
            if (m_states.request_quitting || m_states.request_new || m_states.request_load)
                fulfillPendingDocumentRequest();
        }
        else
        {
            m_messages.setError(error);
        }
    }))
    {
        m_states.file_dialog = States::FileDialog::None;
    }
}

//------------------------------------------------------------------------------
void Editor::exportNetTo(Exporter const& exporter)
{
    if (stopSimulationFor("export"))
        return;

    if (net().isEmpty())
    {
        if (m_states.request_quitting || m_states.request_new || m_states.request_load)
        {
            discardUnsavedChanges();
            fulfillPendingDocumentRequest();
            m_states.file_dialog = States::FileDialog::None;
        }
        else
        {
            m_messages.setError("Cannot save dummy net!");
        }
        return ;
    }

    const char* title = (m_states.file_dialog == States::FileDialog::Export) ? "Choose the Petri file to export"
        : (m_states.request_quitting ? "Choose the Petri file to save before quitting"
        : (m_states.request_new ? "Choose the Petri file to save before creating new document"
        : (m_states.request_load ? "Choose the Petri file to save before opening another document"
        : "Choose the Petri file to save")));

    FileDialogHelper::openSave("ExportDlgKey", title, exporter.extensions.c_str());

    if (FileDialogHelper::display("ExportDlgKey", [this, &exporter](std::string const& path)
    {
        // The GRAFCETs of a document run together: exported together.
        std::string error;
        if ((exporter.exportAllFct != nullptr) && (activeDocument().netCount() > 1u))
        {
            std::vector<Net> nets;
            for (size_t i = 0u; i < activeDocument().netCount(); ++i)
                nets.push_back(activeDocument().getNet(i).net);
            error = exporter.exportAllFct(nets, path);
        }
        else
        {
            error = exporter.exportFct(net(), path);
        }
        if (error.empty())
        {
            if (m_states.file_dialog == States::FileDialog::Export)
            {
                m_messages.setInfo("Exported with success '" + path + "'");
            }
            else
            {
                setSavePath(path);
                m_messages.setInfo("Saved with success '" + path + "'");
                activeDocument().setModified(false);
                net().modified = false;
            }
            if (m_states.request_quitting || m_states.request_new || m_states.request_load)
                fulfillPendingDocumentRequest();
        }
        else
        {
            m_messages.setError(error);
            net().modified = true;
        }
    }))
    {
        m_states.file_dialog = States::FileDialog::None;
        m_states.pending_exporter = nullptr;
    }
}

//--------------------------------------------------------------------------
void Editor::setAutoScreenshot(std::string const& path, bool const exit_after,
                               bool const whole_window)
{
    m_screenshot_window = whole_window;
    m_auto_screenshot_path = path;
    m_auto_screenshot_pending = !path.empty();
    m_exit_after_screenshot = exit_after;
    m_last_screenshot_ok = false;
    // Let the windows and the docking layout settle before the capture.
    m_screenshot_defer_frames = std::max(m_screenshot_defer_frames, 10);
}

//--------------------------------------------------------------------------
bool Editor::saveScreenshot(std::string const& path)
{
    if (m_screenshot_window)
        return Application::screenshot(path);

    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    if (!m_view.screenshotRect(x, y, width, height))
        return false;

    return Application::screenshot(path, x, y, width, height);
}

//--------------------------------------------------------------------------
void Editor::onFrameEnd()
{
    if (!m_auto_screenshot_pending || m_auto_screenshot_path.empty())
        return;

    if (m_screenshot_defer_frames > 0)
    {
        --m_screenshot_defer_frames;
        return;
    }
    if (ImGui::GetTime() < m_screenshot_delay)
        return;

    std::string const path = m_auto_screenshot_path;
    m_last_screenshot_ok = saveScreenshot(path);
    m_auto_screenshot_pending = false;
    m_auto_screenshot_path.clear();

    if (m_last_screenshot_ok)
        m_messages.setInfo("Screenshot taken as file '" + path + "'");
    else
        m_messages.setError("Failed to save screenshot to file '" + path + "'");

    if (m_exit_after_screenshot)
        halt();
}

//--------------------------------------------------------------------------
void Editor::takeScreenshot()
{
    FileDialogHelper::openSave("ScreenshotDlgKey",
        "Choose the PNG file to save the screenshot", ".png");

    if (FileDialogHelper::display("ScreenshotDlgKey", [this](std::string const& path)
    {
        setAutoScreenshot(path, false);
        m_screenshot_defer_frames = 1;
    }))
    {
        m_states.file_dialog = States::FileDialog::None;
    }
}

//--------------------------------------------------------------------------
void Editor::clearNet()
{
    if (simulation().isRunning())
        return ;

    m_journal.clear();
    auto action = std::make_unique<NetModificationAction>([this]() -> Net& { return net(); });
    action->before(net());

    net().reset(net().type());

    action->after(net());
    m_history.add(std::move(action));
}

//------------------------------------------------------------------------------
bool Editor::hasUnsavedChanges() const
{
    if (activeDocument().isModified())
        return true;

    auto const& doc = activeDocument();
    for (size_t i = 0; i < doc.netCount(); ++i)
    {
        if (doc.getNet(i).net.modified)
            return true;
    }

    return false;
}

//------------------------------------------------------------------------------
void Editor::discardUnsavedChanges()
{
    m_states.paused = false;
    activeDocument().stopAllSimulations();

    auto& doc = activeDocument();
    doc.setModified(false);
    for (size_t i = 0; i < doc.netCount(); ++i)
        doc.getNet(i).net.modified = false;

    m_path_to_save.clear();
    m_history.clear();
}

//------------------------------------------------------------------------------
void Editor::fulfillPendingDocumentRequest()
{
    if (m_states.request_quitting)
    {
        m_states.request_quitting = false;
        halt();
    }
    else if (m_states.request_new)
    {
        m_states.request_new = false;
        m_marked_arcs.clear();
        m_path_to_save.clear();
        clearNet();
    }
    else if (m_states.request_load)
    {
        // The current document has been dealt with (saved or discarded); now
        // open the load file dialog. loadDocumentFromFile() replaces the nets.
        m_states.request_load = false;
        m_states.file_dialog = States::FileDialog::Load;
    }
}

//--------------------------------------------------------------------------
std::vector<Messages::TimedMessage> const& Editor::getLogs() const
{
    return m_messages.getMessages();
}

//--------------------------------------------------------------------------
void Editor::clearLogs()
{
    m_messages.clear();
}

//--------------------------------------------------------------------------
void Editor::undo()
{
    if (simulation().isRunning())
        return ;

    if (!m_history.undo())
    {
        m_messages.setInfo("Cannot do more undos!");
    }
    else
    {
        m_messages.setInfo("Undo");
        m_view.clearAllSelections();
        net().modified = true;
    }
}

//--------------------------------------------------------------------------
void Editor::redo()
{
    if (simulation().isRunning())
        return ;

    if (!m_history.redo())
    {
        m_messages.setInfo("Cannot do more redos!");
    }
    else
    {
        m_messages.setInfo("Redo");
        m_view.clearAllSelections();
        net().modified = true;
    }
}

//------------------------------------------------------------------------------
void Editor::pasteFromClipboard(Net& target_net, ImVec2 const& position,
                                std::vector<Node*>& selected_nodes)
{
    auto action = std::make_unique<NetModificationAction>([this]() -> Net& { return net(); });
    action->before(target_net);

    m_clipboard.paste(target_net, position.x, position.y, selected_nodes);

    action->after(target_net);
    m_history.add(std::move(action));
    target_net.modified = true;
}

} // namespace tpne
