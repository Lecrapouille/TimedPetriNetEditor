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

    // Generate the C++ namespace and header guards
    std::string name_space = net.name;
    std::string header_guards(name_space);
    std::for_each(header_guards.begin(), header_guards.end(), [](char & c) {
        c = char(::toupper(int(c)));
    });

    file << "// This file has been generated and you should avoid editing it." << std::endl;
    file << "// Note: the code generator is still experimental !" << std::endl;
    file << "" << std::endl;
    file << "#ifndef GENERATED_GRAFCET_" << header_guards << "_HPP" << std::endl;
    file << "#  define GENERATED_GRAFCET_" << header_guards << "_HPP" << std::endl;
    file << "" << std::endl;
    file << "#  include <iostream>" << std::endl;
    file << "#  include \"MQTT.hpp\"" << std::endl;
    file << "" << std::endl;
    file << "namespace " << name_space << " {" << std::endl;

    file << R"PN(
// *****************************************************************************
//! \brief
// *****************************************************************************
class Grafcet: public MQTT
{
private: // MQTT

    //-------------------------------------------------------------------------
    //! \brief Callback when this class is connected to the MQTT broker.
    //-------------------------------------------------------------------------
    virtual void onConnected(int /*rc*/) override;

    //-------------------------------------------------------------------------
    //! \brief Callback when this class is has received a new message from the
    //! MQTT broker.
    //-------------------------------------------------------------------------
    virtual void onMessageReceived(const struct mosquitto_message& message) override;

    //-------------------------------------------------------------------------
    //! \brief Transmit to the Petri net editor all transitions that have been
    //! fired.
    //-------------------------------------------------------------------------
    void publish()
    {
        static char message[MAX_TRANSITIONS + 1u] = { 'T' };

        for (size_t i = 0u; i < MAX_TRANSITIONS; ++i)
            message[i + 1u] = T[i];

        MQTT::publish(topic().c_str(), std::string(message, MAX_TRANSITIONS + 1u), MQTT::QoS::QoS0);
    }

public:

    //-------------------------------------------------------------------------
    //! \brief Restore all states of the GRAFCET to their initial states.
    //-------------------------------------------------------------------------
    Grafcet() { initGPIO(); reset(); }

    //-------------------------------------------------------------------------
    //! \brief Return the MQTT topic to talk with the Petri net editor.
    //! Call Grafcet grafcet
    //-------------------------------------------------------------------------
    std::string& topic() { return m_topic; }

    //-------------------------------------------------------------------------
    //! \brief Print values of transitions and steps
    //-------------------------------------------------------------------------
    void debug() const
    {
       std::cout << "Transitions:" << std::endl;
       for (size_t i = 0u; i < MAX_TRANSITIONS; ++i)
       {
          std::cout << "  Transition[" << i << "] = " << T[i]
                    << std::endl;
       }

       std::cout << "Steps:" << std::endl;
       for (size_t i = 0u; i < MAX_STEPS; ++i)
       {
          std::cout << "  Step[" << i << "] = " << X[i]
                    << std::endl;
       }
    }

    //-------------------------------------------------------------------------
    //! \brief Desactivate all steps except the ones initially activated
    //-------------------------------------------------------------------------
    void reset()
    {
)PN";

    auto const& places = net.places();
    for (size_t i = 0; i < places.size(); ++i)
    {
        file << "        X[" << places[i].id << "] = "
             << (places[i].tokens ? "true; " : "false;")
             << " // " << places[i].caption
             << std::endl;
    }

    file << R"PN(    }

    //-------------------------------------------------------------------------
    //! \brief
    //-------------------------------------------------------------------------
    void step()
    {
        doActions();
        readInputs();
        setTransitions();
        setSteps();
    }

private:

    //-------------------------------------------------------------------------
    //! \brief
    //-------------------------------------------------------------------------
    void initGPIO();

    //-------------------------------------------------------------------------
    //! \brief
    //-------------------------------------------------------------------------
    void readInputs();

