// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// Petri Net Markup Language: see ExportPNML.cpp. Reads the standard files
// (GreatSPN, PIPE, ...), the ones of Symmetri (one <initialMarking> per
// token, colors of the arcs) and the <toolspecific> elements of this editor.
// The files written by the old versions of this editor (the duration in the
// inscription of the arcs) are recognized by the id of their net.

#include "IO/Imports/Imports.hpp"
#include "PetriNet/PetriNet.hpp"
#include "tinyxml2/tinyxml2.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace tpne {

using tinyxml2::XMLElement;

//------------------------------------------------------------------------------
//! \brief Text of the <text> child of the element, or empty.
static std::string textOf(XMLElement const* element)
{
    if (element == nullptr)
        return {};
    XMLElement const* text = element->FirstChildElement("text");
    return ((text != nullptr) && (text->GetText() != nullptr)) ? text->GetText() : "";
}

//------------------------------------------------------------------------------
//! \brief Text of the child of the <toolspecific> element of this editor.
static std::string toolValue(XMLElement const* element, char const* name)
{
    for (XMLElement const* tool = element->FirstChildElement("toolspecific"); tool != nullptr;
         tool = tool->NextSiblingElement("toolspecific"))
    {
        char const* id = tool->Attribute("tool");
        if ((id == nullptr) || (std::strcmp(id, "TimedPetriNetEditor") != 0))
            continue;
        XMLElement const* value = tool->FirstChildElement(name);
        if ((value != nullptr) && (value->GetText() != nullptr))
            return value->GetText();
    }
    return {};
}

//------------------------------------------------------------------------------
//! \brief The unsigned number of the text (0 if none).
static size_t number(std::string const& text)
{
    return static_cast<size_t>(std::strtoull(text.c_str(), nullptr, 10));
}

//------------------------------------------------------------------------------
static Point positionOf(XMLElement const* element)
{
    Point p;
    XMLElement const* graphics = element->FirstChildElement("graphics");
    XMLElement const* position = (graphics != nullptr) ? graphics->FirstChildElement("position") : nullptr;
    if (position != nullptr)
    {
        p.x = position->FloatAttribute("x");
        p.y = position->FloatAttribute("y");
    }
    return p;
}

//------------------------------------------------------------------------------
//! \brief The pages of the net, and the pages they hold.
static void pages(XMLElement const* parent, std::vector<XMLElement const*>& found)
{
    for (XMLElement const* page = parent->FirstChildElement("page"); page != nullptr;
         page = page->NextSiblingElement("page"))
    {
        found.push_back(page);
        pages(page, found);
    }
}

