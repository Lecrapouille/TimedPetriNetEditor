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

#  include "Editor/DearImGui/DearUtils.hpp"
#  include <algorithm>

//------------------------------------------------------------------------------
void inputInteger(std::string const& title, size_t const maxTokens, size_t& tokens)
{
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%zu", tokens);
    ImGui::SameLine();

    float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
    ImGui::PushButtonRepeat(true);
    if (ImGui::ArrowButton("##left", ImGuiDir_Left))
    {
        tokens = std::max(size_t(1), tokens);
        --tokens;
    }
    ImGui::SameLine(0.0f, spacing);
    if (ImGui::ArrowButton("##right", ImGuiDir_Right))
    {
        tokens = std::min(maxTokens, ++tokens);
    }
    ImGui::PopButtonRepeat();
    ImGui::SameLine();
    ImGui::Text("%s", title.c_str());
}