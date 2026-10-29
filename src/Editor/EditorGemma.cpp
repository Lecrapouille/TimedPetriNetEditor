// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// ****************************************************************************
//! \file The GEMMA window: the procedures kept (A, F, D), the links between
//! them with their conditions, the checks, and the GRAFCETs Safety, Control
//! and Production made from them.
// ****************************************************************************

#include "Editor/Editor.hpp"
#include "Editor/Theme.hpp"
#include "Editor/TopDownLayout.hpp"
#include "PetriNet/Gemma.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>

namespace tpne {

//------------------------------------------------------------------------------
//! \brief The inputs the generated GRAFCETs usually read, drawn as on a
//! machine (the others are switches).
static GrafcetInput gemmaInput(std::string const& name)
{
    GrafcetInput input;
    input.name = name;
    if (name == "EStop")
    {
        input.kind = GrafcetInput::Kind::Emergency;
        input.description = "Emergency stop (mushroom button)";
    }
    else if (name == "Rearm")
    {
        input.kind = GrafcetInput::Kind::Button;
        input.color = "blue";
        input.description = "Rearm after an emergency stop";
    }
    else if (name == "Ack")
    {
        input.kind = GrafcetInput::Kind::Button;
        input.color = "white";
        input.description = "Acknowledge (after a failure)";
    }
    else if (name == "Start")
    {
        input.kind = GrafcetInput::Kind::Button;
        input.color = "green";
        input.description = "Start of the production";
    }
    else if ((name == "Stop") || (name == "Pause"))
    {
        input.kind = GrafcetInput::Kind::Button;
        input.color = (name == "Stop") ? "red" : "yellow";
        input.description = (name == "Stop") ? "Stop at the end of the cycle" : "Stop in a given state";
    }
    return input;
}

//------------------------------------------------------------------------------
void Editor::generateFromGemma()
{
    Document& doc = activeDocument();
    if (!doc.info().gemma || stopSimulationFor("make the GRAFCETs of the GEMMA"))
        return;
    Gemma const& gemma = *doc.info().gemma;

    Net safety(TypeOfNet::GRAFCET);
    Net control(TypeOfNet::GRAFCET);
    generateGemma(gemma, safety, control);
    arrangeTopDown(safety);
    arrangeTopDown(control);

    // Production is the work of the user: kept, its references to the steps
    // of Control follow their procedures. Made when there is none.
    // The actions the user gave to the procedures are kept.
    if (Document::NetEntry const* old = doc.findNet("Control"))
        keepControlActions(old->net, control);
    Document::NetEntry* production = doc.findNet("Production");
    Net made(TypeOfNet::GRAFCET);
    if (production != nullptr)
    {
        Document::NetEntry const* old = doc.findNet("Control");
        renumberControlSteps(production->net, (old != nullptr) ? gemmaSteps(old->net)
                                                               : std::map<std::string, size_t>{},
                             gemmaSteps(control));
    }
    else
    {
        gemmaProductionTemplate(gemma, control, made);
        arrangeTopDown(made);
    }

    m_journal.clear();
    m_marked_arcs.clear();
    m_history.clear();
    doc.unregisterNets();
    // The empty nets of a new document go away.
    for (size_t i = doc.nets().size(); i-- > 0u;)
    {
        std::string const& name = doc.nets()[i]->net.name;
        if ((name != "Safety") && (name != "Control") && (name != "Production") && doc.nets()[i]->net.isEmpty())
            doc.nets().erase(doc.nets().begin() + static_cast<ptrdiff_t>(i));
    }
    auto const put = [&doc](Net& net) {
        Document::NetEntry* entry = doc.findNet(net.name);
        if (entry == nullptr)
            entry = &doc.addNet(TypeOfNet::GRAFCET, net.name);
        entry->net = std::move(net);
    };
    put(safety);
    put(control);
    if (production == nullptr)
        put(made);

    // The usual inputs drawn as on a machine.
    std::vector<std::string> names = gemmaInputNames(gemma);
    names.insert(names.end(), { "EStop", "Rearm" });
    for (std::string const& name : names)
    {
        GrafcetInput const input = gemmaInput(name);
        if ((input.kind != GrafcetInput::Kind::Switch) && (findInput(doc.info(), name) == nullptr))
            doc.info().inputs.push_back(input);
    }

    doc.registerNets();
    doc.setActiveNetIndex(0);
    doc.setModified(true);
    m_front_net = BUSY_FRAMES;
    m_busy_frames = BUSY_FRAMES;
    for (auto const& entry : doc.nets())
    {
        entry->net.modified = true;
        entry->view_state.center_request = Document::ViewState::CENTER_FRAMES;
        if (entry->net.type() == TypeOfNet::GRAFCET)
            entry->simulation->validateReceptivities();
    }
    m_messages.setInfo("GRAFCETs Safety, Control" + std::string((production == nullptr) ? ", Production" : "") +
                       " made from the GEMMA");
}

//------------------------------------------------------------------------------
//! \brief Color from the problems: red for an error, orange for a warning.
//! The problems of a link (link >= 0), else of a procedure without its links
//! (code), else all of them.
static ImVec4 issueColor(std::vector<GemmaIssue> const& issues, std::string const& code, int link)
{
    bool warning = false;
    for (auto const& issue : issues)
    {
        bool const match = (link >= 0) ? (issue.link == link)
                                       : (code.empty() || ((issue.code == code) && (issue.link < 0)));
        if (match && issue.error)
            return ImGui::ColorConvertU32ToFloat4(palette().critical);
        warning = warning || match;
    }
    return warning ? ImVec4(0.95f, 0.6f, 0.1f, 1.0f) : ImGui::GetStyleColorVec4(ImGuiCol_Text);
}

//! \brief Color of the procedure selected and of its links.
static ImVec4 const GEMMA_SELECTED(0.3f, 0.85f, 0.45f, 1.0f);
//! \brief Color of the links arriving at the procedure selected.
static ImVec4 const GEMMA_ARRIVING(0.4f, 0.65f, 1.0f, 1.0f);

//------------------------------------------------------------------------------
//! \brief A rectangle of the GEMMA form, and where its name is written: the
//! positions of doc/gemma/GEMMA.drawio, in its units (A1 30 to the right, room
//! for the condition of A6 -> A1).
struct GemmaBox { char const* code; float x, y, w, h; float lx, ly, lw; };
static GemmaBox const GEMMA_BOXES[] = {
    { "A6", 199.0f, 133.91f, 170.0f, 60.0f, 219.0f, 148.91f, 130.0f },
    { "A1", 432.0f, 133.91f, 138.0f, 60.0f, 461.0f, 148.91f, 85.0f },
    { "A7", 267.0f, 214.91f, 120.0f, 80.0f, 273.0f, 221.91f, 110.0f },
    { "A4", 554.0f, 224.91f, 97.0f, 60.0f, 572.0f, 239.91f, 60.0f },
    { "A5", 199.0f, 333.91f, 170.0f, 98.09f, 219.0f, 348.91f, 130.0f },
    { "A2", 411.0f, 351.91f, 120.0f, 82.09f, 433.0f, 375.91f, 77.0f },
    { "A3", 543.0f, 352.91f, 120.0f, 79.0f, 564.0f, 366.91f, 78.0f },
    { "D2", 276.5f, 504.91f, 100.0f, 105.09f, 288.0f, 519.0f, 75.0f },
    { "D3", 443.0f, 528.91f, 109.0f, 57.09f, 448.0f, 533.0f, 100.0f },
    { "D1", 211.0f, 653.91f, 280.0f, 60.0f, 213.0f, 664.91f, 270.0f },
    { "F2", 749.0f, 232.91f, 71.0f, 118.0f, 751.5f, 241.91f, 66.0f },
    { "F3", 841.0f, 231.91f, 81.0f, 119.0f, 840.0f, 239.91f, 86.0f },
    { "F1", 727.0f, 386.91f, 205.0f, 159.0f, 762.0f, 391.91f, 140.0f },
    { "F4", 972.0f, 165.91f, 143.0f, 110.0f, 974.0f, 176.91f, 133.0f },
    { "F5", 972.0f, 285.91f, 143.0f, 114.0f, 976.0f, 296.91f, 135.0f },
    { "F6", 972.0f, 446.91f, 147.0f, 80.0f, 980.5f, 451.91f, 130.0f },
};

//------------------------------------------------------------------------------
//! \brief An arrow of the GEMMA form: its path, from the border of a
//! procedure to the border of another one, as drawn in GEMMA.drawio.
struct GemmaRoute { char const* from; char const* to; std::vector<ImVec2> path; };
static std::vector<GemmaRoute> const& gemmaRoutes()
{
    static std::vector<GemmaRoute> const routes = {
        { "A1", "F1", { { 570.0f, 178.91f }, { 739.3f, 178.91f }, { 739.3f, 386.91f } } },
        { "A1", "F2", { { 570.0f, 163.91f }, { 784.5f, 163.91f }, { 784.5f, 232.91f } } },
        { "A1", "F4", { { 501.0f, 133.91f }, { 501.0f, 114.91f }, { 1007.75f, 114.91f }, { 1007.75f, 165.91f } } },
        { "A1", "F5", { { 501.0f, 133.91f }, { 501.0f, 114.91f }, { 946.0f, 114.91f }, { 946.0f, 342.91f }, { 972.0f, 342.91f } } },
        { "A1", "F6", { { 501.0f, 133.91f }, { 501.0f, 114.91f }, { 946.0f, 114.91f }, { 946.0f, 486.91f }, { 972.0f, 486.91f } } },
        { "A6", "A1", { { 369.0f, 163.91f }, { 432.0f, 163.91f } } },
        { "A2", "A1", { { 471.0f, 351.91f }, { 471.0f, 193.91f } } },
        { "A3", "A4", { { 603.0f, 352.91f }, { 603.0f, 284.91f } } },
        { "A4", "F1", { { 651.0f, 254.91f }, { 727.0f, 254.91f }, { 727.0f, 386.91f } } },
        { "A5", "A6", { { 241.5f, 333.91f }, { 241.5f, 193.91f } } },
        { "A5", "A7", { { 327.0f, 333.91f }, { 327.0f, 294.91f } } },
        { "A7", "A4", { { 387.0f, 254.91f }, { 554.0f, 254.91f } } },
        { "D1", "A5", { { 227.8f, 653.91f }, { 227.8f, 432.0f } } },
        { "D1", "D2", { { 302.0f, 653.91f }, { 302.0f, 610.0f } } },
        { "D1", "F6", { { 421.0f, 653.91f }, { 421.0f, 640.0f }, { 1070.0f, 640.0f }, { 1070.0f, 526.91f } } },
        { "D2", "A5", { { 326.5f, 504.91f }, { 326.5f, 432.0f } } },
        { "D2", "F6", { { 376.5f, 610.0f }, { 1008.75f, 610.0f }, { 1008.75f, 526.91f } } },
        { "D3", "D2", { { 443.0f, 557.46f }, { 376.5f, 557.46f } } },
        { "D3", "A2", { { 470.25f, 528.91f }, { 470.25f, 434.0f } } },
        { "D3", "A3", { { 552.0f, 543.18f }, { 603.0f, 543.18f }, { 603.0f, 431.91f } } },
        { "F2", "F1", { { 784.5f, 350.91f }, { 784.5f, 386.91f } } },
        { "F3", "A1", { { 881.5f, 231.91f }, { 881.5f, 148.91f }, { 570.0f, 148.91f } } },
        { "F4", "A6", { { 1035.0f, 165.91f }, { 1035.0f, 103.91f }, { 284.0f, 103.91f }, { 284.0f, 133.91f } } },
        { "F1", "A2", { { 727.0f, 506.16f }, { 501.0f, 506.16f }, { 501.0f, 434.0f } } },
        { "F1", "A3", { { 727.0f, 426.66f }, { 695.0f, 426.66f }, { 695.0f, 392.41f }, { 663.0f, 392.41f } } },
        { "F1", "F3", { { 880.75f, 386.91f }, { 880.75f, 350.91f } } },
        { "F1", "F4", { { 932.0f, 386.91f }, { 952.0f, 386.91f }, { 952.0f, 220.91f }, { 972.0f, 220.91f } } },
        { "F1", "F5", { { 932.0f, 426.66f }, { 1043.5f, 426.66f }, { 1043.5f, 399.91f } } },
        { "F1", "F6", { { 932.0f, 505.07f }, { 972.0f, 505.07f } } },
        { "F1", "D1", { { 880.75f, 545.91f }, { 880.75f, 668.91f }, { 491.0f, 668.91f } } },
        { "F1", "D2", { { 789.0f, 545.91f }, { 789.0f, 629.0f }, { 351.5f, 629.0f }, { 351.5f, 610.0f } } },
        { "F1", "D3", { { 778.25f, 545.91f }, { 778.25f, 557.0f }, { 552.0f, 557.0f } } },
    };
    return routes;
}

//------------------------------------------------------------------------------
//! \brief Does the segment from p to q (horizontal or vertical) cross the
//! rectangle?
static bool crosses(ImVec2 const& p, ImVec2 const& q, GemmaBox const& r)
{
    float const x0 = std::min(p.x, q.x), x1 = std::max(p.x, q.x);
    float const y0 = std::min(p.y, q.y), y1 = std::max(p.y, q.y);
    return (x1 > r.x + 1.0f) && (x0 < r.x + r.w - 1.0f) && (y1 > r.y + 1.0f) && (y0 < r.y + r.h - 1.0f);
}

//------------------------------------------------------------------------------
//! \brief A path for a link the form does not draw, in the units of the
//! form: straight when the two rectangles face each other, else with one or
//! two bends (L or Z), the one crossing the fewest other procedures kept.
//! Moved aside by shift (the other way of the same link).
static std::vector<ImVec2> routeLink(Gemma const& gemma, GemmaBox const& a, GemmaBox const& b, float shift)
{
    ImVec2 const ca(a.x + a.w * 0.5f + shift, a.y + a.h * 0.5f + shift);
    ImVec2 const cb(b.x + b.w * 0.5f + shift, b.y + b.h * 0.5f + shift);
    bool const right = (cb.x > ca.x);
    bool const down = (cb.y > ca.y);
    float const ax = right ? a.x + a.w : a.x;   // side of a toward b
    float const bx = right ? b.x : b.x + b.w;   // side of b toward a
    float const ay = down ? a.y + a.h : a.y;
    float const by = down ? b.y : b.y + b.h;
    std::vector<std::vector<ImVec2>> candidates;
    float const ox = std::max(a.x, b.x), ow = std::min(a.x + a.w, b.x + b.w) - ox;
    float const oy = std::max(a.y, b.y), oh = std::min(a.y + a.h, b.y + b.h) - oy;
    if (ow > 10.0f)
        candidates.push_back({ { ox + ow * 0.5f + shift, ay }, { ox + ow * 0.5f + shift, by } });
    if (oh > 10.0f)
        candidates.push_back({ { ax, oy + oh * 0.5f + shift }, { bx, oy + oh * 0.5f + shift } });
    candidates.push_back({ { ax, ca.y }, { cb.x, ca.y }, { cb.x, by } });                   // L: across, then up or down
    candidates.push_back({ { ca.x, ay }, { ca.x, cb.y }, { bx, cb.y } });                   // L: up or down, then across
    float const mx = (ax + bx) * 0.5f;
    candidates.push_back({ { ax, ca.y }, { mx, ca.y }, { mx, cb.y }, { bx, cb.y } });       // Z
    float const my = (ay + by) * 0.5f;
    candidates.push_back({ { ca.x, ay }, { ca.x, my }, { cb.x, my }, { cb.x, by } });       // Z
    auto const cost = [&](std::vector<ImVec2> const& path) {
        int crossed = 0;
        for (GemmaBox const& box : GEMMA_BOXES)
        {
            if ((box.code == a.code) || (box.code == b.code) || (gemma.find(box.code) == nullptr))
                continue;
            for (size_t i = 1u; i < path.size(); ++i)
                crossed += crosses(path[i - 1u], path[i], box) ? 1 : 0;
        }
        return crossed * 10 + int(path.size());
    };
    return *std::min_element(candidates.begin(), candidates.end(),
                             [&](auto const& p, auto const& q) { return cost(p) < cost(q); });
}

//------------------------------------------------------------------------------
//! \brief A path with an arrow head at its end.
static void arrowPath(ImDrawList* draw, std::vector<ImVec2> const& path, ImU32 color, float thickness, float head)
{
    if (path.size() < 2u)
        return;
    ImVec2 const a = path[path.size() - 2u];
    ImVec2 const b = path.back();
    ImVec2 const d = b - a;
    float const length = std::sqrt(d.x * d.x + d.y * d.y);
    if (length < 0.5f)
        return;
    ImVec2 const u = d / length;
    ImVec2 const n(-u.y, u.x);
    std::vector<ImVec2> line = path;
    line.back() = b - u * std::min(head * 0.8f, length);
    draw->AddPolyline(line.data(), int(line.size()), color, ImDrawFlags_None, thickness);
    draw->AddTriangleFilled(b, b - u * head + n * head * 0.4f, b - u * head - n * head * 0.4f, color);
}

//------------------------------------------------------------------------------
//! \brief Where to write the condition of a path, in screen units: in the
//! middle of its longest segment, else of another one, away from the
//! conditions already written and from the rectangles.
static ImVec2 labelPlace(std::vector<ImVec2> const& path, ImVec2 const& dim, std::vector<ImRect>& taken,
                         std::vector<ImRect> const& boxes)
{
    std::vector<size_t> segments;
    for (size_t i = 1u; i < path.size(); ++i)
        segments.push_back(i);
    auto const length = [&path](size_t i) {
        ImVec2 const d = path[i] - path[i - 1u];
        return std::fabs(d.x) + std::fabs(d.y);
    };
    std::stable_sort(segments.begin(), segments.end(), [&](size_t a, size_t b) { return length(a) > length(b); });
    auto const free = [&](ImRect const& r, bool with_boxes) {
        for (ImRect const& t : taken)
            if (t.Overlaps(r))
                return false;
        if (with_boxes)
        {
            for (ImRect const& b : boxes)
                if (b.Overlaps(r))
                    return false;
        }
        return true;
    };
    ImRect best;
    bool found = false;
    for (bool const with_boxes : { true, false })
    {
        for (size_t const i : segments)
        {
            for (float const t : { 0.5f, 0.3f, 0.7f, 0.15f, 0.85f })
            {
                ImVec2 const c = path[i - 1u] + (path[i] - path[i - 1u]) * t;
                ImRect const r(c - dim * 0.5f - ImVec2(3, 1), c + dim * 0.5f + ImVec2(3, 1));
                if (free(r, with_boxes))
                {
                    best = r;
                    found = true;
                    break;
                }
            }
            if (found)
                break;
        }
        if (found)
            break;
    }
    if (!found)
    {
        ImVec2 const c = path[segments[0] - 1u] + (path[segments[0]] - path[segments[0] - 1u]) * 0.5f;
        best = ImRect(c - dim * 0.5f - ImVec2(3, 1), c + dim * 0.5f + ImVec2(3, 1));
    }
    taken.push_back(best);
    return best.Min + ImVec2(3, 1);
}

//------------------------------------------------------------------------------
//! \brief The GEMMA drawn as its form (doc/gemma/GEMMA.drawio), read only: the
//! zones A, F, D, the frame of the production, the procedures (kept: their
//! step Xn of Control), the links with their conditions. A click selects a
//! procedure.
static void gemmaGraph(Gemma const& gemma, std::vector<GemmaIssue> const& issues, std::string& selected,
                       float& zoom)
{
    // The form spans x 20 ... 1140, y 15 ... 830 of the drawio units.
    ImVec2 const form_origin(15.0f, 10.0f);
    ImVec2 const form_size(1130.0f, 825.0f);
    ImVec2 const room = ImGui::GetContentRegionAvail();
    float const fit = std::max(0.2f, std::min(room.x / form_size.x, room.y / form_size.y));
    ImGui::BeginChild("##gemma_graph", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None,
                      ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImVec2 const origin = ImGui::GetCursorScreenPos();
    // The wheel zooms around the mouse, the right or middle button drags the
    // form (as on the nets).
    ImGuiIO const& io = ImGui::GetIO();
    if (ImGui::IsWindowHovered())
    {
        if (io.MouseWheel != 0.0f)
        {
            float const before = zoom;
            zoom = std::clamp(zoom * std::pow(1.15f, io.MouseWheel), 0.5f, 10.0f);
            ImVec2 const scroll = ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY()) +
                                  (io.MousePos - origin) * (zoom / before - 1.0f);
            ImGui::SetScrollX(scroll.x);
            ImGui::SetScrollY(scroll.y);
        }
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right) || ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
        {
            ImGui::SetScrollX(ImGui::GetScrollX() - io.MouseDelta.x);
            ImGui::SetScrollY(ImGui::GetScrollY() - io.MouseDelta.y);
        }
    }
    float const scale = fit * zoom;
    ImGui::InvisibleButton("##form", form_size * scale);
    bool const clicked = ImGui::IsItemClicked();
    bool const hovered = ImGui::IsItemHovered();
    ImVec2 const mouse = io.MousePos;
    ImDrawList* draw = ImGui::GetWindowDrawList();
    auto const at = [&](float x, float y) { return origin + (ImVec2(x, y) - form_origin) * scale; };
    auto const screen = [&](std::vector<ImVec2> path) {
        for (auto& p : path)
            p = at(p.x, p.y);
        return path;
    };
    ImFont* font = ImGui::GetFont();
    float const text_size = std::max(8.0f, 12.0f * scale);
    float const title_size = std::max(9.0f, 14.0f * scale);
    float const head = std::max(5.0f, 9.0f * scale);
    ImU32 const text = ImGui::GetColorU32(ImGuiCol_Text);
    ImU32 const disabled = ImGui::GetColorU32(ImGuiCol_TextDisabled);
    ImU32 const green = ImGui::ColorConvertFloat4ToU32(GEMMA_SELECTED);
    ImU32 const blue = ImGui::ColorConvertFloat4ToU32(GEMMA_ARRIVING);
    ImU32 const red = palette().critical;
    ImU32 const orange = IM_COL32(230, 150, 40, 255);
    bool const fr = (gemma.language == "fr");
    // Text centered in a width, from its top.
    auto const centered = [&](float x, float y, float w, float size, ImU32 color, std::string const& s) {
        ImVec2 const dim = font->CalcTextSizeA(size, FLT_MAX, w * scale, s.c_str());
        ImVec2 const p = at(x, y) + ImVec2((w * scale - dim.x) * 0.5f, 0.0f);
        draw->AddText(font, size, p, color, s.c_str(), nullptr, w * scale);
    };

