//=====================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2022 Quentin Quadrat <lecrapouille@gmail.com>
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
#include <ctype.h>

//------------------------------------------------------------------------------
// Default net configuration: timed petri net. To change the type of nets, call
// PetriNet::changeTypeOfNet(PetriNet::Type const)
size_t Settings::maxTokens = std::numeric_limits<size_t>::max();
Settings::Fire Settings::firing = Settings::Fire::OneByOne;

//------------------------------------------------------------------------------
bool Transition::isEnabled() const
{
    if (arcsIn.size() == 0u)
        return false;

    for (auto& a: arcsIn)
    {
        if (a->tokensIn() == 0u)
            return false;
    }

    return true;
}

//------------------------------------------------------------------------------
size_t Transition::howManyTokensCanBurnt() const
{
    if ((arcsIn.size() == 0u) || (receptivity == false))
        return 0u;

    size_t burnt = static_cast<size_t>(-1);
    for (auto& a: arcsIn)
    {
        const size_t tokens = a->tokensIn();
        if (tokens == 0u)
            return 0u;

        if (tokens < burnt)
            burnt = tokens;
    }
    return burnt;
}

//------------------------------------------------------------------------------
PetriNet& PetriNet::operator=(PetriNet const& other)
{
    if (this != &other)
    {
        m_type = other.m_type;
        m_places = other.m_places;
        m_transitions = other.m_transitions;
        m_marks = other.m_marks;
        m_next_place_id = other.m_next_place_id;
        m_next_transition_id = other.m_next_transition_id;

        // We have to redo references
        m_arcs.clear();
        for (auto const& it: other.m_arcs)
        {
            Node& from = (it.from.type == Node::Type::Place)
                         ? reinterpret_cast<Node&>(m_places[it.from.id])
                         : reinterpret_cast<Node&>(m_transitions[it.from.id]);
            Node& to = (it.to.type == Node::Type::Place)
                       ? reinterpret_cast<Node&>(m_places[it.to.id])
                       : reinterpret_cast<Node&>(m_transitions[it.to.id]);
            m_arcs.push_back(Arc(from, to, it.duration));
        }
        generateArcsInArcsOut();
    }

    return *this;
}

//------------------------------------------------------------------------------
void PetriNet::reset()
{
    m_places.clear();
    m_transitions.clear();
    m_shuffled_transitions.clear();
    m_arcs.clear();
    m_marks.clear();
    m_next_place_id = 0u;
    m_next_transition_id = 0u;
    modified = false;
}

//------------------------------------------------------------------------------
void PetriNet::changeTypeOfNet(PetriNet::Type const mode)
{
    m_type = mode;
    switch (mode)
    {
    case PetriNet::Type::GRAFCET:
        Settings::maxTokens = 1u;
        Settings::firing = Settings::Fire::OneByOne;
        break;
    case PetriNet::Type::Petri:
        Settings::maxTokens = std::numeric_limits<size_t>::max();
        Settings::firing = Settings::Fire::MaxPossible;
        break;
    case PetriNet::Type::TimedPetri:
        Settings::maxTokens = std::numeric_limits<size_t>::max();
        Settings::firing = Settings::Fire::OneByOne;
        break;
        // TODO: Missing type
        //case PetriNet::Type::TimedGraphEvent:
        //case PetriNet::Type::GraphEvent:
        // configurate the editor to draw directly doc/Graph01.png
        // break;
    default:
        assert(false && "Undefined Petri behavior");
        break;
    }
}

//------------------------------------------------------------------------------
void PetriNet::resetReceptivies()
{
    if (m_type == PetriNet::Type::Petri)
    {
        for (auto& transition: m_transitions)
        {
            transition.receptivity = false;
        }
    }
    else
    {
        for (auto& transition: m_transitions)
        {
            transition.receptivity = true;
        }
    }
}

