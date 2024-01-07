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
#include "TimedPetriNetEditor/Algorithms.hpp"
#include "TimedPetriNetEditor/SparseMatrix.hpp"
#include "Net/Howard.h"

namespace tpne {

//------------------------------------------------------------------------------
// FIXME pas au bon endroit
// Used to control the behavior of operator<<.
template<> bool SparseMatrix<double>::display_for_julia = true;
template<> bool SparseMatrix<double>::display_as_dense = false;

//------------------------------------------------------------------------------
bool isEventGraph(Net const& net, std::string& error, std::vector<Arc*>& erroneous_arcs)
{
    erroneous_arcs.clear();
    error.clear();
    if (net.isEmpty())
    {
        error = "Empty Petri net is not an event graph";
        return false;
    }

    // The Petri net shall be an event graph: all places shall have a single
    // input arc and a single output arc. Else, we cannot generate the linear
    // system.
    for (auto const& p: net.places())
    {
        if (!((p.arcsIn.size() == 1u) && (p.arcsOut.size() == 1u)))
        {
            std::stringstream message;

            // Help the user to debug the Petri net. // TODO: could be nice to
            // show directly odd arcs in red but for the moment we display on
            // the console.
            message << "The Petri net is not an event graph. Because:"
                    << std::endl;

            if (p.arcsOut.size() != 1u)
            {
                message << "  " << p.key
                        << ((p.arcsOut.size() > 1u)
                            ? " has more than one output arc:"
                            : " has no output arc");
                for (auto const& a: p.arcsOut)
                {
                    erroneous_arcs.push_back(a);
                    message << " " << a->to.key;
                }
                message << std::endl;
            }

            if (p.arcsIn.size() != 1u)
            {
                message << "  " << p.key
                        << ((p.arcsIn.size() > 1u)
                            ? " has more than one input arc:"
                            : " has no input arc");
                for (auto const& a: p.arcsIn)
                {
                    erroneous_arcs.push_back(a);
                    message << " " << a->from.key;
                }
                message << std::endl;
            }

            error = message.str();
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------------------------
bool isEventGraph(Net const& net)
{
    std::string error;
    std::vector<Arc*> erroneous_arcs;
    return isEventGraph(net, error, erroneous_arcs);
}

//------------------------------------------------------------------------------
void toCanonicalForm(Net const& net, Net& canonic)
{
    // Copy the whole net
    canonic = net;

    // Explode Places with more than one tokens and create as many as Places
    // holding a single token. Redo arcs.
    {
        Net::Places& places = canonic.places();
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
    {
        canonic.generateArcsInArcsOut();
        Net::Places& places = canonic.places();
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
}

//------------------------------------------------------------------------------
void toSysLin(Net const& net,
              SparseMatrix<double>& D, SparseMatrix<double>& A,
              SparseMatrix<double>& B, SparseMatrix<double>& C,
              std::vector<size_t> const& indices, size_t const nb_inputs,
              size_t const nb_states, size_t const nb_outputs)
{
    D.clear(); A.clear(); B.clear(); C.clear();
    D.reshape(nb_states, nb_states);
    A.reshape(nb_states, nb_states);
    B.reshape(nb_inputs, nb_states);
    C.reshape(nb_states, nb_outputs);

    // Note origin and destination are inverted because we use the following
    // matrix product convension: M * x where x is a column vector.
    for (auto const& arc: net.arcs())
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
                B.set(indices[td.id], indices[t.id], float(arc.duration));
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
                        A.set(indices[td.id], indices[t.id], arc.duration);
                    }
                    else
                    {
                        D.set(indices[td.id], indices[t.id], arc.duration);
                    }
                }
                else if (td.isOutput())
                {
                    // System outputs: Y(n) = C X(n)
                    C.set(indices[td.id], indices[t.id], arc.duration);
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
bool toSysLin(Net const& net, SparseMatrix<double>& D, SparseMatrix<double>& A,
              SparseMatrix<double>& B, SparseMatrix<double>& C)
{
    std::vector<Arc*> arcs;
    std::string error;

    // Only Petri net with places having a single input and output arcs are
    // allowed.
    if (!isEventGraph(net, error, arcs))
        return false;

    // Duplicate the Petri net to preserve it because we will modify it into
    // its canonical form (places with several tokens will splitted to places
    // with a single token). Canonical Petri net have 0 or 1 tokens on each
    // places.
    Net canonical(net.type());
    toCanonicalForm(net, canonical);

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

    toSysLin(canonical, D, A, B, C, indices, nb_inputs, nb_states, nb_outputs);
    return true;
}

//------------------------------------------------------------------------------
bool toAdjacencyMatrices(Net const& net, SparseMatrix<double>& tokens, SparseMatrix<double>& durations)
{
    size_t const nnodes = net.transitions().size();

    durations.clear(); durations.reshape(nnodes, nnodes);
    tokens.clear(); tokens.reshape(nnodes, nnodes);

    for (auto const& p: net.places())
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
        durations.set(to.id, from.id, p.arcsIn[0]->duration);
        tokens.set(to.id, from.id, float(p.tokens));
    }

    return true;
}

//------------------------------------------------------------------------------
std::stringstream showCounterEquation(Net const& net, std::string const& comment,
                                      bool use_caption,
                                      bool minplus_notation)
{
    std::stringstream ss;

    ss << comment << "Timed event graph represented as counter equation"
       << (minplus_notation ? " (min-plus algebra):" : ":") << std::endl;
    for (auto const& t: net.transitions())
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
std::stringstream showDaterEquation(Net const& net, std::string const& comment,
                                    bool use_caption,
                                    bool maxplus_notation)
{
    std::stringstream ss;

    ss << comment << "Timed event graph represented as dater equation"
       << (maxplus_notation ? " (max-plus algebra):" : ":") << std::endl;
    for (auto const& t: net.transitions())
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
CriticalCycleResult findCriticalCycle(Net const& net)
{
    CriticalCycleResult result;
    std::string error;

    if (!isEventGraph(net, error, result.arcs)) {
        result.message << error;
        result.success = false;
        return result;
    }

    // Number of nodes and number of arcs
    size_t const nnodes = net.transitions().size();
    size_t const narcs = net.places().size();

    // Reserve memory for storing timings
    std::vector<double> T; T.reserve(narcs);
    // Reserve memory for storing Tokens (delays)
    std::vector<double> N; N.reserve(narcs);
    // Reserve memory for storing arcs of the graph:
    // {(source node, destination node), ... }
    // FIXME should be std::vector<size_t> but Howard wants int*
    std::vector<int> IJ; IJ.reserve(2u * narcs);

    for (auto const& p: net.places())
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
        auto const& arcsOut = net.transitions()[size_t(from)].arcsOut;
        for (auto const& it: arcsOut)
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

} // namespace tpne