    // The zones, their titles, and the power on and off of the control part.
    struct Zone { float x, y, w, h; ImU32 fill; };
    for (Zone const& z : { Zone{ 25.0f, 48.0f, 87.0f, 717.0f, IM_COL32(128, 128, 128, 35) },
                           Zone{ 176.0f, 45.0f, 491.0f, 417.0f, IM_COL32(70, 120, 210, 40) },
                           Zone{ 702.0f, 47.5f, 430.0f, 718.0f, IM_COL32(60, 180, 90, 35) },
                           Zone{ 176.0f, 476.5f, 481.0f, 289.0f, IM_COL32(220, 70, 60, 35) } })
    {
        draw->AddRectFilled(at(z.x, z.y), at(z.x + z.w, z.y + z.h), z.fill, 10.0f * scale);
        draw->AddRect(at(z.x, z.y), at(z.x + z.w, z.y + z.h), disabled, 10.0f * scale);
    }
    centered(33.0f, 59.0f, 69.0f, text_size, text, fr ? "P.C. hors énergie" : "Control part off");
    centered(280.0f, 52.0f, 283.0f, title_size, text, fr ? "(A) Procédures d'arrêt" : "(A) Stop procedures");
    centered(779.0f, 51.0f, 283.0f, title_size, text, fr ? "(F) Procédures de fonctionnement" : "(F) Operating procedures");
    centered(231.0f, 735.0f, 391.0f, title_size, text, fr ? "(D) Procédures en défaillance" : "(D) Failure procedures");
    std::string const power_on = fr ? "Mise en énergie de P.C." : "Power on of the control part";
    std::string const power_off = fr ? "Mise hors énergie de P.C." : "Power off of the control part";
    struct Power { std::vector<ImVec2> path; char const* label; ImVec2 where; };
    for (Power const& p : { Power{ { { 112.0f, 227.25f }, { 144.0f, 227.25f }, { 144.0f, 253.5f }, { 176.0f, 253.5f } }, "on", { 120.0f, 205.0f } },
                            Power{ { { 112.0f, 585.75f }, { 144.0f, 585.75f }, { 144.0f, 621.0f }, { 176.0f, 621.0f } }, "on", { 120.0f, 563.0f } },
                            Power{ { { 421.5f, 45.0f }, { 421.5f, 20.0f }, { 68.5f, 20.0f }, { 68.5f, 48.0f } }, "off", { 180.0f, 22.0f } },
                            Power{ { { 416.5f, 765.5f }, { 416.5f, 799.0f }, { 68.5f, 799.0f }, { 68.5f, 765.0f } }, "off", { 180.0f, 801.0f } } })
    {
        arrowPath(draw, screen(p.path), disabled, 1.0f, head);
        draw->AddText(font, text_size, at(p.where.x, p.where.y), disabled,
                      (p.label[1] == 'n') ? power_on.c_str() : power_off.c_str());
    }
    // The frame of the production.
    draw->AddRect(at(399.0f, 306.0f), at(1071.0f, 598.0f), orange, 10.0f * scale, 0, std::max(1.5f, 2.5f * scale));
    draw->AddText(font, title_size, at(654.0f, 572.0f), orange, "Production");