//------------------------------------------------------------------------------
void PetriNet::backupMarks()
{
    m_marks.resize(m_places.size());
    for (auto& place: m_places)
    {
        m_marks[place.id] = place.tokens;
    }
}

//------------------------------------------------------------------------------
bool PetriNet::setMarks(std::vector<size_t> const& marks)
{
    if (m_places.size() != marks.size())
    {
        m_message.str("");
        m_message << current_time()
                  << "the container dimension holding marks does not match the number of places"
                  << std::endl;
        return false;
    }

    size_t i = m_marks.size();
    while (i--)
    {
        m_places[i].tokens = m_marks[i];
    }

    return true;
}

//------------------------------------------------------------------------------
Place& PetriNet::addPlace(float const x, float const y, size_t const tokens)
{
    modified = true;
    m_places.push_back(Place(m_next_place_id++, "", x, y, tokens));
    return m_places.back();
}

//------------------------------------------------------------------------------
Place& PetriNet::addPlace(size_t const id, std::string const& caption, float const x,
                          float const y, size_t const tokens)
{
    modified = true;
    m_places.push_back(Place(id, caption, x, y, tokens));
    if (id + 1u > m_next_place_id)
        m_next_place_id = id + 1u;
    return m_places.back();
}

//------------------------------------------------------------------------------
Transition& PetriNet::addTransition(float const x, float const y)
{
    modified = true;
    m_transitions.push_back(
        Transition(m_next_transition_id++, "", x, y, 0u,
                   (m_type == PetriNet::Type::TimedPetri) ? true : false));
    return m_transitions.back();
}

//------------------------------------------------------------------------------
Transition& PetriNet::addTransition(size_t const id, std::string const& caption,
                                    float const x, float const y, int const angle)
{
    modified = true;
    m_transitions.push_back(
        Transition(id, caption, x, y, angle,
                   (m_type == PetriNet::Type::TimedPetri) ? true : false));
    if (id + 1u > m_next_transition_id)
        m_next_transition_id = id + 1u;
    return m_transitions.back();
}

//------------------------------------------------------------------------------
std::vector<Transition*> const& PetriNet::shuffle_transitions(bool const reset)
{
    static std::random_device rd;
    static std::mt19937 g(rd());

    if (reset)
    {
        // Avoid useless copy at each iteration of the simulation. Do it
        // once at the begining of the simulation.
        m_shuffled_transitions.clear();
        m_shuffled_transitions.reserve(m_transitions.size());
        for (auto& trans: m_transitions)
            m_shuffled_transitions.push_back(&trans);
    }
    std::shuffle(m_shuffled_transitions.begin(),
                 m_shuffled_transitions.end(), g);
    return m_shuffled_transitions;
}

//------------------------------------------------------------------------------
bool PetriNet::addArc(Node& from, Node& to, float const duration)
{
    if (from.type == to.type)
    {
        m_message.str("");
        m_message << "Failed adding arc " << from.key
                  << " --> " << to.key
                  << ": nodes type shall not be the same"
                  << std::endl;
        return false;
    }

    if (findArc(from, to) != nullptr)
    {
        m_message.str("");
        m_message << "Failed adding arc " << from.key
                  << " --> " << to.key
                  << ": Arc already exist"
                  << std::endl;
        return false;
    }

    if (findNode(from.key) == nullptr)
    {
        m_message << "Failed adding arc " << from.key
                  << " --> " << to.key
                  << ": The node " << from.key
                  << " does not exist"
                  << std::endl;
        return false;
    }

    if (findNode(to.key) == nullptr)
    {
        m_message << "Failed adding arc " << from.key
                  << " --> " << to.key
                  << ": The node " << to.key
                  << " does not exist"
                  << std::endl;
        return false;
    }

    m_arcs.push_back(Arc(from, to, duration));
    from.arcsOut.push_back(&m_arcs.back());
    to.arcsIn.push_back(&m_arcs.back());
    modified = true;
    return true;
}

