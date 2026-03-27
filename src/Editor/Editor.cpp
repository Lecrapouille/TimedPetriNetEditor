//=============================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2026 Quentin Quadrat <lecrapouille@gmail.com>
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

#include "PetriNet/PetriNet.hpp"
#include "PetriNet/Algorithms.hpp"
#include "PetriNet/SparseMatrix.hpp"
#include "Editor/Editor.hpp"
#include "Editor/Drawable.hpp"
#include "Editor/FileDialogHelper.hpp"
#include "Editor/KeyBindings.hpp"
#include "Editor/Remote/ZeroMQRemote.hpp"
#include "imgui/imgui_internal.h"

#include <algorithm>
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
#ifdef __EMSCRIPTEN__
 constexpr float FONT_SIZE = 18.0f;
#else
 constexpr float FONT_SIZE = 13.0f;
#endif

    m_messages.setInfo("Path: " + m_path.toString());

    m_states.title = title;

    // Create initial empty document
    createDocument();

    // Set imgui.ini loading/saving location
    ImGuiIO &io = ImGui::GetIO();
    g_ini_filename = m_path.expand("imgui.ini").c_str();
    g_theme_filename = m_path.expand("theme.ini").c_str();
    io.IniFilename = g_ini_filename.c_str();

    // Setup fonts
    io.Fonts->AddFontFromFileTTF(m_path.expand("font.ttf").c_str(), FONT_SIZE);
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
    std::string theme_path = m_path.expand("theme.ini");
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
    std::string theme_path = m_path.expand("theme.ini");
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
    return doc;
}