    // The procedures: kept ones filled, with their step in Control; the
    // others faint. A1 has the double frame of an initial step.
    std::map<std::string, GemmaBox const*> where;
    std::vector<ImRect> box_rects;
    std::map<std::string, size_t> const steps = gemmaStepNumbers(gemma);
    for (GemmaBox const& b : GEMMA_BOXES)
    {
        where[b.code] = &b;
        ImVec2 const lo = at(b.x, b.y);
        ImVec2 const hi = at(b.x + b.w, b.y + b.h);
        bool const kept = (gemma.find(b.code) != nullptr);
        bool const is_selected = (selected == b.code);
        bool const error = std::any_of(issues.begin(), issues.end(), [&b](GemmaIssue const& issue) {
            return issue.error && (issue.link < 0) && (issue.code == b.code);
        });
        if (kept)
        {
            draw->AddRectFilled(lo, hi, ImGui::GetColorU32(ImGuiCol_FrameBg, 1.0f));
            box_rects.push_back(ImRect(lo, hi));
        }
        ImU32 const border = error ? red : (is_selected ? green : (kept ? text : disabled));
        float const thickness = (is_selected || error) ? 3.0f : (kept ? 1.5f : 1.0f);
        draw->AddRect(lo, hi, border, 0.0f, 0, thickness);
        if (std::string(b.code) == "A1")
            draw->AddRect(at(b.x + 3.0f, b.y + 3.0f), at(b.x + b.w - 3.0f, b.y + b.h - 3.0f), border, 0.0f, 0, thickness);
        draw->PushClipRect(lo, hi, true);
        // The name where the form writes it, kept inside the rectangle.
        float const lw = std::min(b.lw, b.w - 6.0f);
        float const lx = std::clamp(b.lx + (b.lw - lw) * 0.5f, b.x + 3.0f, b.x + b.w - 3.0f - lw);
        centered(lx, b.ly, lw, text_size, kept ? text : disabled,
                 "(" + std::string(b.code) + ") " + gemmaName(b.code, gemma.language));
        auto const step = steps.find(b.code);
        if (step != steps.end())
        {
            std::string const x = "X" + std::to_string(step->second);
            ImVec2 const dim = font->CalcTextSizeA(text_size, FLT_MAX, 0.0f, x.c_str());
            ImVec2 const p = hi - dim - ImVec2(4.0f, 3.0f);
            draw->AddRectFilled(p - ImVec2(3, 1), p + dim + ImVec2(3, 1), IM_COL32(60, 120, 220, 230), 3.0f);
            draw->AddText(font, text_size, p, IM_COL32(255, 255, 255, 255), x.c_str());
        }
        draw->PopClipRect();

        bool const inside = (mouse.x >= lo.x) && (mouse.x <= hi.x) && (mouse.y >= lo.y) && (mouse.y <= hi.y);
        if (hovered && inside)
        {
            GemmaProcedure const* procedure = gemmaProcedure(b.code);
            ImGui::BeginTooltip();
            ImGui::Text("%s %s%s", b.code, gemmaName(b.code, gemma.language).c_str(),
                        kept ? "" : (fr ? " (non gardée)" : " (not kept)"));
            if (step != steps.end())
                ImGui::TextDisabled("Step X%zu of Control", step->second);
            ImGui::PushTextWrapPos(320.0f);
            ImGui::TextUnformatted(fr ? procedure->help_fr : procedure->help_en);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        if (clicked && inside)
            selected = is_selected ? std::string() : std::string(b.code);
    }

    // D1 is reached from every state by the emergency stop (Safety).
    if (gemma.find("D1") != nullptr)
    {
        draw->AddCircleFilled(at(614.5f, 791.0f), 8.0f * scale, text);
        arrowPath(draw, screen({ { 614.5f, 782.0f }, { 614.5f, 698.91f }, { 491.0f, 698.91f } }), red, 2.0f, head);
        draw->AddText(font, text_size, at(560.0f, 803.0f), text,
                      fr ? "Depuis tous les états : EStop (Safety)" : "From every state: EStop (Safety)");
    }

    // The links, over the procedures: the path of the form (or its way back
    // for a link the other way), else a path of their own.
    std::vector<ImRect> taken;
    std::vector<std::pair<std::vector<ImVec2>, size_t>> paths;
    std::set<std::pair<std::string, std::string>> drawn;
    for (size_t i = 0u; i < gemma.links.size(); ++i)
    {
        Gemma::Link const& link = gemma.links[i];
        if ((where.count(link.from) == 0u) || (where.count(link.to) == 0u) ||
            (gemma.find(link.from) == nullptr) || (gemma.find(link.to) == nullptr) || (link.from == link.to))
            continue;
        bool const both = std::any_of(gemma.links.begin(), gemma.links.end(), [&](Gemma::Link const& l) {
            return (l.from == link.to) && (l.to == link.from);
        });
        std::vector<ImVec2> path;
        for (auto const& route : gemmaRoutes())
        {
            if ((link.from == route.from) && (link.to == route.to))
                path = route.path;
        }
        if (path.empty() && !both)
        {
            for (auto const& route : gemmaRoutes())
            {
                if ((link.from == route.to) && (link.to == route.from))
                    path.assign(route.path.rbegin(), route.path.rend());
            }
        }
        if (path.empty())
            path = routeLink(gemma, *where[link.from], *where[link.to],
                             (both && drawn.count({ link.to, link.from })) ? 10.0f : 0.0f);
        drawn.insert({ link.from, link.to });
        paths.push_back({ screen(path), i });
    }
    // The grey ones first, the selected and the wrong ones over them.
    auto const color_of = [&](size_t i, bool& strong) {
        Gemma::Link const& link = gemma.links[i];
        bool const error = std::any_of(issues.begin(), issues.end(), [i](GemmaIssue const& issue) {
            return issue.error && (issue.link == int(i));
        });
        bool const leaving = (link.from == selected);
        bool const arriving = (link.to == selected);
        strong = error || leaving || arriving;
        return error ? red : (leaving ? green : (arriving ? blue : (selected.empty() ? text : disabled)));
    };
    for (int pass = 0; pass < 2; ++pass)
    {
        for (auto const& [path, i] : paths)
        {
            bool strong = false;
            ImU32 const color = color_of(i, strong);
            if (strong != (pass == 1))
                continue;
            arrowPath(draw, path, color, strong ? 2.5f : 1.5f, head);
        }
    }
    for (int pass = 0; pass < 2; ++pass)
    {
        for (auto const& [path, i] : paths)
        {
            bool strong = false;
            ImU32 const color = color_of(i, strong);
            if (strong != (pass == 0))
                continue;
            std::string const& condition = gemma.links[i].condition;
            std::string const label = condition.empty() ? std::string("?") : condition;
            ImVec2 const dim = font->CalcTextSizeA(text_size, FLT_MAX, 0.0f, label.c_str());
            ImVec2 const p = labelPlace(path, dim, taken, box_rects);
            draw->AddRectFilled(p - ImVec2(3, 1), p + dim + ImVec2(3, 1), ImGui::GetColorU32(ImGuiCol_WindowBg, 0.9f), 3.0f);
            draw->AddRect(p - ImVec2(3, 1), p + dim + ImVec2(3, 1), color, 3.0f);
            draw->AddText(font, text_size, p, color, label.c_str());
        }
    }
    ImGui::EndChild();
}

//------------------------------------------------------------------------------
//! \brief Begin a GEMMA window: docked by Editor::dockGemmaPanes, its editor
//! on the left and its graph between it and the nets.
static bool beginGemmaWindow(char const* name, bool* open, ImVec2 const& size)
{
    ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);
    return ImGui::Begin(name, open);
}

