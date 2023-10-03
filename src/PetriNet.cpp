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

#include "PetriNet.hpp"
#include "utils/Howard.h"
#include "utils/Utils.hpp"
#include "utils/Theme.hpp"
#include "nlohmann/json.hpp"
#include <iomanip>
#include <iostream>
#include <fstream>
#include <cstring>
#include <ctype.h>
#include <limits>
#include <map>

#include "formats/ImportJSON.hpp"
#include "formats/ImportFlowshop.hpp"

#include "formats/ExportJSON.hpp"
#include "formats/ExportCoDeSys.hpp"
#include "formats/ExportDrawIO.hpp"
#include "formats/ExportGrafcetLaTeX.hpp"
#include "formats/ExportPetriLaTeX.hpp"
#include "formats/ExportSymfony.hpp"
#include "formats/ExportGrafcetCpp.hpp"
#include "formats/ExportGraphviz.hpp"
#include "formats/ExportJulia.hpp"
#include "formats/ExportPnEditor.hpp"

//------------------------------------------------------------------------------
// Default net configuration: timed petri net. To change the type of nets, call
// PetriNet::changeTypeOfNet(PetriNet::Type const)
size_t Settings::maxTokens = std::numeric_limits<size_t>::max();
Settings::Fire Settings::firing = Settings::Fire::OneByOne;

//------------------------------------------------------------------------------
// Used to control the behavior of operator<<.
bool SparseMatrix::display_for_julia = true;

//------------------------------------------------------------------------------
std::string PetriNet::to_str(PetriNet::Type const mode)
{
    switch (mode)
    {
    case PetriNet::Type::GRAFCET:
        return "GRAFCET";
    case PetriNet::Type::Petri:
        return "Petri net";
    case PetriNet::Type::TimedPetri:
        return "Timed Petri net";
    case PetriNet::Type::TimedGraphEvent:
        return "Timed graph event";
    default:
        return "Undefined type of net";
    }
}

