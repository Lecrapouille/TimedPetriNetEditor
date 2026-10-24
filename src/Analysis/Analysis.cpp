// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Analysis/Analysis.hpp"
#include "PetriNet/PetriNet.hpp"

#include <algorithm>
#include <map>
#include <numeric>
#include <sstream>

namespace tpne {

//! \brief Max number of rows kept by the algorithm of Farkas.
static constexpr size_t MAX_FARKAS_ROWS = 10000u;

//------------------------------------------------------------------------------
//! \brief Name of a node for the reports: "P0" or "P0 (Ready)".
//------------------------------------------------------------------------------
static std::string nameOf(Node const& node)
{
    if (node.caption.empty() || (node.caption == node.key))
        return node.key;
    return node.key + " (" + node.caption + ")";
}

//------------------------------------------------------------------------------
ReachabilityGraph reachabilityGraph(Net const& net, size_t const max_states)
{
    ReachabilityGraph graph;
    std::map<Marking, size_t> known;

    graph.states.push_back(net.tokens());
    known.emplace(graph.states[0], 0u);

    // The states are explored in the order they are found: breadth first.
    for (size_t s = 0u; s < graph.states.size(); ++s)
    {
        std::vector<size_t> const enabled = enabledTransitions(net, graph.states[s]);
        if (enabled.empty())
            graph.deadlocks.push_back(s);

        for (size_t t: enabled)
        {
            Marking next = fire(net, graph.states[s], t);
            auto it = known.find(next);
            if (it == known.end())
            {
                if (graph.states.size() >= max_states)
                {
                    graph.complete = false;
                    continue;
                }
                it = known.emplace(next, graph.states.size()).first;
                graph.states.push_back(std::move(next));
            }
            graph.edges.push_back({ s, t, it->second });
        }
    }
    return graph;
}

//------------------------------------------------------------------------------
//! \brief Why Karp-Miller does not apply to the net (empty if it does): the
//! firing rule shall be monotonic (more tokens never disable a transition).
//------------------------------------------------------------------------------
static std::string notMonotonic(Net const& net)
{
    unsigned const f = features(net);
    if (f & Feature::Inhibitors)
        return "inhibitor arcs";
    if (f & Feature::Resets)
        return "reset arcs";
    for (auto const& p: net.places())
    {
        if (p.capacity != std::numeric_limits<size_t>::max())
            return "places with a capacity";
    }
    return {};
}

//------------------------------------------------------------------------------
//! \brief The firing rule with OMEGA (only normal arcs, no capacity): OMEGA
//! minus or plus some tokens stays OMEGA.
//------------------------------------------------------------------------------
static bool coverEnabled(Net const& net, Marking const& m, size_t const t)
{
    for (auto const& a: net.arcsIn(net.transitions()[t]))
    {
        if ((m[a.from.id] != OMEGA) && (m[a.from.id] < a.weight))
            return false;
    }
    return true;
}

static Marking coverFire(Net const& net, Marking const& m, size_t const t)
{
    Marking next(m);
    Transition const& transition = net.transitions()[t];
    for (auto const& a: net.arcsIn(transition))
    {
        if (next[a.from.id] != OMEGA)
            next[a.from.id] -= a.weight;
    }
    for (auto const& a: net.arcsOut(transition))
    {
        if (next[a.to.id] != OMEGA)
            next[a.to.id] += a.weight;
    }
    return next;
}

//------------------------------------------------------------------------------
CoverabilityGraph coverabilityGraph(Net const& net, size_t const max_states)
{
    CoverabilityGraph graph;
    graph.error = notMonotonic(net);
    if (!graph.error.empty())
    {
        graph.error = "Karp-Miller does not apply to nets with " + graph.error;
        graph.complete = false;
        return graph;
    }

    std::map<Marking, size_t> known;
    // State from which each state has been found: its ancestors.
    std::vector<size_t> parent;

    graph.states.push_back(net.tokens());
    parent.push_back(0u);
    known.emplace(graph.states[0], 0u);

    for (size_t s = 0u; s < graph.states.size(); ++s)
    {
        for (size_t t = 0u; t < net.transitions().size(); ++t)
        {
            if (!coverEnabled(net, graph.states[s], t))
                continue;

            // An ancestor covered by the new marking: the places having more
            // tokens can grow without bound.
            Marking next = coverFire(net, graph.states[s], t);
            for (size_t a = s;; a = parent[a])
            {
                Marking const& old = graph.states[a];
                if (std::equal(old.begin(), old.end(), next.begin(),
                               [](size_t o, size_t n) { return o <= n; }))
                {
                    for (size_t p = 0u; p < next.size(); ++p)
                    {
                        if (next[p] > old[p])
                            next[p] = OMEGA;
                    }
                }
                if (a == 0u)
                    break;
            }

            auto it = known.find(next);
            if (it == known.end())
            {
                if (graph.states.size() >= max_states)
                {
                    graph.complete = false;
                    continue;
                }
                it = known.emplace(next, graph.states.size()).first;
                graph.states.push_back(std::move(next));
                parent.push_back(s);
            }
            graph.edges.push_back({ s, t, it->second });
        }
    }
    return graph;
}

//------------------------------------------------------------------------------
//! \brief Strongly connected components (Tarjan, without recursion: the
//! graphs can have many states).
//! \return the component of each state.
//------------------------------------------------------------------------------
static std::vector<size_t> components(size_t const count,
                                      std::vector<ReachabilityGraph::Edge> const& edges,
                                      size_t& number)
{
    constexpr size_t NONE = std::numeric_limits<size_t>::max();

    // Successors of each state.
    std::vector<size_t> first(count + 1u, 0u);
    for (auto const& e: edges)
        ++first[e.from + 1u];
    std::partial_sum(first.begin(), first.end(), first.begin());
    std::vector<size_t> next(edges.size());
    std::vector<size_t> fill(first.begin(), first.end() - 1);
    for (auto const& e: edges)
        next[fill[e.from]++] = e.to;

    std::vector<size_t> index(count, NONE), low(count, 0u), component(count, NONE);
    std::vector<size_t> stack, calls, edge_of(count, 0u);
    size_t counter = 0u;
    number = 0u;

    for (size_t root = 0u; root < count; ++root)
    {
        if (index[root] != NONE)
            continue;
        calls.push_back(root);
        while (!calls.empty())
        {
            size_t const v = calls.back();
            if (index[v] == NONE)
            {
                index[v] = low[v] = counter++;
                edge_of[v] = first[v];
                stack.push_back(v);
            }
            if (edge_of[v] < first[v + 1u])
            {
                size_t const w = next[edge_of[v]++];
                if (index[w] == NONE)
                    calls.push_back(w);
                else if (component[w] == NONE)
                    low[v] = std::min(low[v], index[w]);
                continue;
            }
            calls.pop_back();
            if (!calls.empty())
                low[calls.back()] = std::min(low[calls.back()], low[v]);
            if (low[v] == index[v])
            {
                size_t w;
                do
                {
                    w = stack.back();
                    stack.pop_back();
                    component[w] = number;
                } while (w != v);
                ++number;
            }
        }
    }
    return component;
}

//------------------------------------------------------------------------------
//! \brief Transitions that are not live (L4) in a complete reachability
//! graph: a transition is live when it labels an edge of each terminal
//! component (the ones the net cannot leave).
//------------------------------------------------------------------------------
static std::vector<size_t> notLive(Net const& net, ReachabilityGraph const& graph)
{
    size_t number;
    std::vector<size_t> const component = components(graph.states.size(), graph.edges, number);

    std::vector<bool> terminal(number, true);
    std::vector<std::vector<bool>> fired(number, std::vector<bool>(net.transitions().size(), false));
    for (auto const& e: graph.edges)
    {
        if (component[e.from] != component[e.to])
            terminal[component[e.from]] = false;
        else
            fired[component[e.from]][e.transition] = true;
    }

    std::vector<size_t> result;
    for (size_t t = 0u; t < net.transitions().size(); ++t)
    {
        for (size_t c = 0u; c < number; ++c)
        {
            if (terminal[c] && !fired[c][t])
            {
                result.push_back(t);
                break;
            }
        }
    }
    return result;
}

//------------------------------------------------------------------------------
//! \brief Transitions labeling no edge.
//------------------------------------------------------------------------------
static std::vector<size_t> neverFired(Net const& net, std::vector<ReachabilityGraph::Edge> const& edges)
{
    std::vector<bool> fired(net.transitions().size(), false);
    for (auto const& e: edges)
        fired[e.transition] = true;

    std::vector<size_t> result;
    for (size_t t = 0u; t < fired.size(); ++t)
    {
        if (!fired[t])
            result.push_back(t);
    }
    return result;
}

//------------------------------------------------------------------------------
//! \brief Max tokens of each place over the states.
//------------------------------------------------------------------------------
static std::vector<size_t> maxTokens(size_t const places, std::vector<Marking> const& states)
{
    std::vector<size_t> bounds(places, 0u);
    for (auto const& m: states)
    {
        for (size_t p = 0u; p < places; ++p)
            bounds[p] = std::max(bounds[p], m[p]);
    }
    return bounds;
}

//------------------------------------------------------------------------------
Properties properties(Net const& net, size_t const max_states)
{
    Properties result;

    if (net.type() == TypeOfNet::TimedPetriNet)
        result.notes.push_back("The durations are not considered: the timed net may "
                               "reach fewer markings.");
    else if (net.type() == TypeOfNet::GRAFCET)
        result.notes.push_back("The rules of evolution of GRAFCET (receptivities, "
                               "simultaneous firing) are not considered.");
    if (features(net) & Feature::Colors)
        result.notes.push_back("The colors of the tokens are not considered.");

    ReachabilityGraph const graph = reachabilityGraph(net, max_states);
    result.states = graph.states.size();
    result.complete = graph.complete;

    // Deadlock: the first one found is the closest to the initial marking,
    // the path to it follows the edges that found each state.
    if (!graph.deadlocks.empty())
    {
        result.deadlock = true;
        size_t s = graph.deadlocks[0];
        result.deadlock_marking = graph.states[s];

        constexpr size_t NONE = std::numeric_limits<size_t>::max();
        std::vector<size_t> found_by(graph.states.size(), NONE);
        for (size_t e = 0u; e < graph.edges.size(); ++e)
        {
            size_t const to = graph.edges[e].to;
            if ((to != 0u) && (found_by[to] == NONE))
                found_by[to] = e;
        }
        for (; s != 0u; s = graph.edges[found_by[s]].from)
            result.deadlock_path.push_back(graph.edges[found_by[s]].transition);
        std::reverse(result.deadlock_path.begin(), result.deadlock_path.end());
    }

    if (graph.complete)
    {
        result.dead_transitions = neverFired(net, graph.edges);
        result.bounds = maxTokens(net.places().size(), graph.states);
        result.not_live = notLive(net, graph);
        result.live = result.not_live.empty();
        return result;
    }

    // Too many markings: the net is maybe unbounded. Karp-Miller tells it and
    // gives the bounds and the dead transitions, but not the deadlocks nor
    // the liveness.
    result.notes.push_back("Stopped after " + std::to_string(max_states) +
                           " markings: no deadlock found is not a proof, the "
                           "liveness is unknown.");
    CoverabilityGraph const cover = coverabilityGraph(net, max_states);
    if (!cover.complete)
    {
        result.notes.push_back(cover.error.empty()
            ? "The coverability graph has too many states: the bounds and "
              "the dead transitions are the ones of the explored markings."
            : cover.error + ": the bounds and the dead transitions are the "
              "ones of the explored markings.");
        result.dead_transitions = neverFired(net, graph.edges);
        result.bounds = maxTokens(net.places().size(), graph.states);
        result.bounded = false;
        return result;
    }

    result.dead_transitions = neverFired(net, cover.edges);
    result.bounds = maxTokens(net.places().size(), cover.states);
    result.bounded = std::find(result.bounds.begin(), result.bounds.end(), OMEGA) == result.bounds.end();
    return result;
}

//------------------------------------------------------------------------------
//! \brief Incidence matrix C (places x transitions): tokens given minus
//! tokens taken by each firing. Only normal arcs.
//------------------------------------------------------------------------------
static std::vector<std::vector<int64_t>> incidence(Net const& net, std::string& error)
{
    error.clear();
    std::vector<std::vector<int64_t>> C(net.places().size(),
                                        std::vector<int64_t>(net.transitions().size(), 0));
    for (auto const& a: net.arcs())
    {
        if (a.kind != ArcKind::Normal)
        {
            error = "Only the normal arcs are considered: the invariants may not "
                    "hold with inhibitor or reset arcs.";
            continue;
        }
        int64_t const w = static_cast<int64_t>(a.weight);
        if (a.from.isPlace())
            C[a.from.id][a.to.id] -= w;
        else
            C[a.to.id][a.from.id] += w;
    }
    return C;
}

//------------------------------------------------------------------------------
//! \brief Minimal semi-positive vectors y such as y.A = 0 (algorithm of
//! Farkas): rows are combined two by two to cancel the columns one after the
//! other, keeping the rows of minimal support.
//------------------------------------------------------------------------------
static std::vector<std::vector<int64_t>>
farkas(std::vector<std::vector<int64_t>> const& A, size_t const columns, std::string& error)
{
    size_t const n = A.size();

    // A row: the part of A to cancel, then the combination of the rows of A.
    struct Row
    {
        std::vector<int64_t> a;
        std::vector<int64_t> y;
    };
    std::vector<Row> rows;
    for (size_t i = 0u; i < n; ++i)
    {
        Row r{ A[i], std::vector<int64_t>(n, 0) };
        r.y[i] = 1;
        rows.push_back(std::move(r));
    }

    auto support = [](Row const& r, Row const& s) {
        // Is the support of r included in the one of s ?
        for (size_t i = 0u; i < r.y.size(); ++i)
        {
            if ((r.y[i] != 0) && (s.y[i] == 0))
                return false;
        }
        return true;
    };

    for (size_t j = 0u; j < columns; ++j)
    {
        std::vector<Row> next;
        for (auto const& r: rows)
        {
            if (r.a[j] == 0)
                next.push_back(r);
        }
        for (auto const& r: rows)
        {
            if (r.a[j] <= 0)
                continue;
            for (auto const& s: rows)
            {
                if (s.a[j] >= 0)
                    continue;
                int64_t const kr = -s.a[j];
                int64_t const ks = r.a[j];
                Row c{ std::vector<int64_t>(columns), std::vector<int64_t>(n) };
                int64_t g = 0;
                for (size_t k = 0u; k < columns; ++k)
                    g = std::gcd(g, c.a[k] = kr * r.a[k] + ks * s.a[k]);
                for (size_t k = 0u; k < n; ++k)
                    g = std::gcd(g, c.y[k] = kr * r.y[k] + ks * s.y[k]);
                if (g > 1)
                {
                    for (auto& x: c.a) x /= g;
                    for (auto& x: c.y) x /= g;
                }
                next.push_back(std::move(c));
            }
        }

        // Remove the rows whose support strictly includes the one of another
        // row, and the duplicated rows.
        std::vector<bool> minimal(next.size(), true);
        for (size_t i = 0u; i < next.size(); ++i)
        {
            for (size_t k = 0u; (k < next.size()) && minimal[i]; ++k)
            {
                if ((k == i) || !support(next[k], next[i]))
                    continue;
                if (!support(next[i], next[k]))
                    minimal[i] = false;
                else if ((k < i) && (next[k].y == next[i].y) && (next[k].a == next[i].a))
                    minimal[i] = false;
            }
        }
        rows.clear();
        for (size_t i = 0u; i < next.size(); ++i)
        {
            if (minimal[i])
                rows.push_back(std::move(next[i]));
        }
        if (rows.size() > MAX_FARKAS_ROWS)
        {
            error = "Too many invariants: stopped the computation.";
            return {};
        }
    }

    std::vector<std::vector<int64_t>> result;
    for (auto& r: rows)
        result.push_back(std::move(r.y));
    return result;
}

//------------------------------------------------------------------------------
std::vector<std::vector<int64_t>> pInvariants(Net const& net, std::string& error)
{
    std::vector<std::vector<int64_t>> const C = incidence(net, error);
    std::string err;
    auto result = farkas(C, net.transitions().size(), err);
    if (!err.empty())
        error = err;
    return result;
}

//------------------------------------------------------------------------------
std::vector<std::vector<int64_t>> tInvariants(Net const& net, std::string& error)
{
    std::vector<std::vector<int64_t>> const C = incidence(net, error);
    std::vector<std::vector<int64_t>> Ct(net.transitions().size(),
                                         std::vector<int64_t>(net.places().size(), 0));
    for (size_t p = 0u; p < C.size(); ++p)
    {
        for (size_t t = 0u; t < C[p].size(); ++t)
            Ct[t][p] = C[p][t];
    }
    std::string err;
    auto result = farkas(Ct, net.places().size(), err);
    if (!err.empty())
        error = err;
    return result;
}

//------------------------------------------------------------------------------
std::string toDot(Net const& net, std::vector<Marking> const& states,
                  std::vector<ReachabilityGraph::Edge> const& edges)
{
    std::vector<bool> leaves(states.size(), true);
    for (auto const& e : edges)
        leaves[e.from] = false;

    auto quote = [](std::string const& text) {
        std::string s = "\"";
        for (char const c : text)
            s += ((c == '"') || (c == '\\')) ? std::string("\\") + c : std::string(1, c);
        return s + "\"";
    };
    std::stringstream ss;
    ss << "digraph " << quote(net.name.empty() ? "markings" : net.name) << " {\n"
       << "  node [shape=box, style=rounded, fontname=\"monospace\"];\n"
       << "  // Places: ";
    for (auto const& p : net.places())
        ss << p.key << (p.caption.empty() || (p.caption == p.key) ? "" : "=" + p.caption) << " ";
    ss << "\n";
    for (size_t s = 0u; s < states.size(); ++s)
    {
        std::string label;
        for (size_t const n : states[s])
            label += (label.empty() ? "" : " ") + ((n == OMEGA) ? std::string("w") : std::to_string(n));
        ss << "  M" << s << " [label=" << quote(label);
        if (s == 0u)
            ss << ", penwidth=2";
        if (leaves[s])
            ss << ", color=red, fontcolor=red";
        ss << "];\n";
    }
    for (auto const& e : edges)
    {
        Transition const& t = net.transitions()[e.transition];
        ss << "  M" << e.from << " -> M" << e.to << " [label=" << quote(nameOf(t)) << "];\n";
    }
    ss << "}\n";
    return ss.str();
}

//------------------------------------------------------------------------------
//! \brief "2 P0 + P1" for the invariant x over the nodes.
//------------------------------------------------------------------------------
template<class Nodes>
static std::string sum(Nodes const& nodes, std::vector<int64_t> const& x)
{
    std::string s;
    for (size_t i = 0u; i < x.size(); ++i)
    {
        if (x[i] == 0)
            continue;
        if (!s.empty())
            s += " + ";
        if (x[i] != 1)
            s += std::to_string(x[i]) + " ";
        s += nameOf(nodes[i]);
    }
    return s;
}

//------------------------------------------------------------------------------
std::string report(Net const& net, size_t const max_states)
{
    std::stringstream ss;
    Properties const prop = properties(net, max_states);

    auto transitions = [&net](std::vector<size_t> const& ids) {
        std::string s;
        for (size_t t: ids)
            s += (s.empty() ? "" : ", ") + nameOf(net.transitions()[t]);
        return s;
    };

    ss << "Net: " << (net.name.empty() ? "unnamed" : net.name) << " ("
       << to_str(net.type()) << ", " << net.places().size() << " places, "
       << net.transitions().size() << " transitions)\n";
    ss << "Reachable markings: " << prop.states << (prop.complete ? "" : " (stopped)") << "\n";

    ss << "Deadlock: ";
    if (!prop.deadlock)
        ss << (prop.complete ? "none" : "none found");
    else
    {
        ss << "yes, marking (";
        for (size_t p = 0u; p < prop.deadlock_marking.size(); ++p)
            ss << (p ? ", " : "") << net.places()[p].key << "=" << prop.deadlock_marking[p];
        ss << ")";
        if (prop.deadlock_path.empty())
            ss << " initially";
        else
        {
            // Successive firings of a transition: "T5 x4".
            ss << " after firing ";
            std::vector<size_t> const& path = prop.deadlock_path;
            for (size_t i = 0u, n; i < path.size(); i += n)
            {
                for (n = 1u; (i + n < path.size()) && (path[i + n] == path[i]); ++n) {}
                ss << (i ? ", " : "") << nameOf(net.transitions()[path[i]]);
                if (n > 1u)
                    ss << " x" << n;
            }
        }
    }
    ss << "\n";

    ss << "Dead transitions: " << (prop.dead_transitions.empty() ? "none" : transitions(prop.dead_transitions)) << "\n";

    ss << "Bounded: ";
    if (prop.bounded)
    {
        size_t const k = prop.bounds.empty() ? 0u : *std::max_element(prop.bounds.begin(), prop.bounds.end());
        ss << "yes (" << k << "-bounded)";
    }
    else
    {
        std::string unbounded;
        for (size_t p = 0u; p < prop.bounds.size(); ++p)
        {
            if (prop.bounds[p] == OMEGA)
                unbounded += (unbounded.empty() ? "" : ", ") + nameOf(net.places()[p]);
        }
        ss << (unbounded.empty() ? "unknown" : "no, unbounded: " + unbounded);
    }
    ss << "\n";

    ss << "Live: ";
    if (!prop.complete)
        ss << "unknown";
    else if (prop.live)
        ss << "yes";
    else
        ss << "no, not live: " << transitions(prop.not_live);
    ss << "\n";

    std::string error;
    Marking const m0 = net.tokens();
    auto const pinv = pInvariants(net, error);
    ss << "P-invariants:" << (pinv.empty() ? " none" : "") << "\n";
    for (auto const& y: pinv)
    {
        int64_t c = 0;
        for (size_t p = 0u; p < y.size(); ++p)
            c += y[p] * static_cast<int64_t>(m0[p]);
        ss << "  " << sum(net.places(), y) << " = " << c << "\n";
    }
    auto const tinv = tInvariants(net, error);
    ss << "T-invariants:" << (tinv.empty() ? " none" : "") << "\n";
    for (auto const& x: tinv)
        ss << "  " << sum(net.transitions(), x) << "\n";

    std::vector<std::string> notes = prop.notes;
    if (!error.empty())
        notes.push_back(error);
    for (auto const& note: notes)
        ss << "Note: " << note << "\n";
    return ss.str();
}

} // namespace tpne