//------------------------------------------------------------------------------
//! \brief What a GEMMA window says when the document has none.
static bool noGemma(Document& doc)
{
    if (doc.info().gemma)
        return false;
    ImGui::TextWrapped("This document has no GEMMA. A GEMMA lists the run and stop modes the "
                       "machine keeps among the 16 of the guide, and the links between them; the "
                       "editor makes from it the GRAFCETs Safety, Control and Production.");
    if (ImGui::Button("Start a GEMMA"))
    {
        doc.info().gemma = defaultGemma();
        doc.setModified(true);
    }
    return true;
}

//------------------------------------------------------------------------------
//! \brief The colors of the links of the procedure selected, said in words.
static void selectionLegend(std::string const& selected)
{
    if (selected.empty())
    {
        ImGui::TextDisabled("Click a procedure to see its links.");
        return;
    }
    ImGui::TextColored(GEMMA_SELECTED, "Green: leaving %s", selected.c_str());
    ImGui::SameLine();
    ImGui::TextColored(GEMMA_ARRIVING, "  Blue: arriving at %s", selected.c_str());
}

//------------------------------------------------------------------------------
//! \brief A window is docked, or will be by its saved settings.
static bool docked(char const* name)
{
    ImGuiID id = 0u;
    if (ImGuiWindow const* window = ImGui::FindWindowByName(name))
        id = window->DockId;
    else if (ImGuiWindowSettings const* settings = ImGui::FindWindowSettingsByID(ImHashStr(name)))
        id = settings->DockId;
    return (id != 0u) && (ImGui::DockBuilderGetNode(id) != nullptr);
}

