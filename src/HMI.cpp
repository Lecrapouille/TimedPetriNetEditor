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

#include "HMI.hpp"
#include "PetriEditor.hpp"
#include "project_info.hpp"

//------------------------------------------------------------------------------
static void about()
{
    ImGui::Begin("About");
    ImGui::Text("A timed Petri net and graph event editor and");
    ImGui::Text("simulator combined to (max,+) algebra with");
    ImGui::Text("wrapped API for Julia langage.");
    ImGui::Separator();
    std::string version("Version: " +
                std::to_string(project::info::major_version) + '.' +
                std::to_string(project::info::minor_version) + '.' +
                std::to_string(project::info::patch_version));
    ImGui::Text("%s", version.c_str());
    ImGui::Separator();
    ImGui::Text("Git branch: %s", project::info::git_branch.c_str());
    ImGui::Text("Git SHA1: %s", project::info::git_sha1.c_str());
    ImGui::Text("Compiled as %s", (project::info::mode == project::info::Mode::debug) ? "Debug" : "Release");
    ImGui::Separator();
    ImGui::Text("Developed by Quentin Quadrat");
    ImGui::Text("Email: lecrapouille@gmail.com");
    ImGui::Separator();
    ImGui::End();
}

//------------------------------------------------------------------------------
static void help(PetriEditor const& editor)
{
    ImGui::Begin("Help");
    ImGui::Text("%s", editor.help().str().c_str());
    ImGui::Separator();
    ImGui::Text("Data path: %s", project::info::data_path.c_str());
    ImGui::Text("Temporary path: %s", project::info::tmp_path.c_str());
    ImGui::Text("Log path: %s", project::info::log_path.c_str());
    ImGui::End();
}

//------------------------------------------------------------------------------
static void console(PetriEditor& editor)
{
    ImGui::Begin("Console");
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
    if (ImGui::Button("Clear##console_clear"))
    {
        editor.clearLogs();
    }

    ImGui::PopStyleVar();
    ImGui::Spacing();
    for (auto& it: editor.getLogs())
    {
        ImGui::Separator();
        ImGui::Text("%s %s", it.time.c_str(), it.txt.c_str());
    }
    ImGui::End();
}

//------------------------------------------------------------------------------
static void messagebox(PetriEditor const& editor)
{
    ImGui::Begin("Message");
    ImGui::Text("%s", editor.getError().c_str()); // FIXME: by copy
    ImGui::End();
}

//------------------------------------------------------------------------------
void inspector(PetriEditor& editor)
{
    PetriNet& net = editor.m_petri_net;

    // Do not allow editing when running simulation
    const auto readonly = editor.m_simulating ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None;
    ImGui::Begin("Places");
    // TODO pour event graph: afficher ImGui::InputText("T1 P0 T2", &p.places);
    for (auto& p: net.places())
    {
        ImGui::InputText(p.key.c_str(), &p.caption, readonly);
    }
    ImGui::End();

    // FIXME parse and clear sensors if and only if we modified entrytext
    ImGui::Begin("Transitions");
    if (!editor.m_simulating)
        net.m_sensors.clear();
    for (auto& transition: net.transitions())
    {
        ImGui::InputText(transition.key.c_str(), &transition.caption, readonly);
        if (!editor.m_simulating)
        {
            std::string err = net.parse(transition, true);
            if (!err.empty())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", err.c_str());
            }
        }
    }
    ImGui::End();

    ImGui::Begin("Sensors");
    for (auto& it: net.m_sensors.database())
    {
        ImGui::SliderInt(it.first.c_str(), &it.second, 0, 1);
    }
    ImGui::End();

    ImGui::Begin("Arcs");
    for (auto& a: net.arcs())
    {
        if (a.from.type == Node::Type::Transition)
        {
            std::string arc(a.from.key + " -> " + a.to.key);
            ImGui::InputFloat(arc.c_str(), &a.duration, 0.01f, 1.0f, "%.3f", readonly);
        }
    }
    ImGui::End();
}

//------------------------------------------------------------------------------
static void showDaterCounterEquation(PetriNet const& net, bool const dater, bool const counter)
{
    if (dater || counter)
    {
        const char* title = dater ? "Dater Equation" : "Counter Equation";
        ImGui::OpenPopup(title);

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal(title, NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            static bool use_caption = false;
            static bool maxplus_notation = false;
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            ImGui::Checkbox(dater ? "(max,+)" : "(min,+)", &maxplus_notation);
            ImGui::SameLine();
            ImGui::Checkbox("Use caption", &use_caption);
            ImGui::PopStyleVar();

            ImGui::Separator();
            if (dater)
            {
                ImGui::Text("%s", net.showDaterEquation("", use_caption, maxplus_notation).str().c_str());
            }
            else
            {
                ImGui::Text("%s", net.showCounterEquation("", use_caption, maxplus_notation).str().c_str());
            }

            if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }
    }
}