//------------------------------------------------------------------------------
Arc* PetriNet::findArc(Node const& from, Node const& to)
{
    for (auto& it: m_arcs)
    {
        if ((it.from == from) && (it.to == to))
            return &it;
    }
    return nullptr;
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
bool PetriNet::isEventGraph(std::vector<Arc*>& erroneous_arcs)
{
    erroneous_arcs.clear();
    if (isEmpty())
    {
        m_message.str("");
        m_message << "Empty Petri net is not an event graph" << std::endl;
        return false;
    }
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
            m_message.str("");
            m_message << "The Petri net is not an event graph. Because:"
                      << std::endl;

            if (p.arcsOut.size() != 1u)
            {
                m_message << "  " << p.key
                          << ((p.arcsOut.size() > 1u)
                              ? " has more than one output arc:"
                              : " has no output arc");
                for (auto const& a: p.arcsOut)
                {
                    erroneous_arcs.push_back(a);
                    m_message << " " << a->to.key;
                }
                m_message << std::endl;
            }

            if (p.arcsIn.size() != 1u)
            {
                m_message << "  " << p.key
                          << ((p.arcsIn.size() > 1u)
                              ? " has more than one input arc:"
                              : " has no input arc");
                for (auto const& a: p.arcsIn)
                {
                    erroneous_arcs.push_back(a);
                    m_message << " " << a->from.key;
                }
                m_message << std::endl;
            }

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
                    canonic.addArc(*from, tmp1);
                    Node& tmp2 = canonic.addTransition(20.0f, 20.0f);
                    canonic.addArc(tmp1, tmp2);

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
                canonic.addArc(*from, tmp1);
                canonic.addArc(tmp1, tmp2, duration);
                canonic.addArc(tmp2, p);
            }

            // Outputs
            Node* to = &(p.arcsOut[0]->to);
            if (reinterpret_cast<Transition*>(to)->isOutput())
            {
                canonic.removeArc(p, *to);

                Node& tmp1 = canonic.addTransition(60.0f, 60.0f);
                Node& tmp2 = canonic.addPlace(50.0f, 50.0f, 0u);
                canonic.addArc(p, tmp1);
                canonic.addArc(tmp1, tmp2);
                canonic.addArc(tmp2, *to);
            }
        }
    }
}

