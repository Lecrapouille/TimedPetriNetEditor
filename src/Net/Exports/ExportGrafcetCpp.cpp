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

#include "TimedPetriNetEditor/PetriNet.hpp"
#include "Net/Exports/Exports.hpp"
#include "Net/Receptivities.hpp"
#include <fstream>
#include <cstring>

namespace tpne {

static std::string camelCase(std::string const& line)
{
    std::string res(line);
    bool active = true;

    for(int i = 0; res[i] != '\0'; i++)
    {
        if (std::isalpha(res[i]))
        {
            if (active)
            {
                res[i] = char(std::toupper(res[i]));
                active = false;
            }
            else
            {
                res[i] = char(std::tolower(res[i]));
            }
        }
        else if (res[i] == ' ')
        {
            active = true;
        }
    }
    return res;
}

//------------------------------------------------------------------------------
std::string exportToGrafcetCpp(Net const& net, std::string const& filename)
{
    // Open the file
    std::ofstream file(filename);
    if (!file)
    {
        std::stringstream error;
        error << "Failed to export the Petri net to '" << filename
              << "'. Reason was " << strerror(errno) << std::endl;
        return error.str();
    }

    // Generate the C++ namespace
    std::string name_space = net.name;
      std::for_each(name_space.begin(), name_space.end(), [](char & c) {
        c = char(::tolower(int(c)));
        if (c == ' ') { c = '_'; }
    });
    // Generate the C++ header guards
    std::string header_guards(name_space);
    std::for_each(header_guards.begin(), header_guards.end(), [](char & c) {
        c = char(::toupper(int(c)));
        if (c == ' ') { c = '_'; }
    });

    file << "// This file has been generated and you should avoid editing it." << std::endl;
    file << "// Note: the code generator is still experimental !" << std::endl;
    file << "" << std::endl;
    file << "#ifndef GENERATED_GRAFCET_" << header_guards << "_HPP" << std::endl;
    file << "#  define GENERATED_GRAFCET_" << header_guards << "_HPP" << std::endl;
    file << "" << std::endl;
    // FIXME #ifndef GRAFCET_WITH_DEBUG
    //file << "#  include <iostream>" << std::endl;
    //file << "" << std::endl;
    file << "#  ifndef GRAFCET_SENSOR_TYPE" << std::endl;
    file << "#    define GRAFCET_SENSOR_TYPE bool" << std::endl;
    file << "#  endif" << std::endl << std::endl;
    file << "namespace " << name_space << " {" << std::endl;

    file << R"PN(
// *****************************************************************************
//! \brief
// *****************************************************************************
class Grafcet
{
public:

    //-------------------------------------------------------------------------
    //! \brief Restore all states of the GRAFCET to their initial states.
    //-------------------------------------------------------------------------
    Grafcet() { initInputsGPIOs(); initOutputGPIOs(); reset(); }

    //-------------------------------------------------------------------------
    //! \brief Reset the sequence to the initial step.
    //-------------------------------------------------------------------------
    void reset()
    {
)PN";
    file << "// Reset sensors ?" << std::endl;
    file << "        init = true;" << std::endl;
    auto const& places = net.places();
    for (size_t i = 0; i < places.size(); ++i)
    {
        file << "        X[" << places[i].id << "] = "
             << (places[i].tokens ? "true; " : "false;")
             << std::endl;
    }

    file << R"PN(    }

    //-------------------------------------------------------------------------
    //! \brief Update one cycle of the GRAFCET: read sensors, update states,
    //! write outputs. The update follows the document
    //! http://legins69.free.fr/automatisme/PL7Pro/GRAFCET.pdf
    //-------------------------------------------------------------------------
    void update()
    {
)PN";

    std::string del;

    // Read sensors
    file << "        // Read sensors:" << std::endl;
    for (auto const& s: Sensors::instance().database())
    {
        file << "        " << s.first
             << " = readSensor" << camelCase(s.first) << "();"
             << std::endl;
    }

    file << std::endl << "        // Update GRAFCET states:" << std::endl;
    // Compute T[n] = X[n] . R[n]
    for (auto const& t: net.transitions())
    {
        file << "        T[" << t.id << "] = ";
        del = "";
        for (auto const& p: t.arcsIn)
        {
            file << del << "X[" << p->from.id << "]";
            del = " & ";
        }
        file << del << t.key << "();"
             << " // Transition " << t.id << ": " << t.caption
             << std::endl;
    }

    // Compute X[n] = T[n-1] + X[n] . /T[n]
    for (auto const& p: net.places())
    {
        file << "        X[" << p.id << "] = ";
        del = "";
        for (auto const& t: p.arcsIn)
        {
            file << del << "T[" << t->from.id << "]";
            del = " | ";
        }
        if (p.arcsIn.size() > 0u)
        {
            file << " | ";
        }
        if (p.arcsOut.size() == 0u)
        {
            file << "X[" << p.id << "]";
        }
        else
        {
            file << "(X[" << p.id << "] & ";
            del = "";
            for (auto const& t: p.arcsOut)
            {
                file << del << "(!T[" << t->to.id << "])";
                del = " & ";
            }
            file << ")";
        }
        if (p.tokens > 0u)
        {
            file << " | init";
        }
        file << "; // Step " << p.id << ": " << p.caption << std::endl;
    }

    file << std::endl << "        // Update outputs:" << std::endl;
    // TODO Sorties
    // Pour toutes les sorties: faire la liste des Etapes qui les utilisent avec |
    for (auto const& p: net.places())
    {
        file << "        outputs[xxx] = X[yyy] + (X[zzz] & inibiteur[zzz]);" << std::endl;
    }
    for (auto const& p: net.places())
    {
        file << "        P" << p.id << "(outputs[xxx]);" << std::endl;
    }

    file << std::endl << "        // End of the initial GRAFCET cycle" << std::endl;
    file << "        init = false;";
    file << R"PN(
    }

private:  // You have to implement the following methods in the C++ file

    //-------------------------------------------------------------------------
    //! \brief Initialize the input GPIOs.
    //-------------------------------------------------------------------------
    void initInputsGPIOs();
    //-------------------------------------------------------------------------
    //! \brief Initialize the output GPIOs.
    //-------------------------------------------------------------------------
    void initOutputGPIOs();

)PN";