//------------------------------------------------------------------------------
std::string importFromPNML(Net& net, std::string const& filename)
{
    std::stringstream error;
    std::ifstream file(filename);
    if (!file)
    {
        error << "Failed opening '" << filename << "'. Reason: '"
              << strerror(errno) << "'" << std::endl;
        return error.str();
    }

    tinyxml2::XMLDocument xml;
    if (xml.LoadFile(filename.c_str()) != tinyxml2::XML_SUCCESS)
    {
        error << "Failed parsing '" << filename << "'. Reason: '" << xml.ErrorStr() << "'" << std::endl;
        return error.str();
    }
    XMLElement const* root = xml.FirstChildElement("pnml");
    XMLElement const* jnet = (root != nullptr) ? root->FirstChildElement("net") : nullptr;
    if (jnet == nullptr)
    {
        error << "Failed parsing '" << filename << "'. Reason: 'No <pnml><net> element'" << std::endl;
        return error.str();
    }

    // Type of net: given by this editor, else timed when there are durations.
    std::string const type = toolValue(jnet, "type");
    char const* old_id = jnet->Attribute("id");
    bool const legacy = type.empty() && (old_id != nullptr) &&
        ((std::strcmp(old_id, "Petri net") == 0) || (std::strcmp(old_id, "Timed Petri net") == 0) ||
         (std::strcmp(old_id, "Timed event graph") == 0) || (std::strcmp(old_id, "GRAFCET") == 0));
    std::string const kind = legacy ? old_id : type;
    net.reset((kind == "GRAFCET") ? TypeOfNet::GRAFCET
              : ((kind == "Timed Petri net") || (kind == "Timed event graph")) ? TypeOfNet::TimedPetriNet
              : TypeOfNet::PetriNet);
    std::string const name = textOf(jnet->FirstChildElement("name"));
    net.name = name.empty() ? ((old_id != nullptr) && !legacy ? old_id : "") : name;

    std::vector<XMLElement const*> all;
    pages(jnet, all);
    if (all.empty())
        all.push_back(jnet); // nodes right in the net

    std::map<std::string, NodeId> ids;
    for (XMLElement const* page : all)
    {
        for (XMLElement const* child = page->FirstChildElement("place"); child != nullptr;
             child = child->NextSiblingElement("place"))
        {
            char const* id = child->Attribute("id");
            if (id == nullptr)
                continue;
            // Standard: one <initialMarking> with the number of tokens.
            // Symmetri: one per token (a color, no text).
            size_t tokens = 0u;
            for (XMLElement const* m = child->FirstChildElement("initialMarking"); m != nullptr;
                 m = m->NextSiblingElement("initialMarking"))
            {
                std::string const text = textOf(m);
                tokens += text.empty() ? 1u : number(text);
                if (char const* color = m->Attribute("color"))
                    net.colors.add(color);
            }
            std::string caption = textOf(child->FirstChildElement("name"));
            if (caption.empty())
                caption = id;
            Place& place = net.addPlace(net.places().size(), caption, tokens);
            net.position(place) = positionOf(child);
            std::string const capacity = toolValue(child, "capacity");
            if (!capacity.empty())
                place.capacity = std::max(size_t(1u), number(capacity));
            ids[id] = place.nodeId();
        }
        for (XMLElement const* child = page->FirstChildElement("transition"); child != nullptr;
             child = child->NextSiblingElement("transition"))
        {
            char const* id = child->Attribute("id");
            if (id == nullptr)
                continue;
            std::string caption = textOf(child->FirstChildElement("name"));
            if (caption.empty())
                caption = id;
            Transition& transition = net.addTransition(net.transitions().size(), caption);
            net.position(transition) = positionOf(child);
            std::string const priority = toolValue(child, "priority");
            if (!priority.empty())
                transition.priority = std::atoi(priority.c_str());
            std::string const delay = toolValue(child, "delay");
            if (!delay.empty())
                transition.delay = std::max(0.0f, std::strtof(delay.c_str(), nullptr));
            ids[id] = transition.nodeId();
        }
    }

    for (XMLElement const* page : all)
    {
        for (XMLElement const* child = page->FirstChildElement("arc"); child != nullptr;
             child = child->NextSiblingElement("arc"))
        {
            char const* source = child->Attribute("source");
            char const* target = child->Attribute("target");
            auto const from = ids.find((source != nullptr) ? source : "");
            auto const to = ids.find((target != nullptr) ? target : "");
            if ((from == ids.end()) || (to == ids.end()))
            {
                error << "Failed parsing '" << filename << "'. Reason: 'Arc "
                      << (source ? source : "?") << " -> " << (target ? target : "?")
                      << " refers to unknown nodes'" << std::endl;
                return error.str();
            }
            if (from->second.type == to->second.type)
            {
                error << "Failed parsing '" << filename << "'. Reason: 'Arc " << source << " -> "
                      << target << " links two nodes of the same kind'" << std::endl;
                return error.str();
            }

            // Old files of this editor: the inscription is the duration.
            std::string const inscription = textOf(child->FirstChildElement("inscription"));
            std::string duration = toolValue(child, "duration");
            size_t weight = 1u;
            if (legacy)
                duration = inscription;
            else if (!inscription.empty())
                weight = std::max(size_t(1u), number(inscription));

            Node& a = net.node(from->second);
            Node& b = net.node(to->second);
            float const d = duration.empty() ? 0.0f : std::max(0.0f, std::strtof(duration.c_str(), nullptr));
            std::string const msg = net.addArc(a, b, d);
            if (!msg.empty())
            {
                error << "Failed parsing '" << filename << "'. Reason: '" << msg << "'" << std::endl;
                return error.str();
            }
            Arc* arc = net.findArc(a, b);
            arc->weight = weight;
            if (arc->from.isTransition() && duration.empty())
                arc->duration.reset();
            std::string const kind_name = toolValue(child, "kind");
            if (arc->from.isPlace() && (kind_name == "inhibitor"))
                arc->kind = ArcKind::Inhibitor;
            else if (arc->from.isPlace() && (kind_name == "reset"))
                arc->kind = ArcKind::Reset;
            if (char const* color = child->Attribute("color"))
                arc->color = net.colors.add(color);
        }
    }

    if (toolValue(jnet, "compact") == "true" || (legacy && (kind == "Timed event graph")))
    {
        std::vector<size_t> arcs;
        std::string msg;
        convertToEventGraph(net, msg, arcs);
    }
    return {};
}

} // namespace tpne