//------------------------------------------------------------------------------
void Editor::dockGemmaPanes(unsigned int const dockspace)
{
    bool const editor = m_states.show_gemma && !docked("GEMMA editor");
    bool const graph = m_states.show_gemma_graph && !docked("GEMMA");
    ImGuiDockNode const* central = ImGui::DockBuilderGetCentralNode(dockspace);
    if ((!editor && !graph) || (central == nullptr))
        return;
    // Cut from the nets: the panes of the user stay as they are. The editor
    // on the left, all the height of the nets and of the console under them.
    if (editor)
    {
        ImGuiDockNode const* column = central;
        while ((column->ParentNode != nullptr) && (column->ParentNode->SplitAxis == ImGuiAxis_Y))
            column = column->ParentNode;
        ImGui::DockBuilderDockWindow("GEMMA editor",
            ImGui::DockBuilderSplitNode(column->ID, ImGuiDir_Left, 0.2f, nullptr, nullptr));
        central = ImGui::DockBuilderGetCentralNode(dockspace);
    }
    // The graph on the left of the nets (their tabs).
    if (graph && (central != nullptr))
        ImGui::DockBuilderDockWindow("GEMMA",
            ImGui::DockBuilderSplitNode(central->ID, ImGuiDir_Left, 0.45f, nullptr, nullptr));
    ImGui::DockBuilderFinish(dockspace);
    for (auto const& entry : activeDocument().nets())
        entry->view_state.center_request = Document::ViewState::CENTER_FRAMES;
    m_busy_frames = BUSY_FRAMES;
}

