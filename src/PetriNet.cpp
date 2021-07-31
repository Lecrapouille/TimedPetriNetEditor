//=====================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 Quentin Quadrat <lecrapouille@gmail.com>
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
//=====================================================================

#include "PetriNet.hpp"
#include "utils/Howard.h"
#include "utils/Json.hpp"
#include "utils/Utils.hpp"
#include <iomanip>
#include <iostream>
#include <fstream>
#include <cstring>

//------------------------------------------------------------------------------
std::atomic<size_t> Place::s_next_id{0u};
std::atomic<size_t> Transition::s_next_id{0u};

//------------------------------------------------------------------------------
size_t Transition::canFire()
{
    if (arcsIn.size() == 0u)
        return 0u;

#if 1 // Version 1: return 0 or 1 token

    for (auto& a: arcsIn)
    {
        if (a->tokensIn() == 0u)
            return 0u;
    }
    return 1u;

#else // Version 2: return the maximum possibe of tokens that can be burnt

    size_t burnt = static_cast<size_t>(-1);

    for (auto& a: arcsIn)
    {
        size_t tokens = a->tokensIn();
        if (tokens == 0u)
            return 0u;

        if (tokens < burnt)
            burnt = tokens;
    }
    return burnt;

#endif
}

//------------------------------------------------------------------------------
void PetriNet::generateArcsInArcsOut()
{
    for (auto& trans: m_transitions)
    {
        trans.arcsIn.clear();
        trans.arcsOut.clear();

        for (auto& a: m_arcs)
        {
            if ((a.from.type == Node::Type::Place) && (a.to.id == trans.id))
                trans.arcsIn.push_back(&a);
            else if ((a.to.type == Node::Type::Place) && (a.from.id == trans.id))
                trans.arcsOut.push_back(&a);
        }
    }

    // if (true)
    for (auto& p: m_places)
    {
        p.arcsIn.clear();
        p.arcsOut.clear();

        for (auto& a: m_arcs)
        {
            if ((a.from.type == Node::Type::Transition) && (a.to.id == p.id))
                p.arcsIn.push_back(&a);
            else if ((a.to.type == Node::Type::Transition) && (a.from.id == p.id))
                p.arcsOut.push_back(&a);
        }
    }
}