//------------------------------------------------------------------------------
bool PetriNet::toAdjacencyMatrices(SparseMatrix& N, SparseMatrix&T)
{
    generateArcsInArcsOut(/*arcs: true*/);
    size_t const nnodes = m_transitions.size();

    T.clear(); T.dim(nnodes, nnodes);
    N.clear(); N.dim(nnodes, nnodes);

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
    if (!isEventGraph(m_result_arcs))
        return false;

    // Duplicate the Petri net since we potentially modify it to transform it to
    // its canonical form.
    PetriNet canonical(m_type);
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
    if (!isEventGraph(m_result_arcs))
        return false;

    // TODO quick test check if we have to do the canonical form, this can avoid
    // duplicating the Petri net
    // if (!cannonical) {

    // Duplicate the Petri net since we potentially modify it to transform it to
    // its canonical form.
    PetriNet canonical(type());
    toCanonicalForm(canonical);

    // TODO
    // } else { return ::exportToJulia(*this, filename); }

    // Open the file
    std::ofstream file(filename);
    if (!file)
    {
        m_message.str("");
        m_message << "Failed to export the Petri net to '" << filename
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
bool PetriNet::findCriticalCycle(std::vector<Arc*>& result)
{
    if (!isEventGraph(result))
        return false;
    result.clear();

    // Number of nodes and number of arcs
    size_t const nnodes = m_transitions.size();
    size_t const narcs = m_places.size();

    // Reserve memory
    std::vector<double> T; T.reserve(narcs); // Timings
    std::vector<double> N; N.reserve(narcs); // Tokens (delays)
    // Arcs of the graph: {(source node, destination node), ... }
    // FIXME should be std::vector<size_t> but Howard wants int*
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

        //std::cout << "# Arc " << p.key << ": " << from.key << " -> " << to.key
        //          << " (Duration: " << p.arcsIn[0]->duration << ", Tokens: "
        //          << p.tokens << ")" << std::endl;

        IJ.push_back(int(to.id)); // Transposed is needed
        IJ.push_back(int(from.id));
        T.push_back(p.arcsIn[0]->duration);
        N.push_back(double(p.tokens));
    }

    std::vector<double> V(nnodes); // bias
    std::vector<double> chi(nnodes); // cycle time vector
    std::vector<int> policy(nnodes); // optimal policy
    int ncomponents; // Number of connected components of the optimal policy
    int niterations; // Number of iteration needed by the algorithm
    int verbosemode = 0; // No verbose
    int res = Semi_Howard(IJ.data(), T.data(), N.data(),
                          int(nnodes), int(narcs),
                          chi.data(), V.data(), policy.data(),
                          &niterations, &ncomponents, verbosemode);

    std::cout << "V:";
    for (auto const& it: V)
    {
        std::cout << ' ' << it;
    }
    std::cout << std::endl << "CHI:";
    for (auto const& it: chi)
    {
        std::cout << ' ' << it;
    }
    std::cout << std::endl << "Policies: " << ncomponents << std::endl;
    if (ncomponents > 0)
    {
        std::cout << "Policy:";
        for (auto const& it: policy)
        {
            std::cout << ' ' << it;
        }
        std::cout << std::endl;
    }

    if (ncomponents > 0)
    {
        size_t to = 0u;
        result.clear();
        result.reserve(nnodes);
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
                    result.push_back(it);
                    result.push_back(it->to.arcsOut[0]);
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
bool PetriNet::exportToLaTeX(std::string const& filename, float const scale_x,
                             float const scale_y)
{
    std::ofstream file(filename);
    if (!file)
    {
        m_message.str("");
        m_message << "Failed to export the Petri net to '" << filename
                  << "'. Reason was " << strerror(errno) << std::endl;
        return false;
    }

    file << R"PN(\documentclass[border = 0.2cm]{standalone}
\usepackage{tikz}
\usetikzlibrary{petri,positioning}
\begin{document}
\begin{tikzpicture}
)PN";

    // Places
    file << std::endl << "% Places" << std::endl;
    for (auto const& p: m_places)
    {
        file << "\\node[place, "
             << "label=above:$" << p.caption << "$, "
             << "fill=blue!25, "
             << "draw=blue!75, "
             << "tokens=" << p.tokens << "] "
             << "(" << p.key << ") at (" << int(p.x * scale_x)
             << ", " << int(-p.y * scale_y) << ") {};"
             << std::endl;
    }

    // Transitions
    file << std::endl << "% Transitions" << std::endl;
    for (auto const& t: m_transitions)
    {
        std::string color = (t.canFire() ? "green" : "red");

        file << "\\node[transition, "
             << "label=above:$" << t.caption << "$, "
             << "fill=" << color << "!25, "
             << "draw=" << color << "!75] "
             << "(" << t.key << ") at (" << int(t.x * scale_x)
             << ", " << int(-t.y * scale_y) << ") {};"
             << std::endl;
    }

    // Arcs
    file << std::endl << "% Arcs" << std::endl;
    for (auto const& a: m_arcs)
    {
        if (a.from.type == Node::Type::Transition)
        {
            std::stringstream duration;
            duration << std::fixed << std::setprecision(2) << a.duration;
            file << "\\draw[-latex, thick] "
                 << "(" << a.from.key << ") -- "
                 << "node[midway, above right] "
                 << "{" << duration.str() << "} "
                 << "(" << a.to.key << ");"
                 << std::endl;
        }
        else
        {
            file << "\\draw[-latex, thick] "
                 << "(" << a.from.key << ") -- " << "(" << a.to.key << ");"
                 << std::endl;
        }
    }

    file << R"PN(
\end{tikzpicture}
\end{document}
)PN";

    return true;
}

//------------------------------------------------------------------------------
bool PetriNet::exportToGraphviz(std::string const& filename)
{
    std::ofstream file(filename);
    if (!file)
    {
        m_message.str("");
        m_message << "Failed to export the Petri net to '" << filename
                  << "'. Reason was " << strerror(errno) << std::endl;
        return false;
    }

    file << "digraph G {" << std::endl;

    // Places
    file << "node [shape=circle, color=blue]" << std::endl;
    for (auto const& p: m_places)
    {
        file << "  " << p.key << " [label=\"" << p.caption;
        if (p.tokens > 0u)
        {
            file << "\\n" << p.tokens << "&bull;";
        }
        file << "\"];" << std::endl;
    }

    // Transitions
    file << "node [shape=box, color=red]" << std::endl;
    for (auto const& t: m_transitions)
    {
        if (t.canFire())
        {
            file << "  " << t.key << " [label=\""
                 << t.caption << "\", color=green];"
                 << std::endl;
        }
        else
        {
            file << "  " << t.key << " [label=\""
                 << t.caption << "\"];"
                 << std::endl;
        }
    }

    // Arcs
    file << "edge [style=\"\"]" << std::endl;
    for (auto const& a: m_arcs)
    {
        file << "  " << a.from.key << " -> " << a.to.key;
        if (a.from.type == Node::Type::Transition)
        {
            file << " [label=\"" << a.duration << "\"]";
        }
        file << ";" << std::endl;
    }

    file << "}" << std::endl;
    return true;
}

//------------------------------------------------------------------------------
//! \brief Write int32_t as little endian
template<class T> void write_int32(std::ofstream& file, T const val)
{
    int32_t d = int32_t(val);

    file.put(char(d));
    file.put(char(d >> 8));
    file.put(char(d >> 16));
    file.put(char(d >> 24));
}

template<class T> void write_float32(std::ofstream& file, T const val)
{
    float d = float(val);

    file.write(reinterpret_cast<const char*>(&d), sizeof(float));
}

//------------------------------------------------------------------------------
bool PetriNet::exportToPNEditor(std::string const& filename)
{
    generateArcsInArcsOut();

    // .pns file: contains the logical contents of the petri net
    {
        std::string filename_pns(filename.substr(0, filename.find_last_of('.')) + ".pns");
        std::ofstream file(filename_pns, std::ios::out | std::ios::binary);
        if (!file)
        {
            m_message.str("");
            m_message << "Failed to export the Petri net to '" << filename_pns
                      << "'. Reason was " << strerror(errno) << std::endl;
            return false;
        }

        // Places
        write_int32(file, m_places.size());
        for (auto const& p: m_places)
        {
            write_int32(file, p.tokens);
        }

        // Transitions
        write_int32(file, m_transitions.size());
        for (auto const& t: m_transitions)
        {
            write_int32(file, t.arcsOut.size());
            for (auto const& a: t.arcsOut)
            {
                write_int32(file, a->to.id);
            }
            write_int32(file, t.arcsIn.size());
            for (auto const& a: t.arcsIn)
            {
                write_int32(file, a->from.id);
            }
        }
    }

    // .pnl file: describes the layout of the petri net
    {
        std::string filename_pnl(filename.substr(0, filename.find_last_of('.')) + ".pnl");
        std::ofstream file(filename_pnl, std::ios::out | std::ios::binary);
        if (!file)
        {
            m_message.str("");
            m_message << "Failed to export the Petri net to '" << filename_pnl
                      << "'. Reason was " << strerror(errno) << std::endl;
            return false;
        }

        for (auto const& t: m_transitions)
        {
            write_float32(file, t.x);
            write_float32(file, t.y);
        }

        for (auto const& p: m_places)
        {
            write_float32(file, p.x);
            write_float32(file, p.y);
        }
    }

    // .pnkp: list of names for all the transitions
    {
        std::string filename_pnkp(filename.substr(0, filename.find_last_of('.')) + ".pnkp");
        std::ofstream file(filename_pnkp);
        if (!file)
        {
            m_message.str("");
            m_message << "Failed to export the Petri net to '" << filename_pnkp
                      << "'. Reason was " << strerror(errno) << std::endl;
            return false;
        }

        for (auto const& p: m_places)
        {
            file << p.caption << std::endl;
        }
    }

    // .pnk: list of names for all the places
    {
        std::string filename_pnk(filename.substr(0, filename.find_last_of('.')) + ".pnk");
        std::ofstream file(filename_pnk);
        if (!file)
        {
            m_message.str("");
            m_message << "Failed to export the Petri net to '" << filename_pnk
                      << "'. Reason was " << strerror(errno) << std::endl;
            return false;
        }

        for (auto const& t: m_transitions)
        {
            file << t.caption << std::endl;
        }
    }

    return true;
}

//------------------------------------------------------------------------------
bool PetriNet::exportToCpp(std::string const& filename, std::string const& name)
{
    std::ofstream file(filename);
    if (!file)
    {
        m_message.str("");
        m_message << "Failed to export the Petri net to '" << filename
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
             << " // " << m_places[i].caption
             << std::endl;
    }

    file << R"PN(    }

    void update()
    {
)PN";

    file << "        // Read inputs" << std::endl << std::endl;

    file << "        // Update transitions" << std::endl;
    for (auto const& trans: m_transitions)
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

    file  << std::endl << "        // Update steps" << std::endl;
    for (auto const& p: m_places)
    {
        file << "        X[" << p.id << "] = X[" << p.id << "] &&";

        for (size_t a = 0; a < p.arcsOut.size(); ++a)
        {
            Arc& arc = *p.arcsOut[a];
            if (a > 0u) { file << " &&"; }
            file << " !T[" << arc.to.id << "]";
        }

        for (size_t a = 0; a < p.arcsIn.size(); ++a)
        {
            Arc& arc = *p.arcsIn[a];
            file << " || T[" << arc.from.id << "]";
        }

        file << ";" << std::endl;
    }

    file << std::endl;
    file << "        // Do actions of enabled steps" << std::endl;
    for (size_t p = 0u; p < m_places.size(); ++p)
    {
        file << "        if (X[" << p << "]) { P" << p << "(); }"
             << std::endl;
    }

    file << R"PN(    }

    void debug()
    {
       std::cout << "Steps:" << std::endl;
       for (size_t i = 0u; i < MAX_PLACES; ++i)
          std::cout << "  X[" << i << "] = " << X[i] << std::endl;

       std::cout << "Transitions:" << std::endl;
       for (size_t i = 0u; i < MAX_TRANSITIONS; ++i)
          std::cout << "  T[" << i << "] = " << T[i] << std::endl;
    }

private:

    //! \brief Fonction not generated to let the user initializing
    //! inputs (i.e. TTL gpio, ADC ...) and outputs (i.e. TTL gpio,
    //! PWM ...)
    void initIO();
)PN";

    for (auto const& t: m_transitions)
    {
        file << "    //! \\brief Transition " << t.id <<  ": " << t.caption << std::endl;
        file << "    //! \\return true if the transition is enabled." << std::endl;
        file << "    bool T" << t.id << "() const;" << std::endl;
    }

    for (auto const& p: m_places)
    {
        file << "    //! \\brief Actions for the step " << p.id << " (" << p.caption << ")" << std::endl;
        file << "    void P" << p.id << "();" << std::endl;
    }

    file << R"PN(
private:

)PN";

    file << "    static const size_t MAX_PLACES = " << m_places.size() << "u;"  << std::endl;
    file << "    static const size_t MAX_TRANSITIONS = " << m_transitions.size() << "u;" << std::endl;
    file << "    //static const size_t MAX_INPUTS = X;" << std::endl;
    file << "    //static const size_t MAX_OUTPUTS = Y;" << std::endl;
    file << "" << std::endl;
    file << "    //! \\brief Steps"  << std::endl;
    file << "    bool X[MAX_PLACES];" << std::endl;
    file << "    //! \\brief Transitions"  << std::endl;
    file << "    bool T[MAX_TRANSITIONS];" << std::endl;
    file << "};" << std::endl;
    file << "" << std::endl;
    file << "} // namespace " << name << std::endl;
    file << "#endif // GENERATED_GRAFCET_" << upper_name << "_HPP" << std::endl;

    return true;
}