//------------------------------------------------------------------------------
void menu(PetriEditor& editor)
{
    bool dater = false;
    bool counter = false;

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("New", nullptr, false))
            {/*TODO*/};

        ImGui::Separator();
        if (ImGui::MenuItem("Open", nullptr, false))
            editor.load();
        if (ImGui::BeginMenu("Import"))
        {
            // TODO
            ImGui::EndMenu();
        }

        ImGui::Separator();
        if (ImGui::MenuItem("Save", nullptr, false))
            editor.save();
        if (ImGui::MenuItem("Save As", nullptr, false))
            editor.save(true);
        if (ImGui::BeginMenu("Export to"))
        {
            for (auto const& it: editor.exporters())
            {
                if (ImGui::MenuItem(it.second.title().c_str(), nullptr, false))
                    editor.exports(it.first.c_str());
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();
        if (ImGui::MenuItem("Exit", nullptr, false))
            editor.close();
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Actions"))
    {
        if (ImGui::BeginMenu("Type of net"))
        {
            static int current_type = int(editor.m_petri_net.type());

            ImGui::RadioButton("Petri net", &current_type, 0);
            ImGui::RadioButton("Timed Petri net", &current_type, 1);
            ImGui::RadioButton("Timed graph event", &current_type, 2);
            ImGui::RadioButton("GRAFCET", &current_type, 3);
            editor.changeTypeOfNet(PetriNet::Type(current_type));
            ImGui::EndMenu();
        }
        ImGui::Separator();

        if (ImGui::MenuItem("Clear net", nullptr, false))
            editor.clear();
        if (ImGui::MenuItem("Align nodes", nullptr, false))
            editor.align();
        //if (ImGui::MenuItem("Show grid", nullptr, false)) TODO
        //    editor.grid();
        if (ImGui::MenuItem("Take screenshot", nullptr, false))
            editor.screenshot();
        ImGui::Separator();
        //if (ImGui::MenuItem("Run", nullptr, false)) TODO
        //    editor.run();
        //if (ImGui::MenuItem("Stop", nullptr, false)) TODO
        //    editor.stop();
        ImGui::EndMenu();
    }

    if ((editor.m_petri_net.type() == PetriNet::Type::TimedGraphEvent) ||
        (editor.m_petri_net.isEventGraph()))
    {
        if (ImGui::BeginMenu("Graph Events"))
        {
            if (ImGui::MenuItem("Show critical circuit", nullptr, false))
            {
                editor.findCriticalCycle(); // TODO show other information
            }
            if (ImGui::MenuItem("To dynamic linear (max, +) system", nullptr, false))
            {
                SparseMatrix D; SparseMatrix A; SparseMatrix B; SparseMatrix C;
                editor.m_petri_net.toSysLin(D, A, B, C);
                SparseMatrix::display_for_julia = false;
                std::cout << "D: " << D << std::endl
                            << "A: " << A << std::endl
                            << "B: " << B << std::endl
                            << "C: " << C << std::endl;
            }
            if (ImGui::MenuItem("Show Dater equation", nullptr, false))
            {
                dater = true;
            }
            if (ImGui::MenuItem("Show Counter equation", nullptr, false))
            {
                counter = true;
            }
            if (ImGui::MenuItem("Show adjacency matrices", nullptr, false))
            {
                SparseMatrix tokens; SparseMatrix durations;
                editor.m_petri_net.toAdjacencyMatrices(tokens, durations);
                SparseMatrix::display_for_julia = false;
                std::cout << "Durations: " << durations << std::endl;
                std::cout << "Tokens: " << tokens << std::endl;
            }
            ImGui::EndMenu();
        }
    }
    ImGui::EndMenuBar();

    showDaterCounterEquation(editor.m_petri_net, dater, counter);
}

//------------------------------------------------------------------------------
// TODO: menu MQTT: ok/ko, topic, ip/port
void PetriEditor::onDrawIMGui()
{
    if (ImGui::BeginMenuBar()) {
        ::menu(*this);
    }
    ::help(*this);
    ::about();
    ::console(*this);
    ::messagebox(*this);
    ::inspector(*this);
}