//------------------------------------------------------------------------------
void Editor::gemmaGraphWindow()
{
    if (!beginGemmaWindow("GEMMA", &m_states.show_gemma_graph, ImVec2(1150.0f, 880.0f)))
    {
        ImGui::End();
        return;
    }
    Document& doc = activeDocument();
    if (!noGemma(doc))
    {
        Gemma const& gemma = *doc.info().gemma;
        std::vector<GemmaIssue> const issues = checkGemma(gemma);
        if (ImGui::SmallButton("Fit"))
            m_states.gemma_zoom = 1.0f;
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Wheel: zoom. Right or middle drag: move the form.");
        ImGui::SameLine();
        ImGui::TextDisabled("%d %%", int(std::lround(m_states.gemma_zoom * 100.0f)));
        ImGui::SameLine();
        selectionLegend(m_states.gemma_selected);
        gemmaGraph(gemma, issues, m_states.gemma_selected, m_states.gemma_zoom);
    }
    ImGui::End();
}

//------------------------------------------------------------------------------
void Editor::gemmaWindow()
{
    if (!beginGemmaWindow("GEMMA editor", &m_states.show_gemma, ImVec2(1000.0f, 700.0f)))
    {
        ImGui::End();
        return;
    }

    Document& doc = activeDocument();
    if (noGemma(doc))
    {
        ImGui::End();
        return;
    }

    Gemma& gemma = *doc.info().gemma;
    bool modified = false;
    std::vector<GemmaIssue> const issues = checkGemma(gemma);
    size_t const errors = size_t(std::count_if(issues.begin(), issues.end(),
                                               [](GemmaIssue const& i) { return i.error; }));

    // Tool bar: language, make the GRAFCETs, state of the checks.
    bool const fr = (gemma.language == "fr");
    if (ImGui::Button(fr ? "Names: Français" : "Names: English"))
    {
        gemma.language = fr ? "en" : "fr";
        modified = true;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Names of the procedures, in the window and in the GRAFCETs");
    ImGui::SameLine();
    ImGui::BeginDisabled(errors > 0u);
    if (ImGui::Button("Make the GRAFCETs"))
        generateFromGemma();
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("Make again Safety and Control; Production is kept (made when missing).%s",
                          (errors > 0u) ? "\nFix the errors first." : "");
    ImGui::SameLine();
    if (issues.empty())
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.4f, 1.0f), "No problem found");
    else
        ImGui::TextColored(issueColor(issues, "", -1), "%zu error(s), %zu warning(s)", errors,
                           issues.size() - errors);

    ImGui::SameLine();
    if (ImGui::Button(m_states.show_gemma_graph ? "Hide the graph" : "Show the graph"))
        m_states.show_gemma_graph ^= true;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("The GEMMA drawn as its form, above the GRAFCETs");

    // Left: the procedures by family, and the one selected. In a narrow pane
    // (docked beside the nets), above the links instead.
    ImVec2 const avail = ImGui::GetContentRegionAvail();
    bool const wide = (avail.x > 760.0f);
    // Above: a share of the height of the pane, followed when it changes (a
    // size of the user would be the one of the first frame, the pane not
    // sized yet). Beside: resizable, kept between a fifth and most of it.
    if (wide)
    {
        ImGui::SetNextWindowSizeConstraints(ImVec2(avail.x * 0.2f, 0.0f), ImVec2(avail.x * 0.8f, FLT_MAX));
        ImGui::BeginChild("##procedures", ImVec2(avail.x * 0.45f, 0.0f), ImGuiChildFlags_ResizeX);
    }
    else
    {
        ImGui::BeginChild("##procedures", ImVec2(0.0f, avail.y * 0.55f));
    }
    // One section per family, one line per procedure: kept or not, its name.
    struct Family { char letter; char const* en; char const* fr; };
    for (Family const& family : { Family{ 'A', "A - Stop procedures", "A - Procédures d'arrêt" },
                                  Family{ 'F', "F - Operating procedures", "F - Procédures de fonctionnement" },
                                  Family{ 'D', "D - Failure procedures", "D - Procédures en défaillance" } })
    {
        ImGui::SeparatorText(fr ? family.fr : family.en);
        for (auto const& procedure : gemmaProcedures())
        {
            if (procedure.code[0] != family.letter)
                continue;
            std::string const code = procedure.code;
            ImGui::PushID(procedure.code);
            bool kept = (gemma.find(code) != nullptr);
            if (ImGui::Checkbox("##kept", &kept))
            {
                if (kept)
                    gemma.states.push_back({ code, procedure.production });
                else
                    gemma.states.erase(std::remove_if(gemma.states.begin(), gemma.states.end(),
                        [&code](Gemma::State const& st) { return st.code == code; }), gemma.states.end());
                modified = true;
            }
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, kept ? issueColor(issues, code, -1)
                                                      : ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            std::string const label = code + "  " + (fr ? procedure.fr : procedure.en);
            if (ImGui::Selectable(label.c_str(), m_states.gemma_selected == code))
                m_states.gemma_selected = (m_states.gemma_selected == code) ? std::string() : code;
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(320.0f);
                ImGui::TextUnformatted(fr ? procedure.help_fr : procedure.help_en);
                for (auto const& issue : issues)
                {
                    if ((issue.code == code) && (issue.link < 0))
                        ImGui::TextColored(issueColor(issues, code, -1), "%s", issue.message.c_str());
                }
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
            ImGui::PopID();
        }
    }

    // The procedure selected: what the production does there.
    ImGui::Spacing();
    GemmaProcedure const* selected = gemmaProcedure(m_states.gemma_selected);
    if (selected == nullptr)
    {
        ImGui::TextDisabled("Check the procedures the machine keeps; click one to see it and its links.");
    }
    else
    {
        ImGui::SeparatorText((std::string(selected->code) + " " + (fr ? selected->fr : selected->en)).c_str());
        ImGui::TextWrapped("%s", fr ? selected->help_fr : selected->help_en);
        selectionLegend(selected->code);
        if (Gemma::State* state = gemma.find(selected->code))
        {
            ImGui::TextUnformatted("While it is active, Production:");
            int production = int(state->production);
            modified |= ImGui::RadioButton("runs", &production, int(Gemma::Production::Run));
            ImGui::SameLine();
            modified |= ImGui::RadioButton("is in its initial situation {init}", &production,
                                           int(Gemma::Production::Init));
            ImGui::SameLine();
            modified |= ImGui::RadioButton("is frozen {*}", &production, int(Gemma::Production::Freeze));
            state->production = Gemma::Production(production);
        }
        else
        {
            ImGui::TextDisabled("Not kept.");
        }
        for (auto const& issue : issues)
        {
            if (issue.code == selected->code)
                ImGui::TextColored(issueColor(issues, issue.code, issue.link), "%s", issue.message.c_str());
        }
    }
    ImGui::EndChild();
    if (wide)
        ImGui::SameLine();

    // Right (or below): the links between procedures kept.
    ImGui::BeginChild("##links");
    ImGui::SeparatorText("Links (transitions of Control)");
    int remove = -1;
    if (ImGui::BeginTable("##links_table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                              ImGuiTableFlags_SizingFixedFit))
    {
        ImGui::TableSetupColumn("From");
        ImGui::TableSetupColumn("To");
        ImGui::TableSetupColumn("Condition", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("");
        ImGui::TableHeadersRow();
        for (size_t i = 0u; i < gemma.links.size(); ++i)
        {
            Gemma::Link& link = gemma.links[i];
            ImGui::PushID(int(i));
            ImGui::TableNextRow();
            // The links leaving the procedure selected in green on a green
            // row, the ones arriving in blue on a blue row, the others grey
            // (red: an error, whatever the selection).
            bool const leaving = (link.from == m_states.gemma_selected);
            bool const arriving = (link.to == m_states.gemma_selected);
            bool const touched = leaving || arriving;
            ImVec4 color = issueColor(issues, link.from, int(i));
            bool const error = std::any_of(issues.begin(), issues.end(), [i](GemmaIssue const& issue) {
                return issue.error && (issue.link == int(i));
            });
            if (!m_states.gemma_selected.empty() && !error)
                color = leaving ? GEMMA_SELECTED : (arriving ? GEMMA_ARRIVING
                                                             : ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            if (!m_states.gemma_selected.empty() && touched)
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, leaving ? IM_COL32(60, 190, 90, 45)
                                                                          : IM_COL32(80, 150, 255, 45));
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(color, "%s", link.from.c_str());
            ImGui::TableNextColumn();
            ImGui::TextColored(color, "%s", link.to.c_str());
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            modified |= ImGui::InputTextWithHint("##condition", "receptivity: Start . /Fault", &link.condition);
            for (auto const& issue : issues)
            {
                if ((issue.link == int(i)) && ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", issue.message.c_str());
            }
            ImGui::TableNextColumn();
            if (ImGui::SmallButton("x"))
                remove = int(i);
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    if (remove >= 0)
    {
        gemma.links.erase(gemma.links.begin() + remove);
        modified = true;
    }

    // The links of the guide between procedures kept, one click to add.
    bool header = false;
    for (auto const& standard : gemmaStandardLinks())
    {
        if ((gemma.find(standard.from) == nullptr) || (gemma.find(standard.to) == nullptr))
            continue;
        bool const there = std::any_of(gemma.links.begin(), gemma.links.end(), [&](Gemma::Link const& l) {
            return (l.from == standard.from) && (l.to == standard.to);
        });
        if (there)
            continue;
        if (!header)
        {
            ImGui::SeparatorText("Links of the guide");
            header = true;
        }
        std::string const label = "+ " + standard.from + " -> " + standard.to + "  (" + standard.condition + ")";
        if (ImGui::SmallButton(label.c_str()))
        {
            gemma.links.push_back(standard);
            modified = true;
        }
    }

    // Another link, chosen in two lists.
    ImGui::SeparatorText("Another link");
    std::vector<std::string> codes;
    for (auto const& procedure : gemmaProcedures())
    {
        if (gemma.find(procedure.code) != nullptr)
            codes.push_back(procedure.code);
    }
    auto const combo = [&codes](char const* label, std::string& value) {
        ImGui::SetNextItemWidth(70.0f);
        if (ImGui::BeginCombo(label, value.c_str()))
        {
            for (auto const& code : codes)
            {
                if (ImGui::Selectable(code.c_str(), value == code))
                    value = code;
            }
            ImGui::EndCombo();
        }
    };
    combo("##from", m_states.gemma_from);
    ImGui::SameLine();
    ImGui::TextUnformatted("->");
    ImGui::SameLine();
    combo("##to", m_states.gemma_to);
    ImGui::SameLine();
    ImGui::BeginDisabled(m_states.gemma_from.empty() || m_states.gemma_to.empty() ||
                         (m_states.gemma_from == m_states.gemma_to));
    if (ImGui::Button("Add"))
    {
        gemma.links.push_back({ m_states.gemma_from, m_states.gemma_to, "" });
        modified = true;
    }
    ImGui::EndDisabled();

    // All the problems.
    if (!issues.empty())
    {
        ImGui::SeparatorText("Checks");
        for (auto const& issue : issues)
            ImGui::TextColored(issueColor(issues, issue.code, issue.link), "%s %s",
                               issue.error ? "Error:" : "Warning:", issue.message.c_str());
    }
    ImGui::EndChild();

    if (modified)
        doc.setModified(true);
    ImGui::End();
}

} // namespace tpne