//------------------------------------------------------------------------------
bool Transition::isEnabled() const
{
    // Transition source will always produce tokens.
    if (arcsIn.size() == 0u)
        return true;

    // To enabled this current transition, all its previous Places shall have at
    // least one token.
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
    // Transition source will fire one token iff the animated token transitioning
    // along the arcs has reached the Place (in this receptivity becomes true ...
    // FIXME this will conflict if we add code to the receptivity add && animation_done)
    if (arcsIn.size() == 0u)
        return size_t(receptivity != false);

    // The transition is false => it does not let burn tokens.
    if (receptivity == false)
        return 0u;

    // Iterate on all previous places to know how many tokens can be burned.
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
PetriNet::PetriNet(PetriNet::Type const mode)
{
    this->type(mode);
}

//------------------------------------------------------------------------------
PetriNet& PetriNet::operator=(PetriNet const& other)
{
    if (this != &other)
    {
        m_name = other.m_name;
        m_type = other.m_type;
        m_places = other.m_places;
        m_transitions = other.m_transitions;
        m_next_place_id = other.m_next_place_id;
        m_next_transition_id = other.m_next_transition_id;
        m_message.str("");
        modified = true;

        // For arcs: we have to redo references to nodes
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
void PetriNet::clear()
{
    m_name.clear();
    m_places.clear();
    m_transitions.clear();
    m_shuffled_transitions.clear();
    m_arcs.clear();
    m_next_place_id = 0u;
    m_next_transition_id = 0u;
    modified = true;
    m_message.str("");
}

//------------------------------------------------------------------------------
bool PetriNet::start(std::vector<size_t>& tokens)
{
    generateArcsInArcsOut();
    if (resetReceptivies())
    {
        shuffle_transitions(true);
        getTokens(tokens);
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
void PetriNet::end(std::vector<size_t> const& tokens)
{
    setTokens(tokens);
}

//------------------------------------------------------------------------------
bool PetriNet::type(PetriNet::Type const mode, std::vector<Arc*>& erroneous_arcs)
{
    if (m_type == mode)
        return true;

    switch (mode)
    {
    case PetriNet::Type::GRAFCET:
        Settings::maxTokens = 1u;
        Settings::firing = Settings::Fire::OneByOne;
        m_sensors.clear();
        for (auto& transition: m_transitions)
        {
            parse(transition);
        }
        break;
    case PetriNet::Type::Petri:
        Settings::maxTokens = std::numeric_limits<size_t>::max();
        Settings::firing = Settings::Fire::MaxPossible;
        break;
    case PetriNet::Type::TimedPetri:
        Settings::maxTokens = std::numeric_limits<size_t>::max();
        Settings::firing = Settings::Fire::OneByOne;
        break;
    case PetriNet::Type::TimedGraphEvent:
        // Check conditions of a well formed event graph
        if ((!isEmpty()) && (!isEventGraph(erroneous_arcs)))
            return false;
        Settings::maxTokens = std::numeric_limits<size_t>::max();
        Settings::firing = Settings::Fire::OneByOne;
        break;
    default:
        assert(false && "Undefined Petri behavior");
        break;
    }

    // Place this at the end because of possible return false.
    m_type = mode;
    return true;
}

//------------------------------------------------------------------------------
std::string PetriNet::parse(Transition& transition, bool force)
{
    std::string error;

    if (force || (transition.expression.expression == nullptr))
    {
        transition.expression.expression =
        Receptivity::Parser::parse(*this, transition.caption, error);
        transition.expression.valid = error.empty();
    }
    return error;
}

//------------------------------------------------------------------------------
bool PetriNet::resetReceptivies()
{
    bool res = true;

    if (m_type == PetriNet::Type::Petri)
    {
        for (auto& transition: m_transitions)
        {
            transition.receptivity = false;
        }
    }
    else if (m_type == PetriNet::Type::GRAFCET)
    {
        m_sensors.clear();
        for (auto& transition: m_transitions)
        {
            std::string err = parse(transition, true);
            if (!err.empty())
            {
                m_message.str("");
                m_message << err;
                res = false;
            }
        }
    }
    else
    {
        for (auto& transition: m_transitions)
        {
            transition.receptivity = true;
        }
    }

    return res;
}

//------------------------------------------------------------------------------
void PetriNet::getTokens(std::vector<size_t>& marks) const
{
    marks.resize(m_places.size());
    for (auto& place: m_places)
    {
        marks[place.id] = place.tokens;
    }
}

//------------------------------------------------------------------------------
bool PetriNet::setTokens(std::vector<size_t> const& marks)
{
    if (m_places.size() != marks.size())
    {
        m_message.str("");
        m_message << current_time()
                  << "the container dimension holding marks does not match the number of places"
                  << std::endl;
        return false;
    }

    size_t i = marks.size();
    while (i--)
    {
        m_places[i].tokens = marks[i];
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
bool PetriNet::addArc(Node& from, Node& to, float const duration, bool const strict)
{
    // Arc already existing ?
    if (findArc(from, to) != nullptr)
    {
        m_message.str("");
        m_message << "Failed adding arc " << from.key
                  << " --> " << to.key
                  << ": Arc already exist"
                  << std::endl;
        return false;
    }

    // Key if the origin node exists (TBD: is this really
    // necessary since findArc would have returned false ?)
    if (findNode(from.key) == nullptr)
    {
        m_message << "Failed adding arc " << from.key
                  << " --> " << to.key
                  << ": The node " << from.key
                  << " does not exist"
                  << std::endl;
        return false;
    }

    // Key if the destination node exists (TBD: is this really
    // necessary since findArc would have returned false ?)
    if (findNode(to.key) == nullptr)
    {
        m_message << "Failed adding arc " << from.key
                  << " --> " << to.key
                  << ": The node " << to.key
                  << " does not exist"
                  << std::endl;
        return false;
    }

    // The user tried to link two nodes of the same type: this is
    // not possible in Petri nets, GRAFCET ... We offer two options:
    if (from.type == to.type)
    {
        // Option 1: we simply fail (for example when loading file)
        if (strict)
        {
            m_message.str("");
            m_message << "Failed adding arc " << from.key
                    << " --> " << to.key
                    << ": nodes type shall not be the same"
                    << std::endl;
            return false;
        }
        // Option 2: We add the extra node of the good type and
        // we add a second arc.
        else
        {
            float x = to.x + (from.x - to.x) / 2.0f;
            float y = to.y + (from.y - to.y) / 2.0f;
            if (to.type == Node::Type::Place)
            {
                // Frist arc
                Transition& n = addTransition(x, y);
                m_arcs.push_back(Arc(from, n, duration));
                from.arcsOut.push_back(&m_arcs.back());
                n.arcsIn.push_back(&m_arcs.back());

                // Second arc
                m_arcs.push_back(Arc(n, to, duration));
                n.arcsOut.push_back(&m_arcs.back());
                to.arcsIn.push_back(&m_arcs.back());
            }
            else
            {
                // Frist arc
                Place& n = addPlace(x, y);
                m_arcs.push_back(Arc(from, n, duration));
                from.arcsOut.push_back(&m_arcs.back());
                n.arcsIn.push_back(&m_arcs.back());

                // Second arc
                m_arcs.push_back(Arc(n, to, duration));
                n.arcsOut.push_back(&m_arcs.back());
                to.arcsIn.push_back(&m_arcs.back());
            }

            modified = true;
            return true;
        }
    }

    // Arc Place -> Transition or arc Transition -> Place ? Add the arc
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
bool PetriNet::isEventGraph(std::vector<Arc*>& erroneous_arcs) const
{
    erroneous_arcs.clear();
    if (isEmpty())
    {
        m_message.str("");
        m_message << "Empty Petri net is not an event graph" << std::endl;
        return false;
    }

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
void PetriNet::toCanonicalForm(PetriNet& canonic) const
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
bool PetriNet::toAdjacencyMatrices(SparseMatrix& tokens, SparseMatrix& durations)
{
    generateArcsInArcsOut(/*arcs: true*/);
    size_t const nnodes = m_transitions.size();

    durations.clear(); durations.dim(nnodes, nnodes);
    tokens.clear(); tokens.dim(nnodes, nnodes);

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
        durations.add(to.id, from.id, p.arcsIn[0]->duration);
        tokens.add(to.id, from.id, float(p.tokens));
    }

    return true;
}

//------------------------------------------------------------------------------
void PetriNet::toSysLin(SparseMatrix& D, SparseMatrix& A, SparseMatrix& B, SparseMatrix& C,
                        std::vector<size_t> const& indices, size_t const nb_inputs,
                        size_t const nb_states, size_t const nb_outputs)
{
    D.clear(); A.clear(); B.clear(); C.clear();
    D.dim(nb_states, nb_states);
    A.dim(nb_states, nb_states);
    B.dim(nb_inputs, nb_states);
    C.dim(nb_states, nb_outputs);

    // Note origin and destination are inverted because we use the following
    // matrix product convension: M * x where x is a column vector.
    for (auto& arc: m_arcs)
    {
        if (arc.from.type == Node::Type::Place)
            continue;

        Transition& t = *reinterpret_cast<Transition*>(&(arc.from));
        if (t.isInput())
        {
            Place& p = *reinterpret_cast<Place*>(&(arc.to));
            for (auto& a: p.arcsOut)
            {
                // System inputs: B U(n)
                Transition& td = *reinterpret_cast<Transition*>(&(a->to));
                B.add(indices[td.id], indices[t.id], float(arc.duration));
            }
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
                        A.add(indices[td.id], indices[t.id], arc.duration);
                    }
                    else
                    {
                        D.add(indices[td.id], indices[t.id], arc.duration);
                    }
                }
                else if (td.isOutput())
                {
                    // System outputs: Y(n) = C X(n)
                    C.add(indices[td.id], indices[t.id], arc.duration);
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
bool PetriNet::toSysLin(SparseMatrix& D, SparseMatrix& A, SparseMatrix& B, SparseMatrix& C)
{
    generateArcsInArcsOut();

    // Only Petri net with places having a single input and output arcs are
    // allowed.
    if (!isEventGraph(m_result_arcs))
        return false;

    // Duplicate the Petri net to preserve it because we will modify it into
    // its canonical form (places with several tokens will splitted to places
    // with a single token). Canonical Petri net have 0 or 1 tokens on each
    // places.
    PetriNet canonical(m_type);
    toCanonicalForm(canonical);

    // Count the number of inputs, outputs and states for creating matrices.
    size_t nb_states = 0u;
    size_t nb_inputs = 0u;
    size_t nb_outputs = 0u;

    std::vector<size_t> indices;
    indices.resize(canonical.transitions().size());

    for (auto& t: canonical.transitions())
    {
        if (t.isInput())
        {
            indices[t.id] = nb_inputs++;
        }
        if (t.isState())
        {
            indices[t.id] = nb_states++;
        }
        if (t.isOutput())
        {
            indices[t.id] = nb_outputs++;
        }
    }

    canonical.toSysLin(D, A, B, C, indices, nb_inputs, nb_states, nb_outputs);
    return true;
}

//------------------------------------------------------------------------------
std::stringstream PetriNet::showCounterEquation(std::string const& comment,
                                                bool use_caption,
                                                bool minplus_notation) const
{
    std::stringstream ss;

    ss << comment << "Timed event graph represented as counter equation"
       << (minplus_notation ? " (min-plus algebra):" : ":") << std::endl;
    for (auto const& t: this->transitions())
    {
        if (t.arcsIn.size() == 0u)
            continue;

        ss << comment << (use_caption ? t.caption : t.key) << "(t) = ";
        ss << (minplus_notation ? "" : "min(");
        std::string separator1;
        for (auto const& ai: t.arcsIn)
        {
            ss << separator1;
            if (ai->tokensIn() != 0u)
            {
                ss << ai->tokensIn() << (minplus_notation ? " " : " + ");
            }
            std::string separator2;
            for (auto const& ao: ai->from.arcsIn)
            {
                ss << separator2;
                ss << (use_caption ? ao->from.caption : ao->from.key);
                if (ao->duration != 0u)
                {
                    ss << "(t - " << ao->duration << ")";
                }
                else
                {
                    ss << "(t)";
                }
                separator2 = (minplus_notation ? " ⨁ " : ", ");
            }
            separator1 = (minplus_notation ? " ⨁ " : ", ");
        }
        ss << (minplus_notation ? "" : ")") << std::endl;
    }

    return ss;
}

//------------------------------------------------------------------------------
std::stringstream PetriNet::showDaterEquation(std::string const& comment,
                                              bool use_caption,
                                              bool maxplus_notation) const
{
    std::stringstream ss;

    ss << comment << "Timed event graph represented as dater equation"
       << (maxplus_notation ? " (max-plus algebra):" : ":") << std::endl;
    for (auto const& t: this->transitions())
    {
        if (t.arcsIn.size() == 0u)
            continue;

        ss << comment << (use_caption ? t.caption : t.key) << "(n) = ";
        ss << (maxplus_notation ? "" : "max(");
        std::string separator1;
        for (auto const& ai: t.arcsIn)
        {
            ss << separator1;
            std::string separator2;
            for (auto const& ao: ai->from.arcsIn)
            {
                ss << separator2;
                if (ao->duration != 0u)
                {
                    ss << ao->duration << (maxplus_notation ? " " : " + ");
                }
                ss << (use_caption ? ao->from.caption : ao->from.key) << "(n";
                if (ai->tokensIn() != 0u)
                {
                    ss << " - " << ai->tokensIn();
                }
                ss << ")";

                separator2 = (maxplus_notation ? " ⨁ " : ", ");
            }
            separator1 = (maxplus_notation ? " ⨁ " : ", ");
        }
        ss << (maxplus_notation ? "" : ")") << std::endl;
    }

    return ss;
}

//------------------------------------------------------------------------------
PetriNet::CriticalCycleResult PetriNet::findCriticalCycle()
{
    PetriNet::CriticalCycleResult result;

    generateArcsInArcsOut();
    if (!isEventGraph(result.arcs)) {
        result.message << "Not an event graph";
        result.success = false;
        return result;
    }

    // Number of nodes and number of arcs
    size_t const nnodes = m_transitions.size();
    size_t const narcs = m_places.size();

    // Reserve memory for storing timings
    std::vector<double> T; T.reserve(narcs);
    // Reserve memory for storing Tokens (delays)
    std::vector<double> N; N.reserve(narcs);
    // Reserve memory for storing arcs of the graph:
    // {(source node, destination node), ... }
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

        IJ.push_back(int(to.id)); // Transposed is needed
        IJ.push_back(int(from.id));
        T.push_back(p.arcsIn[0]->duration);
        N.push_back(double(p.tokens));
    }

    result.eigenvector.resize(nnodes);
    result.cycle_time.resize(nnodes);
    result.optimal_policy.resize(nnodes); // optimal policy
    int ncomponents; // Number of connected components of the optimal policy
    int niterations; // Number of iteration needed by the algorithm
    int verbosemode = 0; // No verbose
    int res = Semi_Howard(IJ.data(), T.data(), N.data(),
                          int(nnodes), int(narcs),
                          result.cycle_time.data(), result.eigenvector.data(),
                          result.optimal_policy.data(),
                          &niterations, &ncomponents, verbosemode);

    if ((res != 0) || (ncomponents == 0))
    {
        result.eigenvector.clear();
        result.cycle_time.clear();
        result.optimal_policy.clear();
        result.arcs.clear();
        result.message << "No policy found";
        result.success = false;
        return result;
    }

    size_t to = 0u;
    result.arcs.reserve(nnodes);
    result.message << "Critical cycle:" << std::endl;
    for (auto const& from: result.optimal_policy)
    {
        result.message << "  T" << from << " -> T" << to << std::endl;
        for (auto const& it: m_transitions[size_t(from)].arcsOut)
        {
            // Since we are working on an Event Graph we can directly access
            // Place -> arcsOut[0] -> Transition without checks.
            assert(it->to.arcsOut[0] != nullptr);
            assert(it->to.arcsOut[0]->to.type == Node::Type::Transition);
            if (it->to.arcsOut[0]->to.id == to)
            {
                result.arcs.push_back(it);
                result.arcs.push_back(it->to.arcsOut[0]);
                break;
            }
        }
        to += 1u;
    }

    result.message << "Cycle time [unit of time]:" << std::endl;
    for (auto const& it: result.cycle_time)
    {
        result.message << "  " << it << std::endl;
    }

    result.message << "Eigenvector:" << std::endl;
    for (auto const& it: result.eigenvector)
    {
        result.message << "  " << it << std::endl;
    }

    result.success = true;
    return result;
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

//------------------------------------------------------------------------------
void PetriNet::name(std::string const& filename)
{
    size_t lastindex = filename.find_last_of(".");
    m_name = filename.substr(0, lastindex);
    lastindex = m_name.find_last_of("/");
    m_name = m_name.substr(lastindex + 1u);
}

//------------------------------------------------------------------------------
bool PetriNet::load(std::string const& filename)
{
    clear();
    if (!importFromJSON(filename))
    {
        //clear(); FIXME: because this will clear error message
        return false;
    }

    name(filename);
    generateArcsInArcsOut();
    resetReceptivies(); // FIXME pas au bon endroit
    return true;
}
