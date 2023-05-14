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
static void inspector(PetriEditor& editor)
{
    PetriNet& net = editor.m_petri_net;

    ImGui::Begin("Places");
    for (auto& p: net.places())
    {
        ImGui::InputText(p.key.c_str(), &p.caption);
    }
    ImGui::End();

    ImGui::Begin("Transitions");
    for (auto& t: net.transitions())
    {
        ImGui::InputText(t.key.c_str(), &t.caption);
    }
    ImGui::End();

    ImGui::Begin("Arcs");
    for (auto& a: net.arcs())
    {
        if (a.from.type == Node::Type::Transition)
        {
            std::string arc(a.from.key + " -> " + a.to.key);
            ImGui::InputFloat(arc.c_str(), &a.duration, 0.01f, 1.0f, "%.3f");
        }
    }
    ImGui::End();
}

//------------------------------------------------------------------------------
static void menu(PetriEditor& editor)
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New", NULL, false))
                {/*TODO*/};

            ImGui::Separator();
            if (ImGui::MenuItem("Open", NULL, false))
                editor.load();
            if (ImGui::BeginMenu("Import"))
            {
                // TODO
                ImGui::EndMenu();
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Save", NULL, false))
                editor.save();
            if (ImGui::MenuItem("Save As", NULL, false))
                {/*TODO*/};
            if (ImGui::BeginMenu("Export"))
            {
                if (ImGui::MenuItem("To Grafcet C++", NULL, false))
                    editor.exports("C++");
                if (ImGui::MenuItem("To Julia lang", NULL, false))
                    editor.exports("Julia");
                if (ImGui::MenuItem("To PN-Editor", NULL, false))
                    editor.exports("PN-Editor");
                if (ImGui::MenuItem("To Graphviz", NULL, false))
                    editor.exports("Graphviz");
                if (ImGui::MenuItem("To Draw.io", NULL, false))
                    editor.exports("Draw.io");
                if (ImGui::MenuItem("To Symfony", NULL, false))
                    editor.exports("Symfony");
                if (ImGui::MenuItem("To LaTeX (Petri)", NULL, false))
                    editor.exports("Petri-LaTeX");
                if (ImGui::MenuItem("To LaTeX (Grafcet)", NULL, false))
                    editor.exports("Grafcet-LaTeX");
                ImGui::EndMenu();
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Exit", NULL, false))
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

            if (ImGui::MenuItem("Clear net", NULL, false))
                editor.clear();
            if (ImGui::MenuItem("Align nodes", NULL, false))
                editor.align();
            //if (ImGui::MenuItem("Show grid", NULL, false)) TODO
            //    editor.grid();
            if (ImGui::MenuItem("Take screenshot", NULL, false))
                editor.screenshot();
            ImGui::Separator();
            //if (ImGui::MenuItem("Run", NULL, false)) TODO
            //    editor.run();
            //if (ImGui::MenuItem("Stop", NULL, false)) TODO
            //    editor.stop();
            ImGui::EndMenu();
        }

        if (editor.m_petri_net.type() == PetriNet::Type::TimedGraphEvent)
        {
            if (ImGui::BeginMenu("Graph Events"))
            {
                if (ImGui::MenuItem("Show critical circuit", NULL, false))
                {
                    editor.findCriticalCycle();
                }
                // TODO call all methods concerning SysLin
                ImGui::EndMenu();
            }
        }
        ImGui::EndMenuBar();
    }
}

//------------------------------------------------------------------------------
void PetriEditor::onDrawIMGui()
{
    ::menu(*this);
    ::help(*this);
    ::about();
    ::console(*this);
    ::messagebox(*this);
    // TODO:
    ::inspector(*this);
}

