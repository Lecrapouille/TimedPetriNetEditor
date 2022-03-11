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
#include "utils/Splitter.hpp"
#include "utils/Utils.hpp"
#include <iomanip>
#include <iostream>
#include <fstream>
#include <cstring>

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
bool PetriNet::addArc(Node& from, Node& to, float const duration)
{
    if (from.type == to.type)
    {
        std::cerr << "Failed adding arc " << from.key
                  << " --> " << to.key
                  << ": nodes type shall not be the same"
                  << std::endl;
        return false;
    }

    if (findArc(from, to) != nullptr)
    {
        std::cerr << "Failed adding arc " << from.key
                  << " --> " << to.key
                  << ": Arc already exist"
                  << std::endl;
        return false;
    }

    if (findNode(from.key) == nullptr)
    {
        std::cerr << "Failed adding arc " << from.key
                  << " --> " << to.key
                  << ": The node " << from.key
                  << " does not exist"
                  << std::endl;
        return false;
    }

    if (findNode(to.key) == nullptr)
    {
        std::cerr << "Failed adding arc " << from.key
                  << " --> " << to.key
                  << ": The node " << to.key
                  << " does not exist"
                  << std::endl;
        return false;
    }

    m_arcs.push_back(Arc(from, to, duration));
    from.arcsOut.push_back(&m_arcs.back());
    to.arcsIn.push_back(&m_arcs.back());
    return true;
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
    if (isEmpty())
        return false;
    generateArcsInArcsOut();

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
            //for (auto& p: m_places) // FIXME !!!
            //{
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
            //}
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------------------------
// Quick and dirty algorithm.
void PetriNet::toCanonicalForm(PetriNet& canonic)
{
    // Copy the whole net
    canonic = *this;

    // Explode Places with more than one tokens and create as many as Places
    // holding a single token. Redo arcs.
    {
        std::deque<Place>& places = canonic.places();
        size_t i = places.size();
        while (i--)
        {
            Place& p = places[i];
            if (p.tokens > 1u)
            {
                // from: Transition
                Node* from = &(p.arcsIn[0]->from);
                float duration = p.arcsIn[0]->duration;
                size_t tokens = p.tokens - 1u;
                canonic.removeArc(*from, p);
                while (tokens--)
                {
                    Node& tmp1 = canonic.addPlace(10.0f, 10.0f, 1u);
                    canonic.addArc(*from, tmp1, 0.0f);
                    Node& tmp2 = canonic.addTransition(20.0f, 20.0f);
                    canonic.addArc(tmp1, tmp2, 0.0f);

                    from = &tmp2;
                    p.tokens--;

                    if (p.tokens == 1u)
                    {
                        canonic.addArc(tmp2, p, duration);
                    }
                }
            }
        }
    }

    // Manage Places with one token that inputs or outputs
    canonic.generateArcsInArcsOut(/*arcs: true*/);
    std::deque<Place>& places = canonic.places();
    size_t i = places.size();
    while (i--)
    {
        Place& p = places[i];
        if (p.tokens == 1u)
        {
            // Inputs
            Node* from = &(p.arcsIn[0]->from);
            if (reinterpret_cast<Transition*>(from)->isInput())
            {
                float duration = p.arcsIn[0]->duration;
                canonic.removeArc(*from, p);

                Node& tmp1 = canonic.addPlace(50.0f, 50.0f, 0u);
                Node& tmp2 = canonic.addTransition(60.0f, 60.0f);
                canonic.addArc(*from, tmp1, 0.0f);
                canonic.addArc(tmp1, tmp2, duration);
                canonic.addArc(tmp2, p, 0.0f);
            }

            // Outputs
            Node* to = &(p.arcsOut[0]->to);
            if (reinterpret_cast<Transition*>(to)->isOutput())
            {
                canonic.removeArc(p, *to);

                Node& tmp1 = canonic.addTransition(60.0f, 60.0f);
                Node& tmp2 = canonic.addPlace(50.0f, 50.0f, 0u);
                canonic.addArc(p, tmp1, 0.0f);
                canonic.addArc(tmp1, tmp2, 0.0f);
                canonic.addArc(tmp2, *to, 0.0f);
            }
        }
    }
}

//------------------------------------------------------------------------------
bool PetriNet::toAdjacencyMatrices(SparseMatrix& N, SparseMatrix&T)
{
    generateArcsInArcsOut(/*arcs: true*/);
    size_t const nnodes = m_transitions.size();

    T.clear(); T.reserve(m_places.size()); T.dim(nnodes, nnodes);
    N.clear(); N.reserve(m_places.size()); N.dim(nnodes, nnodes);

    for (auto& p: m_places)
    {
        // Since we are sure this Petri net is an event graph: places have a
        // single input arc and a single output arc. We can merge the place and
        // its arcs into a single arc.
        if (p.arcsIn.size() != 1u)
            return false;
        if (p.arcsIn[0]->from.type != Node::Type::Transition)
            return false;
        if (p.arcsOut.size() != 1u)
            return false;
        if (p.arcsOut[0]->to.type != Node::Type::Transition)
            return false;

        Transition& from = *reinterpret_cast<Transition*>(&(p.arcsIn[0]->from));
        Transition& to = *reinterpret_cast<Transition*>(&(p.arcsOut[0]->to));

        // Note origin and destination are inverted because we use the following
        // matrix product convension: M * x where x is a column vector.
        T.add(to.id, from.id, p.arcsIn[0]->duration);
        N.add(to.id, from.id, float(p.tokens));
    }

    return true;
}

//------------------------------------------------------------------------------
void PetriNet::toSysLin(SparseMatrix& D, SparseMatrix& A, SparseMatrix& B, SparseMatrix& C,
                        size_t const nb_inputs, size_t const nb_states, size_t const nb_outputs)
{
    D.clear(); A.clear(); B.clear(); C.clear();
    D.dim(nb_states, nb_states);
    A.dim(nb_states, nb_states);
    B.dim(nb_inputs, nb_inputs);
    C.dim(nb_outputs, nb_outputs);

    for (auto& arc: m_arcs)
    {
        if (arc.from.type == Node::Type::Place)
            continue;

        Transition& t = *reinterpret_cast<Transition*>(&(arc.from));
        if (t.isInput())
        {
            // System inputs: B U(n)
            B.add(t.index, t.index, float(arc.duration));
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
                        A.add(td.index, t.index, arc.duration);
                    }
                    else
                    {
                        D.add(td.index, t.index, arc.duration);
                    }
                }
                else if (td.isOutput())
                {
                    // System outputs: Y(n) = C X(n)
                    C.add(t.index, t.index, arc.duration);
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
bool PetriNet::toSysLin(SparseMatrix& D, SparseMatrix& A, SparseMatrix& B, SparseMatrix& C)
{
    // Only Petri net with places having a single input and output arcs are
    // allowed.
    if (!isEventGraph())
        return false;

    // Duplicate the Petri net since we potentially modify it to transform it to
    // its canonical form.
    PetriNet canonical;
    toCanonicalForm(canonical);

    // Count the number of inputs, outputs and states for creating matrices.
    size_t nb_states = 0u;
    size_t nb_inputs = 0u;
    size_t nb_outputs = 0u;

    for (auto& t: canonical.transitions())
    {
        if (t.isInput())
        {
            t.index = nb_inputs++;
        }

        if (t.isState())
        {
            t.index = nb_states++;
        }

        if (t.isOutput())
        {
            t.index = nb_outputs++;
        }
    }

    canonical.toSysLin(D, A, B, C, nb_inputs, nb_states, nb_outputs);
    return true;
}

//------------------------------------------------------------------------------
//! \note PetriNet shall be an event graph and to a canonical form (each places
//! have at most one token and no token at places in inputs or outputs). No
//! checks are performed and shall be done by the external caller.
//------------------------------------------------------------------------------
bool PetriNet::exportToJulia(std::string const& filename)
{
    // Only Petri net with places having a single input and output arcs are
    // allowed.
    if (!isEventGraph())
        return false;

    // TODO quick test check if we have to do the canonical form, this can avoid
    // duplicating the Petri net
    // if (!cannonical) {

    // Duplicate the Petri net since we potentially modify it to transform it to
    // its canonical form.
    PetriNet canonical;
    toCanonicalForm(canonical);

    // TODO
    // } else { return ::exportToJulia(*this, filename); }

    // Open the file
    std::ofstream file(filename);
    if (!file)
    {
        std::cerr << "Failed to export the Petri net to '" << filename
                  << "'. Reason was " << strerror(errno) << std::endl;
        return false;
    }

    // Generate the Julia header
    file << "# This file has been generated" << std::endl << std::endl;
    file << "using MaxPlus, SparseArrays" << std::endl << std::endl;

    // Count the number of inputs, outputs and states for creating matrices.
    size_t nb_states = 0u;
    size_t nb_inputs = 0u;
    size_t nb_outputs = 0u;

    file << "## Petri Transitions:" << std::endl;

    // Show and count system inputs
    for (auto& t: canonical.transitions())
    {
        if (t.isInput())
        {
            t.index = nb_inputs++;
            file << "# " << t.key << ": input (U"
                 << nb_inputs << ")" << std::endl;
        }
    }

    // Show and count system states
    for (auto& t: canonical.transitions())
    {
        if (t.isState())
        {
            t.index = nb_states++;
            file << "# " << t.key << ": state (X"
                 << nb_states << ")" << std::endl;
        }
    }

    // Show and count system outputs
    for (auto& t: canonical.transitions())
    {
        if (t.isOutput())
        {
            t.index = nb_outputs++;
            file << "# " << t.key << ": output (Y" << nb_outputs
                 << ")" << std::endl;
        }
    }

    // Graph representation. Since an event graph have all its places with a
    // single input arc and and single output arc. We can merge places and its
    // arcs into a single arc (still directing Transitions). Therefore we obtain
    // a graph holding two information: tokens and durations. Since a graph can
    // be represented by an adjacency matrix, here, we genereate two adjacency
    // matrices: one for tokens and one for durations.
    file << std::endl;
    file << "## Timed graph event depict as two graph adjacency matrices:" << std::endl;
    file << "# Nodes are Transitions." << std::endl;
    file << "# Arcs are Places and therefore have tokens and durations" << std::endl;
    SparseMatrix N; SparseMatrix T;
    bool res = canonical.toAdjacencyMatrices(N, T); assert(res == true);
    for (auto& p: canonical.places())
    {
        Transition& from = *reinterpret_cast<Transition*>(&(p.arcsIn[0]->from));
        Transition& to = *reinterpret_cast<Transition*>(&(p.arcsOut[0]->to));

        file << "# Arc " << p.key << ": " << from.key << " -> " << to.key
             << " (Duration: " << p.arcsIn[0]->duration
             << ", Tokens: " << p.tokens << ")" << std::endl;
    }
    size_t const nnodes = canonical.transitions().size();
    file << "N = sparse(" << N << ", " << nnodes << ", " << nnodes << ") # Tokens" << std::endl;
    file << "T = sparse(" << T << ", " << nnodes << ", " << nnodes << ") # Durations" << std::endl;

    // Show the event graph to its Max-Plus counter and dater form
    file << std::endl;
    file << this->showCounterForm().str();
    file << std::endl;
    file << this->showDaterForm().str();

    // Compute the syslin as Julia code using the Max-Plus package
    // X(n) = D X(n) ⨁ A X(n-1) ⨁ B U(n)
    // Y(n) = C X(n)
    SparseMatrix D; SparseMatrix A; SparseMatrix B; SparseMatrix C;
    canonical.toSysLin(D, A, B, C, nb_inputs, nb_states, nb_outputs);

    file << std::endl;
    file << "## Max-Plus implicit linear dynamic system of the dater form:" << std::endl;
    file << "# X(n) = D X(n) ⨁ A X(n-1) ⨁ B U(n)" << std::endl;
    file << "# Y(n) = C X(n)" << std::endl;
    file << "D = sparse(" << D << ", " << nb_states << ", " << nb_states << ") # States without tokens" << std::endl;
    file << "A = sparse(" << A << ", " << nb_states << ", " << nb_states << ") # States with 1 token" << std::endl;
    file << "B = sparse(" << B << ", " << nb_inputs << ", " << nb_inputs << ") # Inputs" << std::endl;
    file << "C = sparse(" << C << ", " << nb_outputs << ", " << nb_outputs << ") # Outputs" << std::endl;
    file << "S = MPSysLin(A, B, C, D)" << std::endl;

    // Semi-Howard
    file << std::endl;
    file << "# TODO" << std::endl;
    file << "l,v = semihoward(S.D, S.A)" << std::endl;

    return true;
}

//------------------------------------------------------------------------------
std::stringstream PetriNet::showCounterForm(std::string const& comment) const
{
    std::stringstream ss;

    ss << comment << "Timed event graph represented as its counter form:" << std::endl;
    for (auto const& t: this->transitions())
    {
        if (t.arcsIn.size() == 0u)
            continue;

        ss << comment << t.key << "(t) = min(";
        std::string separator1;
        for (auto const& ai: t.arcsIn)
        {
            ss << separator1;
            ss << ai->tokensIn() << " + ";
            std::string separator2;
            for (auto const& ao: ai->from.arcsIn)
            {
                ss << separator2;
                ss << ao->from.key << "(t - " << ao->duration << ")";
                separator2 = ", ";
            }
            separator1 = ", ";
        }
        ss << ");" << std::endl;
    }

    return ss;
}

//------------------------------------------------------------------------------
std::stringstream PetriNet::showDaterForm(std::string const& comment) const
{
    std::stringstream ss;

    ss << comment << "Timed event graph represented as its dater form:" << std::endl;
    for (auto const& t: this->transitions())
    {
        if (t.arcsIn.size() == 0u)
            continue;

        ss << comment << t.key << "(n) = max(";
        std::string separator1;
        for (auto const& ai: t.arcsIn)
        {
            ss << separator1;
            std::string separator2;
            for (auto const& ao: ai->from.arcsIn)
            {
                ss << separator2;
                ss << ao->duration << " + " << ao->from.key
                   << "(n - " << ai->tokensIn() << ")";
                separator2 = ", ";
            }
            separator1 = ", ";
        }
        ss << ");" << std::endl;
    }

    return ss;
}

//------------------------------------------------------------------------------
bool PetriNet::showCriticalCycle()
{
    if (!isEventGraph())
        return false;

    // Reserve memory
    size_t const nnodes = m_transitions.size();
    size_t const narcs = m_places.size();
    std::vector<double> N; N.reserve(narcs);
    std::vector<double> T; T.reserve(narcs);
    std::vector<int> IJ; // FIXME should be std::vector<size_t> but Howard wants int*

    IJ.reserve(2u * narcs);
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

        IJ.push_back(int(to.id)); // Transposed is needed
        IJ.push_back(int(from.id));
        T.push_back(p.arcsIn[0]->duration);
        N.push_back(double(p.tokens));
    }

    std::vector<double> V(nnodes); // bias
    std::vector<double> chi(nnodes); // cycle time vector
    std::vector<int> policy(nnodes); // optimal policy
    int ncomponents; //
    int niterations; // nb of iteration of the algorithm
    int verbosemode = 1;
    int res = Semi_Howard(IJ.data(), T.data(), N.data(),
                          int(nnodes), int(narcs),
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
            for (auto const& it: m_transitions[size_t(from)].arcsOut)
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
        c = char(::toupper(int(c)));
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
    Splitter s(filename, " \",");

    if (!s)
    {
        std::cerr << "Failed opening '" << filename << "'. Reason was '"
                  << strerror(errno) << "'" << std::endl;
        return false;
    }

    if (s.split() != "{")
    {
        std::cerr << "Failed loading " << filename
                  << ". Token { missing. Bad JSON file" << std::endl;
        return false;
    }

    reset();

    while (s)
    {
        s.split();
        if ((s.str() == "places") && (s.split() == ":") && (s.split() == "["))
        {
            while (s.split() != "]")
            {
                int id = convert_to<int>(s.str().c_str() + 1u);
                float x = convert_to<float>(s.split());
                float y = convert_to<float>(s.split());
                int tokens = convert_to<int>(s.split());
                if ((id < 0) || (tokens < 0))
                {
                    std::cerr << "Failed loading " << filename
                              << ". Unique identifiers and tokens shall be > 0"
                              << std::endl;
                    return false;
                }
                addPlace(size_t(id), x, y, size_t(tokens));
            }
        }
        else if ((s.str() == "transitions") && (s.split() == ":") && (s.split() == "["))
        {
            while (s.split() != "]")
            {
                int id = convert_to<int>(s.str().c_str() + 1u);
                float x = convert_to<float>(s.split());
                float y = convert_to<float>(s.split());
                int angle = convert_to<int>(s.split());
                if (id < 0)
                {
                    std::cerr << "Failed loading " << filename
                              << ". Unique identifiers shall be > 0"
                              << std::endl;
                    return false;
                }
                addTransition(size_t(id), x, y, angle);
            }
        }
        else if ((s.str() == "arcs") && (s.split() == ":") && (s.split() == "["))
        {
            while (s.split() != "]")
            {
                Node* from = findNode(s.str());
                if (!from)
                {
                    std::cerr << "Failed loading " << filename
                              << ". Origin node " << s.str()
                              << " not found" << std::endl;
                    return false;
                }

                Node* to = findNode(s.split());
                if (!to)
                {
                    std::cerr << "Failed loading " << filename
                              << ". Destination node " << s.str()
                              << " not found" << std::endl;
                    return false;
                }

                float duration = stof(s.split());
                if (duration < 0.0f)
                {
                    std::cout << "Failed loading " << filename
                              << ". Duration " << duration
                              << " shall be > 0" << std::endl;
                    return false;
                }
                if (!addArc(*from, *to, duration))
                {
                    std::cerr << "Failed loading " << filename
                              << ". Arc " << from->key << " -> " << to->key
                              << " is badly formed" << std::endl;
                    return false;
                }
            }
        }
        else if (s.str() == "}")
        {
            // End of the file
        }
        else if (s.str() == "[]")
        {
            // Do nothing
        }
        else if (s.str() != "")
        {
            std::cerr << "Failed loading " << filename
                      << ". Key " << s.str() << " is not a valid token"
                      << std::endl;
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
bool PetriNet::removeArc(Arc const& a)
{
    size_t i = m_arcs.size();
    while (i--)
    {
        if ((m_arcs[i].from == a.from) && (m_arcs[i].to == a.to))
        {
            // Found the undesired arc: make the latest element take its
            // location in the container.
            m_arcs[i] = m_arcs[m_arcs.size() - 1u];
            m_arcs.pop_back();
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
bool PetriNet::removeArc(Node& from, Node& to)
{
    return removeArc(Arc(from, to));
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
        i = m_places.size();
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
                assert(m_next_place_id >= 1u);
                m_next_place_id -= 1u;

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
        i = m_transitions.size();
        while (i--)
        {
            if (m_transitions[i].id == node.id)
            {
                Transition& ti = m_transitions[i];
                Transition& te = m_transitions[m_transitions.size() - 1u];
                m_transitions[i] = Transition(ti.id, te.x, te.y, te.angle);
                assert(m_next_transition_id >= 1u);
                m_next_transition_id -= 1u;

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
