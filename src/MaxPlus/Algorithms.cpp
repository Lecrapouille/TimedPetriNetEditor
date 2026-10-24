// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "MaxPlus/Algorithms.hpp"
#include "PetriNet/PetriNet.hpp"
#include "MaxPlus/SparseMatrix.hpp"
#include "MaxPlus/TropicalAlgebra.hpp"
#include "MaxPlus/Howard.h"

namespace tpne {


//------------------------------------------------------------------------------
void toCanonicalForm(Net const& net, Net& canonic)
{
    // Copy the whole net
    canonic = net;

    // The nodes added between a and b are drawn on the segment [a, b]: the
    // k-th of n at k / (n + 1).
    auto between = [](Point const& a, Point const& b, size_t const k, size_t const n) -> Point {
        float const f = float(k) / float(n + 1u);
        return { a.x + (b.x - a.x) * f, a.y + (b.y - a.y) * f };
    };

    // Explode Places with more than one tokens and create as many as Places
    // holding a single token. Redo arcs. Adding places moves them in memory:
    // the place is found again by its index.
    {
        size_t i = canonic.places().size();
        while (i--)
        {
            if (canonic.places()[i].tokens > 1u)
            {
                // from: Transition
                Arc const& in = canonic.arcsIn(canonic.places()[i]).front();
                NodeId from = in.from;
                float duration = in.durationOrZero();
                size_t const pairs = canonic.places()[i].tokens - 1u;
                Point const a = canonic.position(from);
                Point const b = canonic.position(canonic.places()[i]);
                canonic.removeArc(canonic.node(from), canonic.places()[i]);
                for (size_t k = 0u; k < pairs; ++k)
                {
                    NodeId const tmp1 = canonic.addPlace(1u).nodeId();
                    canonic.position(tmp1) = between(a, b, 2u * k + 1u, 2u * pairs);
                    canonic.addArc(canonic.node(from), canonic.node(tmp1));
                    NodeId const tmp2 = canonic.addTransition().nodeId();
                    canonic.position(tmp2) = between(a, b, 2u * k + 2u, 2u * pairs);
                    canonic.addArc(canonic.node(tmp1), canonic.node(tmp2));
                    from = tmp2;
                }
                canonic.places()[i].tokens = 1u;
                canonic.addArc(canonic.node(from), canonic.places()[i], duration);
            }
        }
    }

    // Manage Places with one token that inputs or outputs
    {
        size_t i = canonic.places().size();
        while (i--)
        {
            if (canonic.places()[i].tokens == 1u)
            {
                // Inputs
                Arc const& in = canonic.arcsIn(canonic.places()[i]).front();
                Transition const& from = canonic.transitionOf(in);
                if (canonic.isInput(from))
                {
                    NodeId const f = from.nodeId();
                    float duration = in.durationOrZero();
                    Point const a = canonic.position(f);
                    Point const b = canonic.position(canonic.places()[i]);
                    canonic.removeArc(canonic.node(f), canonic.places()[i]);

                    NodeId const tmp1 = canonic.addPlace().nodeId();
                    NodeId const tmp2 = canonic.addTransition().nodeId();
                    canonic.position(tmp1) = between(a, b, 1u, 2u);
                    canonic.position(tmp2) = between(a, b, 2u, 2u);
                    canonic.addArc(canonic.node(f), canonic.node(tmp1));
                    canonic.addArc(canonic.node(tmp1), canonic.node(tmp2), duration);
                    canonic.addArc(canonic.node(tmp2), canonic.places()[i]);
                }

                // Outputs
                Transition const& to = canonic.transitionOf(canonic.arcsOut(canonic.places()[i]).front());
                if (canonic.isOutput(to))
                {
                    NodeId const t = to.nodeId();
                    Point const a = canonic.position(canonic.places()[i]);
                    Point const b = canonic.position(t);
                    canonic.removeArc(canonic.places()[i], canonic.node(t));

                    NodeId const tmp1 = canonic.addTransition().nodeId();
                    NodeId const tmp2 = canonic.addPlace().nodeId();
                    canonic.position(tmp1) = between(a, b, 1u, 2u);
                    canonic.position(tmp2) = between(a, b, 2u, 2u);
                    canonic.addArc(canonic.places()[i], canonic.node(tmp1));
                    canonic.addArc(canonic.node(tmp1), canonic.node(tmp2));
                    canonic.addArc(canonic.node(tmp2), canonic.node(t));
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
void toSysLin(Net const& net,
              SparseMatrix<MaxPlus>& D, SparseMatrix<MaxPlus>& A,
              SparseMatrix<MaxPlus>& B, SparseMatrix<MaxPlus>& C,
              std::vector<size_t> const& indices, size_t const nb_inputs,
              size_t const nb_states, size_t const nb_outputs)
{
    D.clear(); A.clear(); B.clear(); C.clear();
    D.reshape(nb_states, nb_states);
    A.reshape(nb_states, nb_states);
    B.reshape(nb_states, nb_inputs);
    C.reshape(nb_outputs, nb_states);

    // Note origin and destination are inverted because we use the following
    // matrix product convension: M * x where x is a column vector.
    for (auto const& arc: net.arcs())
    {
        if (arc.from.type == Node::Type::Place)
            continue;

        Transition const& from = net.transitionOf(arc);
        Place const& p = net.placeOf(arc);
        if (net.isInput(from))
        {
            for (auto const& a: net.arcsOut(p))
            {
                // System inputs: B U(k)
                Transition const& to = net.transitionOf(a);
                B.set(indices[to.id], indices[from.id],
                      static_cast<double>(arc.durationOrZero()));
            }
        }
        else // States or outputs
        {
            for (auto const& a: net.arcsOut(p))
            {
                Transition const& to = net.transitionOf(a);
                if (net.isState(to))
                {
                    // Systems states: X(k) = D X(k) (+) A X(k-1)
                    if (p.tokens == 1u)
                    {
                        A.set(indices[to.id], indices[from.id],
                              static_cast<double>(arc.durationOrZero()));
                    }
                    else
                    {
                        D.set(indices[to.id], indices[from.id],
                              static_cast<double>(arc.durationOrZero()));
                    }
                }
                else if (net.isOutput(to))
                {
                    // System outputs: Y(k) = C X(k)
                    C.set(indices[to.id], indices[from.id],
                          static_cast<double>(arc.durationOrZero()));
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
bool toSysLin(Net const& net, SparseMatrix<MaxPlus>& D, SparseMatrix<MaxPlus>& A,
              SparseMatrix<MaxPlus>& B, SparseMatrix<MaxPlus>& C)
{
    std::vector<size_t> arcs;
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
        if (canonical.isInput(t))
        {
            indices[t.id] = nb_inputs++;
        }
        if (canonical.isState(t))
        {
            indices[t.id] = nb_states++;
        }
        if (canonical.isOutput(t))
        {
            indices[t.id] = nb_outputs++;
        }
    }

    toSysLin(canonical, D, A, B, C, indices, nb_inputs, nb_states, nb_outputs);
    return true;
}

//------------------------------------------------------------------------------
bool toAdjacencyMatrices(Net const& net, SparseMatrix<MaxPlus>& tokens, SparseMatrix<MaxPlus>& durations)
{
    size_t const nnodes = net.transitions().size();

    durations.clear(); durations.reshape(nnodes, nnodes);
    tokens.clear(); tokens.reshape(nnodes, nnodes);

    for (auto const& p: net.places())
    {
        // Since we are sure this Petri net is an event graph: places have a
        // single input arc and a single output arc. We can merge the place and
        // its arcs into a single arc.
        auto const arcs_in = net.arcsIn(p);
        auto const arcs_out = net.arcsOut(p);
        if ((arcs_in.size() != 1u) || (arcs_out.size() != 1u))
            return false;

        Transition const& from = net.transitionOf(arcs_in[0]);
        Transition const& to = net.transitionOf(arcs_out[0]);

        // Note origin and destination are inverted because we use the following
        // matrix product convension: M * x where x is a column vector.
        durations.set(from.id, to.id,
                      static_cast<double>(arcs_in[0].durationOrZero()));
        tokens.set(from.id, to.id, static_cast<double>(p.tokens));
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
        if (net.arcsIn(t).empty())
            continue;

        ss << comment << (use_caption ? t.caption : t.key) << "(t) = ";
        ss << (minplus_notation ? "" : "min(");
        std::string separator1;
        for (auto const& ai: net.arcsIn(t))
        {
            ss << separator1;
            size_t const tokens = net.placeOf(ai).tokens;
            if (tokens != 0u)
            {
                ss << tokens << (minplus_notation ? " " : " + ");
            }
            std::string separator2;
            for (auto const& ao: net.arcsIn(net.placeOf(ai)))
            {
                Transition const& from = net.transitionOf(ao);
                ss << separator2;
                ss << (use_caption ? from.caption : from.key);
                if (ao.durationOrZero() != 0.0f)
                {
                    ss << "(t - " << ao.durationOrZero() << ")";
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
        if (net.arcsIn(t).empty())
            continue;

        ss << comment << (use_caption ? t.caption : t.key) << "(k) = ";
        ss << (maxplus_notation ? "" : "max(");
        std::string separator1;
        for (auto const& ai: net.arcsIn(t))
        {
            ss << separator1;
            std::string separator2;
            size_t const tokens = net.placeOf(ai).tokens;
            for (auto const& ao: net.arcsIn(net.placeOf(ai)))
            {
                Transition const& from = net.transitionOf(ao);
                ss << separator2;
                if (ao.durationOrZero() != 0.0f)
                {
                    ss << ao.durationOrZero() << (maxplus_notation ? " " : " + ");
                }
                ss << (use_caption ? from.caption : from.key) << "(k";
                if (tokens != 0u)
                {
                    ss << " - " << tokens;
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
        assert(net.arcsIn(p).size() == 1u);
        assert(net.arcsOut(p).size() == 1u);
        Arc const& in = net.arcsIn(p)[0];
        Transition const& from = net.transitionOf(in);
        Transition const& to = net.transitionOf(net.arcsOut(p)[0]);

        IJ.push_back(int(from.id)); // Transposed is needed
        IJ.push_back(int(to.id));
        T.push_back(static_cast<double>(in.durationOrZero()));
        N.push_back(double(p.tokens));
    }

    result.eigenvector.resize(nnodes);
    result.durations.resize(nnodes);
    std::vector<int> optimal_policy;
    optimal_policy.resize(nnodes);
    int ncomponents; // Number of connected components of the optimal policy
    int niterations; // Number of iteration needed by the algorithm
    int verbosemode = 0; // No verbose
    // Note Semi_Howard is C code and use directly double instead of (max,+).
    int res = Semi_Howard(IJ.data(), T.data(), N.data(),
                          int(nnodes), int(narcs),
                          result.durations.data(), result.eigenvector.data(),
                          optimal_policy.data(),
                          &niterations, &ncomponents, verbosemode);

    result.cycles = size_t(ncomponents);
    if ((res != 0) || (ncomponents == 0))
    {
        result.eigenvector.clear();
        result.durations.clear();
        result.cycles = 0u;
        result.arcs.clear();
        result.message << "No optimal policy found";
        result.success = false;
        return result;
    }

    result.arcs.reserve(nnodes);
    result.message << "Found " << ncomponents
        << " connected components of the optimal policy:"
        << std::endl;

    // Howard returns a policy 'pi' (optimal_policy[to] = successor of node 'to').
    // This is a functional graph: each node has exactly one outgoing policy arc
    // 'to -> pi[to]'. Following it from any node always ends on a cycle, and only
    // the arcs lying ON a cycle form the critical circuit(s). The transient arcs
    // that merely feed into the cycles are NOT critical and must not be
    // highlighted.
    std::vector<char> visitState(nnodes, 0); // 0=unvisited, 1=on current path, 2=done
    std::vector<char> onCycle(nnodes, 0);
    for (size_t s = 0u; s < nnodes; ++s)
    {
        if (visitState[s] != 0)
            continue;

        std::vector<size_t> path;
        size_t u = s;
        while ((u < nnodes) && (visitState[u] == 0))
        {
            visitState[u] = 1;
            path.push_back(u);
            u = size_t(optimal_policy[u]);
        }

        // We closed a cycle iff we returned onto a node of the current path.
        if ((u < nnodes) && (visitState[u] == 1))
        {
            size_t k = 0u;
            while ((k < path.size()) && (path[k] != u))
                ++k;
            for (; k < path.size(); ++k)
                onCycle[path[k]] = 1;
        }

        for (size_t const v : path)
            visitState[v] = 2;
    }

    // Reconstruct only the critical-circuit arcs (those whose source node is on a
    // policy cycle) into our internal format (including the in-between places).
    for (size_t to = 0u; to < optimal_policy.size(); ++to)
    {
        if (onCycle[to] == 0)
            continue;

        size_t from = size_t(optimal_policy[to]);
        result.message << "  T" << to << " -> T" << from << std::endl;
        // Search for all places that link our transitions.
        for (auto const& p: net.places())
        {
            auto const arcs_in = net.arcsIn(p);
            auto const arcs_out = net.arcsOut(p);
            assert(arcs_in.size() == 1u);
            assert(arcs_out.size() == 1u);
            if ((arcs_out[0].to.id == from) && (arcs_in[0].from.id == to))
            {
                result.arcs.push_back(arcs_in.indices()[0]);
                result.arcs.push_back(arcs_out.indices()[0]);
                break;
            }
        }
    }

    result.message << "Cycle durations [unit of time]:" << std::endl;
    for (size_t i = 0u; i < result.durations.size(); ++i)
    {
        result.message << "  T" << i << ": "
            << result.durations[i] << std::endl;
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
void incidenceMatrices(Net const& net, SparseMatrix<double>& pre, SparseMatrix<double>& post)
{
    size_t const places = net.places().size();
    size_t const transitions = net.transitions().size();
    pre.clear(); pre.reshape(places, transitions);
    post.clear(); post.reshape(places, transitions);
    for (auto const& a: net.arcs())
    {
        if (a.kind != ArcKind::Normal)
            continue;
        if (a.from.isPlace())
            pre.set(a.from.id, a.to.id, double(a.weight));
        else
            post.set(a.to.id, a.from.id, double(a.weight));
    }
}

} // namespace tpne