    for (auto const& s: Sensors::instance().database())
    {
        file << "    //-------------------------------------------------------------------------" << std::endl;
        file << "    //! \\brief Read sensor " << s.first << std::endl;
        file << "    //-------------------------------------------------------------------------" << std::endl;
        file << "    bool readSensor" << camelCase(s.first) << "();" << std::endl;
    }

    file << std::endl;
    for (auto const& t: net.transitions())
    {
        file << "    //-------------------------------------------------------------------------" << std::endl;
        file << "    //! \\brief Compute the receptivity of the transition " << t.id << "." << std::endl;
        file << "    //! RPN boolean equation: \"" << t.caption << "\"" << std::endl;
        file << "    //! \\return true if the transition is enabled." << std::endl;
        file << "    //-------------------------------------------------------------------------" << std::endl;
        file << "    bool T" << t.id << "() const { return !!("
             << Receptivity::Parser::translate(t.caption, "C")
             << "); }" << std::endl;
    }

    file << std::endl;
    for (auto const& p: net.places())
    {
        file << "    //-------------------------------------------------------------------------" << std::endl;
        file << "    //! \\brief Do actions associated with the step " << p.id << ": " << p.caption << std::endl;
        file << "    //-------------------------------------------------------------------------" << std::endl;
        file << "    void P" << p.id << "(const bool activated);" << std::endl;
    }

    file << std::endl << "private:" << std::endl << std::endl;
    file << "    //! \\brief States of transitions."  << std::endl;
    file << "    bool T[" << net.transitions().size() << "];" << std::endl;
    file << "    //! \\brief States of steps." << std::endl;
    file << "    bool X[" << net.places().size() << "];" << std::endl;
    file << "    //! \\brief List of sensors:"  << std::endl;
    for (auto const& s: Sensors::instance().database())
    {
        file << "    GRAFCET_SENSOR_TYPE " << s.first << " = " << s.second << ";" << std::endl;
    }
    //file << "    //! \\brief List of actions:"  << std::endl;
    //for (auto const& s: Actuators::instance().database())
    //{
    //    file << "    GRAFCET_SENSOR_TYPE " << s.first << " = " << s.second << ";" << std::endl;
    //}
    file << "    //! \\brief Initial GRAFCET cycle." << std::endl;
    file << "    bool init = true;" << std::endl;
    file << "};" << std::endl;
    file << "" << std::endl;
    file << "} // namespace " << name_space << std::endl;
    file << "#endif // GENERATED_GRAFCET_" << header_guards << "_HPP" << std::endl;

    return {};
}

} // namespace tpne