//------------------------------------------------------------------------------
bool PetriNet::save(std::string const& filename)
{
    std::string separator;
    std::ofstream file(filename);

    if (isEmpty())
    {
        m_message.str("");
        m_message << "I'll not save empty net" << std::endl;
        return false;
    }

    if (!file)
    {
        m_message.str("");
        m_message << "Failed saving the Petri net in '" << filename
                  << "'. Reason was " << strerror(errno) << std::endl;
        return false;
    }

    file << "{\n  \"places\": [";
    for (auto const& p: m_places)
    {
        file << separator << "\n    " << '\"' << p.key << ',' << p.caption << ','
             << p.x << ',' << p.y << ',' << p.tokens << '\"';
        separator = ",";
    }
    file << "],\n  \"transitions\": [";
    separator = "";
    for (auto const& t: m_transitions)
    {
        file << separator << "\n    " << '\"' << t.key << ',' << t.caption << ','
             << t.x << ',' << t.y << ',' << t.angle << '\"';
        separator = ",";
    }
    file << "],\n  \"arcs\": [";
    separator = "";
    for (auto const& a: m_arcs)
    {
        file << separator << "\n    " << '\"' << a.from.key << ','
             << a.to.key << ',' << a.duration << '\"';
        separator = ",";
    }
    file << "]\n}";

    return true;
}

