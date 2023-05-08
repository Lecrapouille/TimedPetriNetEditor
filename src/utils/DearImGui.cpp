//=============================================================================
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
//=============================================================================

// *****************************************************************************
//! \file This file just gathers imgui cpp files and around them with pragma
//! for reducing compilation warnings. Yep this is misery!
// *****************************************************************************

#include "utils/DearImGui.hpp"
#include "utils/Application.hpp"

//-----------------------------------------------------------------------------
DearImGui::DearImGui(sf::RenderWindow& renderer, DearImGui::Theme const theme)
    : m_renderer(renderer)
{
    ImGui::SFML::Init(m_renderer);
    setTheme(theme);

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;       // Enable Docking
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows

    //configurate();
}

//-----------------------------------------------------------------------------
DearImGui::~DearImGui()
{
    ImGui::SFML::Shutdown();
}

//-----------------------------------------------------------------------------
void DearImGui::configurate()
{
    m_dockspace_flags = ImGuiDockNodeFlags_None;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent
    // window not dockable into, because it would be confusing to have two
    // docking targets within each others.
    m_window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (m_opt_fullscreen)
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        m_window_flags |= ImGuiWindowFlags_NoTitleBar |
                          ImGuiWindowFlags_NoCollapse |
                          ImGuiWindowFlags_NoResize |
                          ImGuiWindowFlags_NoMove;
        m_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus |
                          ImGuiWindowFlags_NoNavFocus;
    }
    else
    {
        m_dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
    }

    if (m_dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
    {
        m_window_flags |= ImGuiWindowFlags_NoBackground;
    }

    if (!m_opt_padding)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    }
}

//-----------------------------------------------------------------------------
void DearImGui::setTheme(Theme const style)
{
    if (style == DearImGui::Theme::Classic)
    {
        ImGui::StyleColorsClassic();
    }
    else
    {
        ImGui::StyleColorsDark();
    }
}

//-----------------------------------------------------------------------------
void DearImGui::update(sf::Time const dt)
{
    ImGui::SFML::Update(m_renderer, dt);
}

//-----------------------------------------------------------------------------
void DearImGui::begin()
{
    configurate();

    ImGui::Begin("DockSpace", &m_opt_dockspace, m_window_flags);

    if (!m_opt_padding)
    {
        //std::cout << "POP 1\n";
        ImGui::PopStyleVar();
    }
    if (m_opt_fullscreen)
    {
        //std::cout << "POP 2\n";
        ImGui::PopStyleVar(2);
    }

    // Submit the DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("DockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), m_dockspace_flags);
    }
}

//-----------------------------------------------------------------------------
void DearImGui::display()
{
    ImGui::SFML::Render(m_renderer);
}

void DearImGui::end()
{
    //std::cout << "END\n";
    ImGui::End();
}