//------------------------------------------------------------------------------
void Editor::closeDocument(Document const* doc)
{
    if (doc == nullptr)
        return;

    for (size_t i = 0; i < m_documents.size(); ++i)
    {
        if (m_documents[i].get() == doc)
        {
            // Unregister nets from the global registry before closing
            doc->unregisterNets();
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
    if (index < m_documents.size())
    {
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
    m_remote = std::make_unique<ZeroMQRemote>(*this);
    if (m_remote->start("tcp://*:5555"))
    {
        m_messages.setInfo("Remote control listening on tcp://*:5555");
    }
    else
    {
        m_messages.setError("Failed to start remote: " + m_remote->error());
    }
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
void Editor::run(Net const& net_to_load)
{
    this->net() = net_to_load;

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

    Application::run();
}

//------------------------------------------------------------------------------
void Editor::onUpdate(float const dt)
{
    if (net().modified)
    {
        title(m_states.title + " -- " + net().name + " **");

        // Validate GRAFCET receptivities when net is modified
        if (net().type() == TypeOfNet::GRAFCET)
        {
            simulation().validateReceptivities();
        }
    }
    else
    {
        title(m_states.title + " -- " + net().name);
    }

    // Step all simulations in the active document
    activeDocument().stepAllSimulations(dt);
    m_force_directed.update();

    // Poll remote control for commands
    if (m_remote && m_remote->isRunning())
    {
        m_remote->poll();
    }
}

//------------------------------------------------------------------------------
void Editor::onDraw()
{
    ImGuiID dockspace_id = ImGui::DockSpaceOverViewport();

    if (m_states.request_vertical_split && activeDocument().netCount() > 1)
    {
        auto& doc = activeDocument();
        size_t count = doc.netCount();

        ImGuiDockNode const* central_node = ImGui::DockBuilderGetCentralNode(dockspace_id);
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
    }

    menu();
    console();
    messagebox();
    inspector();
    view();
}

//------------------------------------------------------------------------------
void Editor::view()
{
    auto& doc = activeDocument();

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
        m_view.loadViewState(entry.view_state);
        m_view.reshape();

        if (is_active_view)
        {
            m_view.onHandleInput(entry.net, *entry.simulation);
        }
        m_view.drawPetriNet(entry.net, *entry.simulation, show_interactive, is_active_view);
        m_view.saveViewState(entry.view_state);
        ImGui::End();
    }

    if (doc.netCount() == 1 && !doc.getNet(0).visible)
    {
        if (doc.isModified())
        {
            m_states.request_quitting = true;
            m_states.file_dialog = States::FileDialog::SaveAs;
        }
        else
        {
            doc.getNet(0).visible = true;
            m_states.request_new = true;
        }
    }
}

//------------------------------------------------------------------------------
void Editor::close()
{
    simulation().stop();
    m_states.request_quitting = true;
}

//------------------------------------------------------------------------------
void Editor::console()
{
    ImGui::Begin("Console");
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
    if (ImGui::Button("Clear##console_clear")) {
        clearLogs();
    }

    ImGui::PopStyleVar();
    ImGui::Spacing();
    auto const& logs = getLogs();
    size_t i = logs.size();
    while (i--)
    {
        ImGui::Separator();
        if (logs[i].level == Messages::Level::Info)
        {
            ImGui::TextColored(ImVec4(0, 255, 0, 255), "%s[info]: %s",
                               logs[i].time.c_str(), logs[i].message.c_str());
        }
        else if (logs[i].level == Messages::Level::Error)
        {
            ImGui::TextColored(ImVec4(255, 0, 0, 255), "%s[error]: %s",
                               logs[i].time.c_str(), logs[i].message.c_str());
        }
        else
        {
            ImGui::TextColored(ImVec4(255, 0, 255, 255), "%s[warn]: %s",
                               logs[i].time.c_str(), logs[i].message.c_str());
        }
    }
    ImGui::End();
}

//------------------------------------------------------------------------------
void Editor::messagebox() const
{
    ImGui::Begin("Message");
    ImGui::Text("%s", getError().c_str());
    ImGui::End();
}

//------------------------------------------------------------------------------
void Editor::toogleStartSimulation()
{
    if (simulation().isRunning())
        simulation().stop();
    else
        simulation().start();
    if (simulation().isRunning())
    {
        m_view.clearAllSelections();
    }
    updateSimulationFramerate();
}

//------------------------------------------------------------------------------
void Editor::toogleStartAllSimulations()
{
    // Toggle simulation for all nets in the document
    if (activeDocument().isAnySimulationRunning())
    {
        activeDocument().stopAllSimulations();
    }
    else
    {
        activeDocument().startAllSimulations();
    }
    updateSimulationFramerate();
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
bool Editor::switchOfNet(TypeOfNet const type)
{
    if (simulation().isRunning())
        return false;

    std::vector<Arc*> arcs;
    std::string error_msg;
    if (convertTo(net(), type, error_msg, arcs))
        return true;

    m_messages.setError(error_msg);
    return false;
}

//------------------------------------------------------------------------------
Node* Editor::getNode(ImVec2 const& position)
{
    return net().findNodeAt(position.x, position.y, PLACE_RADIUS, TRANS_WIDTH);
}

//------------------------------------------------------------------------------
Place* Editor::getPlace(ImVec2 const& position)
{
    return net().findPlaceAt(position.x, position.y, PLACE_RADIUS);
}

//------------------------------------------------------------------------------
Transition* Editor::getTransition(ImVec2 const& position)
{
    return net().findTransitionAt(position.x, position.y, TRANS_WIDTH);
}

//------------------------------------------------------------------------------
Transition& Editor::addTransition(float const x, float const y)
{
    auto action = std::make_unique<NetModificationAction>([this]() -> Net& { return net(); });
    action->before(net());
    Transition& transition = net().addTransition(x, y);
    action->after(net());
    m_history.add(std::move(action));
    return transition;
}

//------------------------------------------------------------------------------
void Editor::addPlace(float const x, float const y)
{
    auto action = std::make_unique<NetModificationAction>([this]() -> Net& { return net(); });
    action->before(net());
    net().addPlace(x, y);
    action->after(net());
    m_history.add(std::move(action));
}

//------------------------------------------------------------------------------
void Editor::removeNode(Node& node)
{
    auto action = std::make_unique<NetModificationAction>([this]() -> Net& { return net(); });
    action->before(net());
    net().removeNode(node);
    action->after(net());
    m_history.add(std::move(action));
}

//------------------------------------------------------------------------------
void Editor::removeArc(Arc const& arc)
{
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
    Node& node = net().addOppositeNode(type, x, y, tokens);
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
void Editor::loadNetFile()
{
    if (simulation().isRunning())
    {
        m_messages.setError("Cannot load during the simulation!");
        return;
    }

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
    std::string ext = Path::extension(filepath);
    Importer const* importer = getImporter(ext);

    // JSON: multi-net format
    if (ext == ".json" || (importer == nullptr && ext.empty()))
    {
        std::vector<Net> nets;
        std::string error = importAllNetsFromJSON(nets, filepath);
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

        doc.setFilepath(filepath);
        doc.setModified(false);
        setSavePath(filepath);
        doc.setActiveNetIndex(0);
        m_states.request_vertical_split = true;

        // Center view on each loaded net and validate GRAFCET receptivities
        for (auto const& entry : doc.nets())
        {
            m_view.centerOnNet(entry->net);
            m_view.saveViewState(entry->view_state);

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
    m_view.centerOnNet(entry.net);
    m_view.saveViewState(entry.view_state);

    m_messages.setInfo("Loaded '" + filepath + "'");
}

//------------------------------------------------------------------------------
void Editor::importNetFrom(Importer const& importer)
{
    if (simulation().isRunning())
    {
        m_messages.setError("Cannot load during the simulation!");
        return ;
    }

    FileDialogHelper::openLoad("ImportDlgKey",
        "Choose the Petri file to load", importer.extensions.c_str());

    if (FileDialogHelper::display("ImportDlgKey", [this, &importer](std::string const& filepath)
    {
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
    if (activeDocument().netCount() > 1)
    {
        saveDocumentAs();
    }
    else
    {
        static Exporter exporter{"TimedPetriNetEditor", ".json", exportToJSON};
        exportNetTo(exporter);
    }
}

//------------------------------------------------------------------------------
void Editor::saveDocumentAs()
{
    if (activeDocument().isAnySimulationRunning())
    {
        m_messages.setError("Cannot save during the simulation!");
        return;
    }

    FileDialogHelper::openSave("SaveDocDlgKey",
        "Choose the file to save", ".json");

    if (FileDialogHelper::display("SaveDocDlgKey", [this](std::string const& path)
    {
        // Collect all nets from the document
        std::vector<Net> nets;
        auto& doc = activeDocument();
        nets.reserve(doc.netCount());
        for (size_t i = 0; i < doc.netCount(); ++i)
        {
            nets.push_back(doc.getNet(i).net);
        }

        std::string error = exportAllNetsToJSON(nets, path);
        if (error.empty())
        {
            setSavePath(path);
            doc.setFilepath(path);
            doc.setModified(false);
            m_messages.setInfo("Saved with success '" + path + "'");
            for (size_t i = 0; i < doc.netCount(); ++i)
                doc.getNet(i).net.modified = false;

            if (m_states.request_quitting)
            {
                m_states.request_quitting = false;
                halt();
            }
            if (m_states.request_new)
            {
                m_states.request_new = false;
                m_marked_arcs.clear();
                clearNet();
                m_path_to_save.clear();
            }
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
    if (simulation().isRunning())
    {
        m_messages.setError("Cannot save during the simulation!");
        return ;
    }

    if (net().isEmpty())
    {
        if (m_states.request_quitting)
        {
            m_states.request_quitting = false;
            halt();
        }
        else if (m_states.request_new)
        {
            m_marked_arcs.clear();
            clearNet();
            net().modified = false;
            m_path_to_save.clear();
            m_states.request_new = false;
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
        : "Choose the Petri file to save"));

    FileDialogHelper::openSave("ExportDlgKey", title, exporter.extensions.c_str());

    if (FileDialogHelper::display("ExportDlgKey", [this, &exporter](std::string const& path)
    {
        std::string error = exporter.exportFct(net(), path);
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
                net().modified = false;
            }
            if (m_states.request_quitting)
            {
                m_states.request_quitting = false;
                halt();
            }
            if (m_states.request_new)
            {
                m_states.request_new = false;
                m_marked_arcs.clear();
                clearNet();
                net().modified = false;
                m_path_to_save.clear();
            }
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
        m_states.request_quitting = false;
        m_states.request_new = false;
    }
}

//--------------------------------------------------------------------------
void Editor::takeScreenshot()
{
    FileDialogHelper::openSave("ScreenshotDlgKey",
        "Choose the PNG file to save the screenshot", ".png");

    if (FileDialogHelper::display("ScreenshotDlgKey", [this](std::string const& path)
    {
        if (Application::screenshot(path))
        {
            m_messages.setInfo("Screenshot taken as file '" + path + "'");
        }
        else
        {
            m_messages.setError("Failed to save screenshot to file '" + path + "'");
        }
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

    auto action = std::make_unique<NetModificationAction>([this]() -> Net& { return net(); });
    action->before(net());

    net().reset(net().type());

    action->after(net());
    m_history.add(std::move(action));
}

//--------------------------------------------------------------------------
std::string Editor::getError() const
{
    if (m_messages.getMessages().empty())
        return {};
    return m_messages.getMessage().message;
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
        m_messages.setInfo("Undo!");
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
        m_messages.setInfo("Redo!");
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