//------------------------------------------------------------------------------
//! \note this a quick and dirty but simple JSON parsing since I do not want to
//! depend on a huge library.
bool PetriNet::load(std::string const& filename)
{
    std::vector<std::string> words(5);

    Splitter s(filename);
    if (!s)
    {
        m_message.str("");
        m_message << "Failed opening '" << filename << "'. Reason was '"
                  << strerror(errno) << "'" << std::endl;
        return false;
    }

    // Expect '{' as first token
    if (s.split(" \t\n", " \t\n") != "{")
    {
        m_message.str("");
        m_message << "Failed loading " << filename
                  << ". Token { missing. Bad JSON file" << std::endl;
        return false;
    }

    reset();
    while (s)
    {
        // Split for tokens "places : [" or "transitions : [" or "arcs : ["
        std::string token(s.split(" \t\n\"", " \t\n\""));
        if ((token == "places") || (token == "transitions") || (token == "arcs"))
        {
            // Split and check the presence of tokens ": ["
            if ((s.split(" \t\n\"", " \t\n")[0] != ':') || (s.split(" \t\n", " ]\t\n\"")[0] != '['))
            {
                m_message.str("");
                m_message << "Failed parsing" << std::endl;
                return false;
            }

            // Parse Petri place "P0,Caption,146,250,1"
            if (token == "places")
            {
                while (s.split(" \t\n\"[", "\"")[0] != ']')
                {
                    if (s.str()[0] == ',')
                        continue ;
                    if (token2vector(s.str(), words) != 5u)
                    {
                        m_message.str("");
                        m_message << "Failed parsing Place" << std::endl;
                        return false;
                    }
                    addPlace(convert_to<size_t>(&words[0][1]),  // id
                             words[1],                      // caption
                             convert_to<float>(words[2]),   // x
                             convert_to<float>(words[3]),   // y
                             convert_to<size_t>(words[4])); // tokens
                }
            }
            // Parse Petri transition "T0,Caption,272,173,315"
            else if (token == "transitions")
            {
                while (s.split(" \t\n\"[", "\"")[0] != ']')
                {
                    if (s.str()[0] == ',')
                        continue ;
                    if (token2vector(s.str(), words) != 5u)
                    {
                        m_message.str("");
                        m_message << "Failed parsing Transition" << std::endl;
                        return false;
                    }
                    addTransition(convert_to<size_t>(&words[0][1]),  // id
                                  words[1],                      // caption
                                  convert_to<float>(words[2]),   // x
                                  convert_to<float>(words[3]),   // y
                                  convert_to<int>(words[4]));    // angle
                }
            }
            // Parse Petri arcs "P0,T0,nan"
            else
            {
                while (s.split(" \t\n\"[", "\"}")[0] != ']')
                {
                    if (s.str()[0] == ',')
                        continue ;
                    if (token2vector(s.str(), words) != 3u)
                    {
                        m_message.str("");
                        m_message << "Failed parsing Arc" << std::endl;
                        return false;
                    }
                    Node* from = findNode(words[0]);
                    if (!from)
                    {
                        m_message << "Failed loading " << filename
                                  << ". Origin node " << words[0]
                                  << " not found" << std::endl;
                        return false;
                    }

                    Node* to = findNode(words[1]);
                    if (!to)
                    {
                        m_message << "Failed loading " << filename
                                  << ". Destination node " << words[1]
                                  << " not found" << std::endl;
                        return false;
                    }

                    float duration = convert_to<float>(words[2]);
                    if (duration < 0.0f)
                    {
                        m_message.str("");
                        m_message << "Failed loading " << filename
                                  << ". Duration " << words[2]
                                  << " shall be > 0" << std::endl;
                        return false;
                    }
                    if (!addArc(*from, *to, duration))
                    {
                        m_message.str("");
                        m_message << "Failed loading " << filename
                                  << ". Arc " << from->key << " -> " << to->key
                                  << " is badly formed" << std::endl;
                        return false;
                    }
                }
            }
        }
        else if (s.str() == "}")
        {
            // End of the file
            return true;
        }
        else if (token != "")
        {
            m_message.str("");
            m_message << "Failed loading " << filename
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
        m_message.str("");
        return nullptr;
    }

    if (key[0] == 'T')
    {
        for (auto& t: m_transitions)
        {
            if (t.key == key)
                return &t;
        }
        m_message.str("");
        return nullptr;
    }

    m_message.str("");
    m_message << "Node key shall start with 'P' or 'T'" << std::endl;
    return nullptr;
}

//------------------------------------------------------------------------------
Transition* PetriNet::findTransition(size_t const id)
{
    for (auto& t: m_transitions)
    {
        if (t.id == id)
            return &t;
    }
    return nullptr;
}

//------------------------------------------------------------------------------
Place* PetriNet::findPlace(size_t const id)
{
    for (auto& p: m_places)
    {
        if (p.id == id)
            return &p;
    }
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
                if (pe.caption == pe.key)
                {
                    m_places[i] = Place(pi.id, pi.key, pe.x, pe.y, pe.tokens);
                }
                else
                {
                    m_places[i] = Place(pi.id, pe.caption, pe.x, pe.y, pe.tokens);
                }
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
                if (te.caption == te.key)
                {
                    m_transitions[i] = Transition(ti.id, ti.key, te.x, te.y, te.angle,
                                                  (m_type == PetriNet::Type::TimedPetri)
                                                  ? true : false);
                }
                else
                {
                    m_transitions[i] = Transition(ti.id, te.caption, te.x, te.y, te.angle,
                                                  (m_type == PetriNet::Type::TimedPetri)
                                                  ? true : false);
                }
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
