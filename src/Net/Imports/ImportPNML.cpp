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

#include "Imports.hpp"
#include "TimedPetriNetEditor/PetriNet.hpp"
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
        error << "Failed opening '" << filename << "'. Reason was '"
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
        int angle = 0;
        net.addTransition(transition_id, caption, x, y, angle);
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
            error << "Failed parsing '" << filename << "'. Reason was 'Arc "
                << source << " -> " << target << " refer to unknown nodes'"
                << std::endl;
            return error.str();
        }

        float duration = NAN;
        if (child->FirstChildElement("inscription") != nullptr)
        {
            duration = std::stof(child->FirstChildElement("inscription")
                            ->FirstChildElement("text")->GetText());
            if (duration < 0.0f)
            {
                error << "Failed parsing '" << filename << "'. Reason was 'Arc "
                    << from->key << " -> " << to->key << " has negative duration'"
                    << std::endl;
                return error.str();
            }
        }
        if (!net.addArc(*from, *to, duration))
        {
            error << "Failed loading " << filename
                << ". Arc " << from->key << " -> " << to->key
                << " is badly formed" << std::endl;
        }
    }
    return {};
}

} // namespace tpne