    //-------------------------------------------------------------------------
    //! \brief
    //-------------------------------------------------------------------------
    void doActions()
    {
)PN";

    for (size_t p = 0u; p < net.places().size(); ++p)
    {
        file << "        if (X[" << p << "]) { P" << p << "(); }"
             << std::endl;
    }

    file << "    }" << std::endl << R"PN(
    //-------------------------------------------------------------------------
    //! \brief
    //-------------------------------------------------------------------------
    void setTransitions()
    {
)PN";

    for (auto const& trans: net.transitions())
    {
        file << "        T[" << trans.id << "] =";
        for (size_t a = 0; a < trans.arcsIn.size(); ++a)
        {
            Arc& arc = *trans.arcsIn[a];
            if (a > 0u) { file << " &&"; }
            file << " X[" << arc.from.id << "]";
        }
        file << " && T"  << trans.id << "();\n";
    }

    file << "        publish();" << std::endl << "    }" << std::endl << R"PN(
    //-------------------------------------------------------------------------
    //! \brief
    //-------------------------------------------------------------------------
    void setSteps()
    {
)PN";

    for (auto const& trans: net.transitions())
    {
        file << "        if (T[" << trans.id << "])" << std::endl;
        file << "        {" << std::endl;

        for (auto const& arc: trans.arcsIn)
        {
            file << "            X[" << arc->from.id << "] = false;" << std::endl;
        }

        for (auto const& arc: trans.arcsOut)
        {
            file << "            X[" << arc->to.id << "] = true;" << std::endl;
        }

        file << "        }" << std::endl;;
    }

    file << "    }" << std::endl << std::endl << "private: // You have to implement the following methods in the C++ file"
         << std::endl << std::endl;

    for (auto const& t: net.transitions())
    {
        file << "    //-------------------------------------------------------------------------" << std::endl;
        file << "    //! \\brief Transition " << t.id <<  ": \"" << t.caption << "\"" << std::endl;
        file << "    //! \\return true if the transition is enabled." << std::endl;
        file << "    //-------------------------------------------------------------------------" << std::endl;
        if (net.type() == TypeOfNet::GRAFCET)
        {
            file << "    bool T" << t.id << "() { return " << Receptivity::Parser::translate(t.caption, "C") << "; } const";
        }
        else
        {
            file << "    bool T" << t.id << "() const;";
        }
        file << std::endl << std::endl;
    }

    for (auto const& p: net.places())
    {
        file << "    //-------------------------------------------------------------------------" << std::endl;
        file << "    //! \\brief Do actions associated with the step " << p.id << ": " << p.caption << std::endl;
        file << "    //-------------------------------------------------------------------------" << std::endl;
        file << "    void P" << p.id << "();" << std::endl << std::endl;
    }

    file << std::endl << "private:" << std::endl << std::endl;
    file << "    const size_t MAX_STEPS = " << net.places().size() << "u;"  << std::endl;
    file << "    const size_t MAX_TRANSITIONS = " << net.transitions().size() << "u;" << std::endl;
    file << "    //! \\brief Steps"  << std::endl;
    file << "    bool X[MAX_STEPS];" << std::endl;
    file << "    //! \\brief Transitions"  << std::endl;
    file << "    bool T[MAX_TRANSITIONS];" << std::endl;
    file << "    //! \\brief MQTT topic to communicate with the Petri net editor"  << std::endl;
    file << "    std::string m_topic = \"pneditor/" << name_space << "\";" << std::endl;
    for (auto const& s: Sensors::instance().database())
    {
        file << "    //! \\brief"  << std::endl;
        file << "    bool " << s.first << " = " << s.second << ";" << std::endl;
    }
    file << "};" << std::endl;
    file << "" << std::endl;
    file << "} // namespace " << name_space << std::endl;
    file << "#endif // GENERATED_GRAFCET_" << header_guards << "_HPP" << std::endl;

    return {};
}

} // namespace tpne