//------------------------------------------------------------------------------
bool PetriNet::isEventGraph()
{
    // The Petri net shall be an event graph: all places shall have a single
    // input arc and a single output arc. Else, we cannot generate the linear
    // system.
    for (auto& p: m_places)
    {
        if (!((p.arcsIn.size() == 1u) && (p.arcsOut.size() == 1u)))
        {
            // Help the user to debug the Petri net. // TODO: could be nice to
            // show directly odd arcs in red but for the moment we display on
            // the console.
            std::cerr << "Your Petri net is not an event graph. Because:"
                      << std::endl;
            for (auto& p: m_places)
            {
                if (p.arcsOut.size() != 1u)
                {
                    std::cerr << "  " << p.key
                              << ((p.arcsOut.size() > 1u)
                                  ? " has more than one output arc:"
                                  : " has no output arc");
                    for (auto const& a: p.arcsOut)
                        std::cerr << " " << a->to.key;
                    std::cerr << std::endl;
                }

                if (p.arcsIn.size() != 1u)
                {
                    std::cerr << "  " << p.key
                              << ((p.arcsIn.size() > 1u)
                                  ? " has more than one input arc:"
                                  : " has no input arc");
                    for (auto const& a: p.arcsIn)
                        std::cerr << " " << a->from.key;
                    std::cerr << std::endl;
                }
            }
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------------------------
// TODO temporary: for the moment we only support Petri nets where Places have
// 0 or 1 tokens and places linked to inputs and outputs shall have 0 token.
bool PetriNet::exportToJulia(std::string const& filename)
{
    std::ofstream file(filename);
    if (!file)
    {
        std::cerr << "Failed to export the Petri net to '" << filename
                  << "'. Reason was " << strerror(errno) << std::endl;
        return false;
    }

    // Generate the Julia header
    file << "using MaxPlus, SparseArrays" << std::endl << std::endl;

    // Update arcs in/out for all transitions to be sure to generate the correct
    // net.
    generateArcsInArcsOut(/*arcs: true*/);

    if (!isEventGraph())
        return false;

    // Count the number of inputs, outputs and states for creating matrices.
    size_t nb_states = 0u;
    size_t nb_inputs = 0u;
    size_t nb_outputs = 0u;

    file << "## Petri Transitions:" << std::endl;

    // Show and count system inputs
    for (auto& t: m_transitions)
    {
        if (t.isInput())
        {
            t.mi = nb_inputs;
            nb_inputs += 1u;
            file << "# " << t.key << ": input (U"
                      << nb_inputs << ")" << std::endl;
        }
    }

    // Show and count system states
    for (auto& t: m_transitions)
    {
        if (t.isState())
        {
            t.mi = nb_states;
            nb_states += 1u;
            file << "# " << t.key << ": state (X"
                      << nb_states << ")" << std::endl;
        }
    }

    // Show and count system outputs
    for (auto& t: m_transitions)
    {
        if (t.isOutput())
        {
            t.mi = nb_outputs;
            nb_outputs += 1u;
            file << "# " << t.key << ": output (Y" << nb_outputs
                      << ")" << std::endl;
        }
    }

    // Graph representation
    file << std::endl;
    file << "## Timed graph event depict as two graph adjacency matrices:" << std::endl;
    file << "# Nodes are Transitions." << std::endl;
    file << "# Arcs are Places and therefore have tokens and durations" << std::endl;
    SparseMatrix N, T;
    for (auto& p: m_places)
    {
        // Since we are sure we are an event graph: places have a single input
        // arc and a aingle output arc.
        assert(p.arcsIn.size() == 1u);
        assert(p.arcsIn[0]->from.type == Node::Type::Transition);
        assert(p.arcsOut.size() == 1u);
        assert(p.arcsOut[0]->to.type == Node::Type::Transition);

        Transition& from = *reinterpret_cast<Transition*>(&(p.arcsIn[0]->from));
        Transition& to = *reinterpret_cast<Transition*>(&(p.arcsOut[0]->to));

        file << "# Arc " << p.key << ": " << from.key << " -> " << to.key
             << " (Duration: " << p.arcsIn[0]->duration
             << ", Tokens: " << p.tokens << ")" << std::endl;

        // Note origin and destination are inverted because of matrix product:
        // T * x with x a column vector
        T.push_back(SparseElement(to.id, from.id, p.arcsIn[0]->duration));
        N.push_back(SparseElement(to.id, from.id, p.tokens));
    }
    size_t const nnodes = m_transitions.size();
    file << "N = sparse(" << N << ", " << nnodes << ", " << nnodes << ") # Tokens" << std::endl;
    file << "T = sparse(" << T << ", " << nnodes << ", " << nnodes << ") # Durations" << std::endl;

    // Show the event graph to its Max-Plus counter form
    file << std::endl;
    file << "## Timed event graph represented as its counter form:" << std::endl;
    for (auto& t: m_transitions)
    {
        if (t.arcsIn.size() == 0u)
            continue;

        file << "# " << t.key << "(t) = min(";
        std::string separator1;
        for (auto& ai: t.arcsIn)
        {
            file << separator1;
            file << ai->tokensIn() << " + ";
            std::string separator2;
            for (auto& ao: ai->from.arcsIn)
            {
                file << separator2;
                file << ao->from.key << "(t - " << ao->duration << ")";
                separator2 = ", ";
            }
            separator1 = ", ";
        }
        file << ");" << std::endl;
    }

    // Show the event graph to its Max-Plus dater form
    file << std::endl;
    file << "## Timed event graph represented as its dater form:" << std::endl;
    for (auto& t: m_transitions)
    {
        if (t.arcsIn.size() == 0u)
            continue;

        file << "# " << t.key << "(n) = max(";
        std::string separator1;
        for (auto& ai: t.arcsIn)
        {
            file << separator1;
            std::string separator2;
            for (auto& ao: ai->from.arcsIn)
            {
                file << separator2;
                file << ao->duration << " + " << ao->from.key
                          << "(n - " << ai->tokensIn() << ")";
                separator2 = ", ";
            }
            separator1 = ", ";
        }
        file << ");" << std::endl;
    }

    file << std::endl;
    file << "## Max-Plus implicit linear dynamic system of the dater form:" << std::endl;
    file << "# X(n) = D X(n) (+) A X(n-1) + B U(n)" << std::endl;
    file << "# Y(n) = C X(n)" << std::endl;

    // Compute the syslin as Julia code using the Max-Plus package
    // X(n) = D X(n) (+) A X(n-1) + B U(n)
    // Y(n) = C X(n)
    SparseMatrix A, D, B, C;
    for (auto& arc: m_arcs)
    {
        if (arc.from.type == Node::Type::Place)
            continue;

        Transition& t = *reinterpret_cast<Transition*>(&(arc.from));
        if (t.isInput())
        {
            // System inputs: B U(n)
            B.push_back(SparseElement(t.mi, t.mi, arc.duration));
        }
        else // States or outputs
        {
            Place& p = *reinterpret_cast<Place*>(&(arc.to));
            for (auto& a: p.arcsOut)
            {
                Transition& td = *reinterpret_cast<Transition*>(&(a->to));

                if (td.isState())
                {
                    // Systems states: X(n) = D X(n) (+) A X(n-1)
                    if (p.tokens == 1u)
                    {
                        A.push_back(SparseElement(td.mi, t.mi, arc.duration));
                    }
                    else
                    {
                        D.push_back(SparseElement(td.mi, t.mi, arc.duration));
                    }
                }
                else if (td.isOutput())
                {
                    // System outputs: Y(n) = C X(n)
                    C.push_back(SparseElement(t.mi, t.mi, arc.duration));
                }
            }
        }
    }

    // Julia Max-Plus Linear system
    file << "D = sparse(" << D << ", " << nb_states << ", " << nb_states << ") # States without tokens" << std::endl;
    file << "A = sparse(" << A << ", " << nb_states << ", " << nb_states << ") # States with 1 token" << std::endl;
    file << "B = sparse(" << B << ", " << nb_inputs << ", " << nb_inputs << ") # Inputs" << std::endl;
    file << "C = sparse(" << C << ", " << nb_outputs << ", " << nb_outputs << ") # Outputs" << std::endl;
    file << "S = MPSysLin(A, B, C, D)" << std::endl;

    // Semi-Howard
    file << std::endl;
    file << "#" << std::endl;
    file << "TODO l,v = semihoward(S.D, S.A)" << std::endl;

    return true;
}

//------------------------------------------------------------------------------
bool PetriNet::showCriticalCycle()
{
    // Update arcs in/out for all transitions to be sure to generate the correct
    // net.
    generateArcsInArcsOut(/*arcs: true*/);

    if (!isEventGraph())
        return false;

    // Reserve memory
    size_t const nnodes = m_transitions.size();
    size_t const narcs = m_places.size();
    std::vector<double> N; N.reserve(narcs);
    std::vector<double> T; T.reserve(narcs);
    std::vector<int> IJ; IJ.reserve(2u * narcs);

    for (auto& p: m_places)
    {
        // Since we are sure we are an event graph: places have a single input
        // arc and a aingle output arc.
        assert(p.arcsIn.size() == 1u);
        assert(p.arcsIn[0]->from.type == Node::Type::Transition);
        assert(p.arcsOut.size() == 1u);
        assert(p.arcsOut[0]->to.type == Node::Type::Transition);

        Transition& from = *reinterpret_cast<Transition*>(&(p.arcsIn[0]->from));
        Transition& to = *reinterpret_cast<Transition*>(&(p.arcsOut[0]->to));

        std::cout << "# Arc " << p.key << ": " << from.key << " -> " << to.key
                  << " (Duration: " << p.arcsIn[0]->duration << ", Tokens: "
                  << p.tokens << ")" << std::endl;

        IJ.push_back(to.id); // Transposed is needed
        IJ.push_back(from.id);
        T.push_back(p.arcsIn[0]->duration);
        N.push_back(p.tokens);
    }

    std::vector<double> V(nnodes); // bias
    std::vector<double> chi(nnodes); // cycle time vector
    std::vector<int> policy(nnodes); // optimal policy
    int ncomponents; //
    int niterations; // nb of iteration of the algorithm
    int verbosemode = 1;
    int res = Semi_Howard(IJ.data(), T.data(), N.data(),
                          nnodes, narcs,
                          chi.data(), V.data(), policy.data(), &niterations,
                          &ncomponents, verbosemode);

    std::cout << "V=";
    for (auto const& it: V)
    {
        std::cout << ' ' << it;
    }
    std::cout << std::endl << "CHI=";
    for (auto const& it: chi)
    {
        std::cout << ' ' << it;
    }
    std::cout << std::endl << "Nb Policy=" << ncomponents << std::endl;
    if (ncomponents > 0)
    {
        std::cout << "POLICY=";
        for (auto const& it: policy)
        {
            std::cout << ' ' << it;
        }
        std::cout << std::endl;
    }

    if (ncomponents > 0)
    {
        size_t to = 0u;
        m_critical.clear();
        m_critical.reserve(nnodes);
        std::cout << "Critical cycle:" << std::endl;
        for (auto const& from: policy)
        {
            std::cout << "T" << from << " -> T" << to << std::endl;
            for (auto const& it: m_transitions[from].arcsOut)
            {
                // Since we are working on an Event Graph we can directly access
                // Place -> arcsOut[0] -> Transition without checks.
                assert(it->to.arcsOut[0] != nullptr);
                assert(it->to.arcsOut[0]->to.type == Node::Type::Transition);
                if (it->to.arcsOut[0]->to.id == to)
                {
                    m_critical.push_back(it);
                    m_critical.push_back(it->to.arcsOut[0]);
                    break;
                }
            }
            to += 1u;
        }
    }
    else
    {
        std::cerr << "No policy found" << std::endl;
        return false;
    }
    return res == 0;
}

//------------------------------------------------------------------------------
bool PetriNet::exportToCpp(std::string const& filename, std::string const& name)
{
    std::ofstream file(filename);
    if (!file)
    {
        std::cerr << "Failed to export the Petri net to '" << filename
                  << "'. Reason was " << strerror(errno) << std::endl;
        return false;
    }

    std::string upper_name(name);
    std::for_each(upper_name.begin(), upper_name.end(), [](char & c) {
        c = ::toupper(c);
    });

    // Update arcs in/out for all transitions to be sure to generate the correct
    // net.
    generateArcsInArcsOut();

    file << "// This file has been generated and you should avoid editing it." << std::endl;
    file << "// Note: the code generator is still experimental !" << std::endl;
    file << "" << std::endl;
    file << "#ifndef GENERATED_GRAFCET_" << upper_name << "_HPP" << std::endl;
    file << "#  define GENERATED_GRAFCET_" << upper_name << "_HPP" << std::endl;
    file << "" << std::endl;
    file << "#  include <iostream>" << std::endl;
    file << "" << std::endl;
    file << "namespace " << name << " {" << std::endl;

    file << R"PN(
class Grafcet
{
public:

    Grafcet()
    {
        initIO();
        reset();
    }

    void reset()
    {
        // Initial states
)PN";

    for (size_t i = 0; i < m_places.size(); ++i)
    {
        file << "        X[" << m_places[i].id << "] = "
             << (m_places[i].tokens ? "true; " : "false;")
             << std::endl;
    }

    file << R"PN(    }

    void update()
    {
)PN";

    file << "        // Do actions of enabled steps" << std::endl;
    for (size_t p = 0u; p < m_places.size(); ++p)
    {
        file << "        if (X[" << p << "]) { X" << p << "(); }"
             << std::endl;
    }

    file << std::endl;
    file << "        // Read inputs (TODO)" << std::endl << std::endl;
    file << "        // Update transitions" << std::endl;

    for (size_t t = 0u; t < m_transitions.size(); ++t)
    {
        Transition& trans = m_transitions[t];
        file << "        T[" << trans.id << "] =";
        for (size_t a = 0; a < trans.arcsIn.size(); ++a)
        {
            Arc& arc = *trans.arcsIn[a];
            if (a > 0u) {  file << " &&"; }
            file << " X[" << arc.from.id << "]";
        }
        file << " && T"  << trans.id << "();";
        file << " // " << trans.key << std::endl;
                                                         }

    file  << std::endl << "        // Update steps" << std::endl;
    for (size_t t = 0u; t < m_transitions.size(); ++t)
    {
        Transition& trans = m_transitions[t];
        file << "        if (T[" << t << "]) {" << std::endl;
        for (size_t a = 0; a < trans.arcsIn.size(); ++a)
        {
            Arc& arc = *trans.arcsIn[a];
            file << "            X[" << arc.from.id
                 << "] = false; // Disable " << arc.from.key << std::endl;
        }
        for (size_t a = 0; a < trans.arcsOut.size(); ++a)
        {
            Arc& arc = *trans.arcsOut[a];
            file << "            X[" << arc.to.id
                 << "] = true; // Enable " << arc.to.key << std::endl;
        }
        file << "        }" << std::endl << std::endl;
    }

    file << "        // Set output values (TODO)" << std::endl;

    file << R"PN(    }

    void debug()
    {
       std::cout << "Transitions:" << std::endl;
       for (size_t i = 0u; i < MAX_TRANSITIONS; ++i)
          std::cout << "  T[" << i << "] = " << T[i] << std::endl;

       //std::cout << "Inputs:" << std::endl;
       //for (size_t i = 0u; i < MAX_INPUTS; ++i)
       //   std::cout << "  I[" << i << "] = " << I[i] << std::endl;

       std::cout << "Steps:" << std::endl;
       for (size_t i = 0u; i < MAX_STEPS; ++i)
          std::cout << "  X[" << i << "] = " << X[i] << std::endl;

       //std::cout << "Outputs:" << std::endl;
       //for (size_t i = 0u; i < MAX_OUTPUTS; ++i)
       //   std::cout << "  O[" << i << "] = " << O[i] << std::endl;
    }

private:

    //! \brief Fonction not generated to let the user initializing
    //! inputs (i.e. TTL gpio, ADC ...) and outputs (i.e. TTL gpio,
    //! PWM ...)
    void initIO();
)PN";

    for (auto const& t: m_transitions)
    {
        file << "    //! \\brief Fonction not generated to let the user writting the" << std::endl;
        file << "    //! transitivity for the Transition " << t.id <<  " depending of the system" << std::endl;
        file << "    //! inputs I[]." << std::endl;
        file << "    //! \\return true if the transition is enabled." << std::endl;
        file << "    bool T" << t.id << "() const;" << std::endl;
    }

    for (auto const& p: m_places)
    {
        file << "    //! \\brief Fonction not generated to let the user writting the" << std::endl;
        file << "    //! reaction for the Step " << p.id << std::endl;
        file << "    void X" << p.id << "();" << std::endl;
    }

    file << R"PN(
private:

)PN";

    file << "    static const size_t MAX_STEPS = " << m_places.size() << "u;"  << std::endl;
    file << "    static const size_t MAX_TRANSITIONS = " << m_transitions.size() << "u;" << std::endl;
    file << "    //static const size_t MAX_INPUTS = X;" << std::endl;
    file << "    //static const size_t MAX_OUTPUTS = Y;" << std::endl;
    file << "" << std::endl;
    file << "    //! \\brief Steps"  << std::endl;
    file << "    bool X[MAX_STEPS];" << std::endl;
    file << "    //! \\brief Transitions"  << std::endl;
    file << "    bool T[MAX_TRANSITIONS];" << std::endl;
    file << "    //! \\brief Inputs"  << std::endl;
    file << "    //uint16_t I[MAX_INPUTS];" << std::endl;
    file << "    //! \\brief Outputs"  << std::endl;
    file << "    //uint16_t O[MAX_OUTPUTS];" << std::endl;
    file << "};" << std::endl;
    file << "" << std::endl;
    file << "} // namespace " << name << std::endl;
    file << "#endif // GENERATED_GRAFCET_" << upper_name << "_HPP" << std::endl;

    std::cerr << "Petri net saved into file '" << filename << "'" << std::endl;
    return true;
}

//------------------------------------------------------------------------------
bool PetriNet::save(std::string const& filename)
{
    std::string separator;
    std::ofstream file(filename);

    if (isEmpty())
    {
        std::cerr << "I'll not save empty net" << std::endl;
        return false;
    }

    if (!file)
    {
        std::cerr << "Failed saving the Petri net in '" << filename
                  << "'. Reason was " << strerror(errno) << std::endl;
        return false;
    }

    file << "{\n  \"places\": [";
    for (auto const& p: m_places)
    {
        file << separator << '\"' << p.key << ','
             << p.x << ',' << p.y << ',' << p.tokens << '\"';
        separator = ", ";
    }
    file << "],\n  \"transitions\": [";
    separator = "";
    for (auto const& t: m_transitions)
    {
        file << separator << '\"' << t.key << ','
             << t.x << ',' << t.y << ',' << t.angle << '\"';
        separator = ", ";
    }
    file << "],\n  \"arcs\": [";
    separator = "";
    for (auto const& a: m_arcs)
    {
        file << separator << '\"' << a.from.key << ','
             << a.to.key << ',' << a.duration << '\"';
        separator = ", ";
    }
    file << "]\n}";

    std::cerr << "Petri net saved into file '" << filename << "'" << std::endl;
    return true;
}

//------------------------------------------------------------------------------
bool PetriNet::load(std::string const& filename)
{
    bool found_places = false;
    bool found_transitions = false;
    bool found_arcs = false;

    Spliter s(filename, " \",");

    if (!s)
    {
        std::cerr << "Failed opening '" << filename << "'. Reason was '"
                  << strerror(errno) << "'" << std::endl;
        return false;
    }

    if (s.split() != "{")
    {
        std::cerr << "Token { missing. Bad JSON file" << std::endl;
        return false;
    }

    reset();

    while (s)
    {
        s.split();
        if ((s.str() == "places") && (s.split() == ":") && (s.split() == "["))
        {
            found_places = true;
            while (s.split() != "]")
            {
                int id = convert_to<size_t>(s.str().c_str() + 1u);
                float x = convert_to<float>(s.split());
                float y = convert_to<float>(s.split());
                int tokens = convert_to<size_t>(s.split());
                if ((id < 0) || (tokens < 0))
                {
                    std::cerr << "Unique identifiers and tokens shall be > 0" << std::endl;
                    return false;
                }
                addPlace(size_t(id), x, y, size_t(tokens));
            }
        }
        else if ((s.str() == "transitions") && (s.split() == ":") && (s.split() == "["))
        {
            found_transitions = true;
            while (s.split() != "]")
            {
                int id = convert_to<size_t>(s.str().c_str() + 1u);
                float x = convert_to<float>(s.split());
                float y = convert_to<float>(s.split());
                int angle = convert_to<int>(s.split());
                if (id < 0)
                {
                    std::cerr << "Unique identifiers shall be > 0" << std::endl;
                    return false;
                }
                addTransition(size_t(id), x, y, angle);
            }
        }
        else if ((s.str() == "arcs") && (s.split() == ":") && (s.split() == "["))
        {
            found_arcs = true;
            while (s.split() != "]")
            {
                Node* from = findNode(s.str());
                if (!from)
                {
                    std::cerr << "Origin node " << s.str() << " not found" << std::endl;
                    return false;
                }

                Node* to = findNode(s.split());
                if (!to)
                {
                    std::cerr << "Destination node " << s.str() << " not found" << std::endl;
                    return false;
                }

                float duration = stof(s.split());
                if (duration < 0.0f)
                {
                    std::cout << "Duration " << duration << " shall be > 0" << std::endl;
                    return false;
                }
                if (!addArc(*from, *to, duration))
                {
                    std::cerr << "Arc " << from->key << " -> " << to->key
                              << " is badly formed" << std::endl;
                    return false;
                }
            }
        }
        else if (s.str() == "}")
        {
            if (!found_places)
                std::cerr << "The JSON file did not contained Places" << std::endl;
            if (!found_transitions)
                std::cerr << "The JSON file did not contained Transitions" << std::endl;
            if (!found_arcs)
                std::cerr << "The JSON file did not contained Arcs" << std::endl;

            if (!(found_places && found_transitions && found_arcs))
                return false;
        }
        else if (s.str() != "")
        {
            std::cerr << "Key " << s.str() << " is not a valid token" << std::endl;
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------------------------
Node* PetriNet::findNode(std::string const& key)
{
    if (key[0] == 'P')
    {
        for (auto& p: m_places)
        {
            if (p.key == key)
                return &p;
        }
        return nullptr;
    }

    if (key[0] == 'T')
    {        for (auto& t: m_transitions)
        {
            if (t.key == key)
                return &t;
        }
        return nullptr;
    }

    std::cerr << "Node key shall start with 'P' or 'T'" << std::endl;
    return nullptr;
}

//------------------------------------------------------------------------------
void PetriNet::removeNode(Node& node)
{
    // Remove all arcs linked to this node.
    // Note: For fastest deletion, we simply swap the undesired arc with the
    // latest arc in the container. To do that, we have to iterate from the end
    // of the container.
    size_t s = m_arcs.size();
    size_t i = s;
    while (i--)
    {
        if ((m_arcs[i].to == node) || (m_arcs[i].from == node))
        {
            // Found the undesired arc: make the latest element take its
            // location in the container.
            m_arcs[i] = m_arcs[m_arcs.size() - 1u];
            m_arcs.pop_back();
        }
    }

    // Search and remove the node.
    // Note: For fastest deletion, we simply swap the undesired node with the
    // latest node in the container. To do that, we have to iterate from the end
    // of the container.
    if (node.type == Node::Type::Place)
    {
        size_t i = m_places.size();
        while (i--)
        {
            // Found the undesired node: make the latest element take its
            // location in the container. But before doing this we have to
            // restore references on impacted arcs.
            if (m_places[i].id == node.id)
            {
                // Swap element but keep the ID of the removed element
                Place& pi = m_places[i];
                Place& pe = m_places[m_places.size() - 1u];
                m_places[i] = Place(pi.id, pe.x, pe.y, pe.tokens);
                Place::s_next_id -= 2u;

                // Update the references to nodes of the arc
                for (auto& a: m_arcs) // TODO optim: use in/out arcs but they may not be generated
                {
                    if (a.to == pe)
                        a = Arc(a.from, m_places[i], a.duration);
                    if (a.from == pe)
                        a = Arc(m_places[i], a.to, a.duration);
                }

                m_places.pop_back();
            }
        }
    }
    else
    {
        size_t i = m_transitions.size();
        while (i--)
        {
            if (m_transitions[i].id == node.id)
            {
                Transition& ti = m_transitions[i];
                Transition& te = m_transitions[m_transitions.size() - 1u];
                m_transitions[i] = Transition(ti.id, te.x, te.y, te.angle);
                Transition::s_next_id -= 2u;

                for (auto& a: m_arcs) // TODO idem
                {
                    if (a.to == te)
                        a = Arc(a.from, m_transitions[i], a.duration);
                    if (a.from == te)
                        a = Arc(m_transitions[i], a.to, a.duration);
                }

                m_transitions.pop_back();
            }
        }
    }

    // Restore in arcs and out arcs for each node
    generateArcsInArcsOut();
}
