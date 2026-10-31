// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// GRML (CosyVerif, written by GreatSPN and read by Symmetri): nodes "place"
// and "transition" with nested attributes (name, marking, priority,
// distribution), arcs with their valuation (weight); inhibitor arcs have the
// type "inhibitorarc". A transition with a DETERMINISTIC distribution gives
// its delay to the durations of its output arcs (a timed net).

#include "IO/Imports/Imports.hpp"
#include "PetriNet/PetriNet.hpp"
// A third-party header: its warnings are not ours.
#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
#include "tinyxml2.h"
#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic pop
#endif

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>

namespace tpne {

using tinyxml2::XMLElement;

//------------------------------------------------------------------------------
//! \brief The <attribute name="..."> child of the element, or null.
static XMLElement const* attribute(XMLElement const* element, char const* name)
{
    if (element == nullptr)
        return nullptr;
    for (XMLElement const* a = element->FirstChildElement("attribute"); a != nullptr;
         a = a->NextSiblingElement("attribute"))
    {
        char const* n = a->Attribute("name");
        if ((n != nullptr) && (std::strcmp(n, name) == 0))
            return a;
    }
    return nullptr;
}

//------------------------------------------------------------------------------
//! \brief Text of the attribute, or of the first value nested in it
//! (<attribute name="expr"><attribute name="intValue">2</attribute>).
static std::string value(XMLElement const* element)
{
    while (element != nullptr)
    {
        if ((element->GetText() != nullptr) && (element->FirstChildElement() == nullptr))
            return element->GetText();
        element = element->FirstChildElement("attribute");
    }
    return {};
}

//------------------------------------------------------------------------------
std::string importFromGRML(Net& net, std::string const& filename)
{
    std::stringstream error;
    std::ifstream file(filename);
    if (!file)
    {
        error << "Failed opening '" << filename << "'. Reason: '" << strerror(errno) << "'" << std::endl;
        return error.str();
    }
    tinyxml2::XMLDocument xml;
    if (xml.LoadFile(filename.c_str()) != tinyxml2::XML_SUCCESS)
    {
        error << "Failed parsing '" << filename << "'. Reason: '" << xml.ErrorStr() << "'" << std::endl;
        return error.str();
    }
    XMLElement const* model = xml.FirstChildElement("model");
    if (model == nullptr)
    {
        error << "Failed parsing '" << filename << "'. Reason: 'No <model> element'" << std::endl;
        return error.str();
    }

    net.reset(TypeOfNet::PetriNet);
    std::map<std::string, NodeId> ids;
    std::map<size_t, float> delays;
    for (XMLElement const* node = model->FirstChildElement("node"); node != nullptr;
         node = node->NextSiblingElement("node"))
    {
        char const* id = node->Attribute("id");
        char const* type = node->Attribute("nodeType");
        if ((id == nullptr) || (type == nullptr))
            continue;
        std::string name = value(attribute(node, "name"));
        if (name.empty())
            name = id;
        Point const at = { node->FloatAttribute("x"), node->FloatAttribute("y") };
        if (std::strcmp(type, "place") == 0)
        {
            std::string const marking = value(attribute(node, "marking"));
            size_t const tokens = marking.empty() ? 0u : static_cast<size_t>(std::strtoull(marking.c_str(), nullptr, 10));
            Place& place = net.addPlace(net.places().size(), name, tokens);
            net.position(place) = at;
            ids[id] = place.nodeId();
        }
        else if (std::strcmp(type, "transition") == 0)
        {
            Transition& transition = net.addTransition(net.transitions().size(), name);
            net.position(transition) = at;
            std::string const priority = value(attribute(node, "priority"));
            if (!priority.empty())
                transition.priority = std::atoi(priority.c_str());
            XMLElement const* distribution = attribute(node, "distribution");
            if (value(attribute(distribution, "type")) == "DETERMINISTIC")
                delays[transition.id] = std::strtof(value(attribute(distribution, "param")).c_str(), nullptr);
            ids[id] = transition.nodeId();
        }
    }

    for (XMLElement const* arc = model->FirstChildElement("arc"); arc != nullptr;
         arc = arc->NextSiblingElement("arc"))
    {
        char const* source = arc->Attribute("source");
        char const* target = arc->Attribute("target");
        auto const from = ids.find((source != nullptr) ? source : "");
        auto const to = ids.find((target != nullptr) ? target : "");
        if ((from == ids.end()) || (to == ids.end()) || (from->second.type == to->second.type))
        {
            error << "Failed parsing '" << filename << "'. Reason: 'Arc " << (source ? source : "?")
                  << " -> " << (target ? target : "?") << " does not link a place and a transition'" << std::endl;
            return error.str();
        }
        Node& a = net.node(from->second);
        Node& b = net.node(to->second);
        std::string const msg = net.addArc(a, b);
        if (!msg.empty())
        {
            error << "Failed parsing '" << filename << "'. Reason: '" << msg << "'" << std::endl;
            return error.str();
        }
        Arc* created = net.findArc(a, b);
        std::string const valuation = value(attribute(arc, "valuation"));
        if (!valuation.empty())
            created->weight = std::max(size_t(1u), static_cast<size_t>(std::strtoull(valuation.c_str(), nullptr, 10)));
        char const* kind = arc->Attribute("arcType");
        if ((kind != nullptr) && (std::strcmp(kind, "inhibitorarc") == 0) && created->from.isPlace())
            created->kind = ArcKind::Inhibitor;
        if (char const* color = arc->Attribute("color"))
            created->color = net.colors.add(color);
        if (created->from.isTransition())
            created->duration.reset();
    }

    // Deterministic delays: durations of the output arcs of a timed net.
    if (!delays.empty())
    {
        std::vector<size_t> arcs;
        std::string msg;
        convertTo(net, TypeOfNet::TimedPetriNet, msg, arcs);
        for (auto const& it : delays)
        {
            for (Arc& a : net.arcsOut(net.transitions()[it.first]))
                a.duration = it.second;
        }
    }
    return {};
}

} // namespace tpne
