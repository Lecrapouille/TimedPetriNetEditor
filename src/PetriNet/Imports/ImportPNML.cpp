// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Imports.hpp"
#include "PetriNet/PetriNet.hpp"
#include "PetriNet/SafeFloat.hpp"
#include "tinyxml2/tinyxml2.h"
#include <sstream>
#include <fstream>
#include <string>
#include <cstring>
#include <map>

namespace tpne {

//------------------------------------------------------------------------------
std::string importFromPNML(Net& net, std::string const& filename)
{
    std::stringstream error;

    // Check if file exists
    std::ifstream file(filename);
    if (!file)
    {
        error << "Failed opening '" << filename << "'. Reason: '"
              << strerror(errno) << "'" << std::endl;
        return error.str();
    }

    tinyxml2::XMLDocument xml;
    xml.LoadFile(filename.c_str());

    tinyxml2::XMLElement *levelElement = xml.FirstChildElement("pnml")
                                         ->FirstChildElement("net")
                                         ->FirstChildElement("page");

    size_t place_id = 0u;
    size_t transition_id = 0u;
    std::map<std::string, std::string> lookup_ids;

    // loop places.
    for (tinyxml2::XMLElement *child = levelElement->FirstChildElement("place");
         child != NULL; child = child->NextSiblingElement("place"))
    {
        lookup_ids[child->Attribute("id")] = Place::to_str(place_id);
        auto tokens =
                (child->FirstChildElement("initialMarking") == nullptr)
                ? 0
                : std::stoi(child->FirstChildElement("initialMarking")
                            ->FirstChildElement("text")
                            ->GetText());
        auto caption = child->FirstChildElement("name")
                            ->FirstChildElement("text")
                            ->GetText();

        float x = 0.0f; float y = 0.0f;
        if (child->FirstChildElement("graphics") != nullptr)
        {
            x = std::stof(child->FirstChildElement("graphics")
                            ->FirstChildElement("position")
                            ->Attribute("x"));
            y = std::stof(child->FirstChildElement("graphics")
                            ->FirstChildElement("position")
                            ->Attribute("y"));
        }

        net.addPlace(place_id, caption, x, y, size_t(tokens));
        place_id++;
    }

    // loop transitions
    for (tinyxml2::XMLElement *child = levelElement->FirstChildElement("transition");
         child != NULL; child = child->NextSiblingElement("transition"))
    {
        lookup_ids[child->Attribute("id")] = Transition::to_str(transition_id);
        auto caption = child->FirstChildElement("name")
                            ->FirstChildElement("text")
                            ->GetText();

        float x = 0.0f; float y = 0.0f;
        if (child->FirstChildElement("graphics") != nullptr)
        {
            x = std::stof(child->FirstChildElement("graphics")
                            ->FirstChildElement("position")
                            ->Attribute("x"));
            y = std::stof(child->FirstChildElement("graphics")
                            ->FirstChildElement("position")
                            ->Attribute("y"));
        }
        net.addTransition(transition_id, caption, x, y);
        transition_id++;
    }

    // loop arcs
    for (tinyxml2::XMLElement *child = levelElement->FirstChildElement("arc");
         child != NULL; child = child->NextSiblingElement("arc"))
    {
        const auto source = child->Attribute("source");
        const auto target = child->Attribute("target");
        Node* from = net.findNode(lookup_ids[source]);
        Node* to = net.findNode(lookup_ids[target]);
        if ((from == nullptr) || (to == nullptr))
        {
            error << "Failed parsing '" << filename << "'. Reason: 'Arc "
                << source << " -> " << target << " refer to unknown nodes'"
                << std::endl;
            return error.str();
        }

        float duration = safeNaNF();
        if (child->FirstChildElement("inscription") != nullptr)
        {
            duration = std::stof(child->FirstChildElement("inscription")
                            ->FirstChildElement("text")->GetText());
            if (duration < 0.0f)
            {
                error << "Failed parsing '" << filename << "'. Reason: 'Arc "
                    << from->key << " -> " << to->key << " has negative duration'"
                    << std::endl;
                return error.str();
            }
        }
        auto error_msg = net.addArc(*from, *to, duration);
        if (!error_msg.empty())
        {
            error << "Failed parsing '" << filename << "'. Reason: '" << error_msg << "'" << std::endl;
            return error.str();
        }
    }
    return {};
}

} // namespace tpne
