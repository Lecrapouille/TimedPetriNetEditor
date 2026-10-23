// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "PetriNet/Exports/Exports.hpp"
#include "PetriNet/PetriNet.hpp"
#include <fstream>
#include <cstring>
#include <sstream>
#include <set>
#include <vector>
#include <algorithm>
#include <queue>

namespace tpne {

//------------------------------------------------------------------------------
static std::string escapeLaTeX(std::string const& s)
{
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s)
    {
        switch (c)
        {
            case '\\': out += "\\textbackslash{}"; break;
            case '&':  out += "\\&"; break;
            case '%':  out += "\\%"; break;
            case '$':  out += "\\$"; break;
            case '#':  out += "\\#"; break;
            case '_':  out += "\\_"; break;
            case '{':  out += "\\{"; break;
            case '}':  out += "\\}"; break;
            case '~':  out += "\\textasciitilde{}"; break;
            case '^':  out += "\\^{}"; break;
            default:   out += c; break;
        }
    }
    return out;
}

//------------------------------------------------------------------------------
struct BackEdge
{
    std::string fromKey;
    std::string toKey;
};

//------------------------------------------------------------------------------
struct GrafcetContext
{
    Net const& net;
    std::set<std::string> generated;
    std::set<std::string> loopTargets;
    std::set<std::string> convergenceTransitions;
    std::vector<BackEdge> backEdges;
    std::stringstream output;
    int indentLevel = 0;

    explicit GrafcetContext(Net const& n) : net(n) {}

    std::string indent() const
    {
        return std::string(static_cast<size_t>(indentLevel * 2), ' ');
    }

    bool isBackEdgeArc(std::string const& from, std::string const& to) const
    {
        for (auto const& be : backEdges)
        {
            if (be.fromKey == from && be.toKey == to)
                return true;
        }
        return false;
    }
};

//------------------------------------------------------------------------------
static Place const* findInitialPlace(Net const& net)
{
    for (auto const& p : net.places())
    {
        if (p.tokens > 0u)
            return &p;
    }
    return nullptr;
}

//------------------------------------------------------------------------------
static std::string getStepNumber(Place const& p)
{
    if (!p.caption.empty())
        return p.caption;
    return std::to_string(p.id);
}

//------------------------------------------------------------------------------
// DFS pre-pass to detect all back edges (arcs to an ancestor in the DFS tree)
//------------------------------------------------------------------------------
static void detectBackEdgesDFS(
    Net const& net,
    Node const& node,
    std::set<std::string>& visited,
    std::set<std::string>& inStack,
    std::vector<BackEdge>& backEdges,
    std::set<std::string>& loopTargets)
{
    visited.insert(node.key);
    inStack.insert(node.key);

    for (auto const& arc : net.arcsOut(node))
    {
        if (inStack.count(arc.to.key()) > 0)
        {
            backEdges.push_back({node.key, arc.to.key()});
            loopTargets.insert(arc.to.key());
        }
        else if (visited.count(arc.to.key()) == 0)
        {
            detectBackEdgesDFS(net, net.node(arc.to), visited, inStack, backEdges, loopTargets);
        }
    }

    inStack.erase(node.key);
}

//------------------------------------------------------------------------------
// BFS to find the common convergence point for a Fork's branches
//------------------------------------------------------------------------------
static std::string findForkConvergence(
    Net const& net,
    Transition const& forkTrans,
    std::set<std::string> const& convergenceTransitions)
{
    auto const branches = net.arcsOut(forkTrans);
    if (branches.size() < 2)
        return "";

    std::vector<std::set<std::string>> reachable(branches.size());

    for (size_t i = 0; i < branches.size(); ++i)
    {
        std::queue<Node const*> q;
        std::set<std::string> seen;
        q.push(&net.node(branches[i].to));
        seen.insert(branches[i].to.key());

        while (!q.empty())
        {
            Node const* cur = q.front();
            q.pop();

            if (convergenceTransitions.count(cur->key) > 0)
            {
                reachable[i].insert(cur->key);
                continue;
            }

            for (auto const& a : net.arcsOut(*cur))
            {
                if (seen.insert(a.to.key()).second)
                    q.push(&net.node(a.to));
            }
        }
    }

    std::set<std::string> common = reachable[0];
    for (size_t i = 1; i < reachable.size(); ++i)
    {
        std::set<std::string> inter;
        std::set_intersection(
            common.begin(), common.end(),
            reachable[i].begin(), reachable[i].end(),
            std::inserter(inter, inter.begin()));
        common = std::move(inter);
    }

    if (common.empty())
        return "";
    return *common.begin();
}

//------------------------------------------------------------------------------
// Check if a branch from 'node' reaches a back edge to 'targetKey',
// without following any back edges (to avoid traversing unrelated loops)
//------------------------------------------------------------------------------
static bool branchHasBackEdgeTo(
    Net const& net,
    Node const& node,
    std::string const& targetKey,
    std::vector<BackEdge> const& backEdges,
    std::set<std::string>& visited)
{
    if (visited.count(node.key) > 0)
        return false;
    visited.insert(node.key);

    for (auto const& be : backEdges)
    {
        if (be.fromKey == node.key && be.toKey == targetKey)
            return true;
    }

    for (auto const& arc : net.arcsOut(node))
    {
        bool isAnyBackEdge = false;
        for (auto const& be : backEdges)
        {
            if (be.fromKey == node.key && be.toKey == arc.to.key())
            {
                isAnyBackEdge = true;
                break;
            }
        }
        if (isAnyBackEdge)
            continue;

        if (branchHasBackEdgeTo(net, net.node(arc.to), targetKey, backEdges, visited))
            return true;
    }
    return false;
}

//------------------------------------------------------------------------------
static void generateFromPlace(Place const& place, GrafcetContext& ctx,
                              std::string const& stopAtKey);
static void generateFromTransition(Transition const& trans, GrafcetContext& ctx,
                                   std::string const& stopAtKey);

//------------------------------------------------------------------------------
static void generateFromTransition(Transition const& trans, GrafcetContext& ctx,
                                   std::string const& stopAtKey)
{
    if (trans.key == stopAtKey)
        return;
    if (ctx.generated.count(trans.key) > 0)
        return;

    ctx.generated.insert(trans.key);
    ctx.output << ctx.indent() << "\\Transition{" << escapeLaTeX(trans.caption) << "}\n";

    auto const outputs = ctx.net.arcsOut(trans);
    if (outputs.size() == 1)
    {
        auto const& next = ctx.net.node(outputs[0].to);
        if (!ctx.isBackEdgeArc(trans.key, next.key))
        {
            generateFromPlace(
                static_cast<Place const&>(next), ctx, stopAtKey);
        }
    }
    else if (outputs.size() > 1)
    {
        // Fork (ET divergence): transition with multiple output arcs
        std::string convKey = findForkConvergence(
            ctx.net, trans, ctx.convergenceTransitions);
        std::string branchStop = convKey.empty() ? stopAtKey : convKey;

        ctx.output << ctx.indent() << "\\Fork{\n";
        ctx.indentLevel++;

        std::set<std::string> allGenerated;
        bool first = true;
        for (auto const& arc : outputs)
        {
            if (!first)
            {
                ctx.indentLevel--;
                ctx.output << ctx.indent() << "}{\n";
                ctx.indentLevel++;
            }
            first = false;

            std::set<std::string> saved = ctx.generated;
            generateFromPlace(ctx.net.placeOf(arc), ctx, branchStop);
            for (auto const& key : ctx.generated)
                allGenerated.insert(key);
            ctx.generated = saved;
        }

        ctx.indentLevel--;
        ctx.output << ctx.indent() << "}\n";
        ctx.generated = allGenerated;

        if (!convKey.empty())
        {
            Node const* convNode = ctx.net.findNode(convKey);
            if (convNode != nullptr)
            {
                generateFromTransition(
                    static_cast<Transition const&>(*convNode),
                    ctx, stopAtKey);
            }
        }
    }
}

//------------------------------------------------------------------------------
static void generateFromPlace(Place const& place, GrafcetContext& ctx,
                              std::string const& stopAtKey)
{
    if (place.key == stopAtKey)
        return;
    if (ctx.generated.count(place.key) > 0)
        return;

    ctx.generated.insert(place.key);

    bool isLoopTarget = (ctx.loopTargets.count(place.key) > 0);
    auto const outputs = ctx.net.arcsOut(place);
    bool hasDivergence = (outputs.size() > 1);

    // For a loop target with divergence, check which branches loop back
    std::vector<bool> branchLoops;
    int loopingCount = 0;

    if (isLoopTarget && hasDivergence)
    {
        branchLoops.resize(outputs.size(), false);
        for (size_t i = 0; i < outputs.size(); ++i)
        {
            std::set<std::string> v;
            if (branchHasBackEdgeTo(
                    ctx.net, ctx.net.node(outputs[i].to), place.key,
                    ctx.backEdges, v))
            {
                branchLoops[i] = true;
                loopingCount++;
            }
        }
    }

    // Wrap the whole block in \Loop{} when:
    //  - loop target with no divergence (simple sequence loop), OR
    //  - loop target with divergence where ALL branches loop back
    bool wrapAll = isLoopTarget &&
        (!hasDivergence ||
         loopingCount == static_cast<int>(outputs.size()));

    if (wrapAll)
    {
        ctx.output << ctx.indent() << "\\Loop{\n";
        ctx.indentLevel++;
    }

    if (place.tokens > 0)
        ctx.output << ctx.indent() << "\\FirstStep["
                   << getStepNumber(place) << "]{}\n";
    else
        ctx.output << ctx.indent() << "\\Step["
                   << getStepNumber(place) << "]{}\n";

    if (outputs.size() == 1)
    {
        auto const& next = ctx.net.node(outputs[0].to);
        if (next.key != stopAtKey)
        {
            generateFromTransition(
                static_cast<Transition const&>(next), ctx, stopAtKey);
        }
    }
    else if (hasDivergence)
    {
        ctx.output << ctx.indent() << "\\Divergence{\n";
        ctx.indentLevel++;

        std::set<std::string> allGenerated;
        bool first = true;
        for (size_t i = 0; i < outputs.size(); ++i)
        {
            if (!first)
            {
                ctx.indentLevel--;
                ctx.output << ctx.indent() << "}{\n";
                ctx.indentLevel++;
            }
            first = false;

            // Wrap only the looping branch(es) in \Loop{} when not
            // all branches loop (partial loop in divergence)
            bool wrapBranch = !wrapAll &&
                !branchLoops.empty() && branchLoops[i];

            if (wrapBranch)
            {
                ctx.output << ctx.indent() << "\\Loop{\n";
                ctx.indentLevel++;
            }

            std::set<std::string> saved = ctx.generated;
            generateFromTransition(ctx.net.transitionOf(outputs[i]), ctx, stopAtKey);
            for (auto const& key : ctx.generated)
                allGenerated.insert(key);
            ctx.generated = saved;

            if (wrapBranch)
            {
                ctx.indentLevel--;
                ctx.output << ctx.indent() << "}\n";
            }
        }

        ctx.indentLevel--;
        ctx.output << ctx.indent() << "}\n";
        ctx.generated = allGenerated;
    }

    if (wrapAll)
    {
        ctx.indentLevel--;
        ctx.output << ctx.indent() << "}\n";
    }
}

//------------------------------------------------------------------------------
std::string exportToGrafcetLaTeX(Net const& net, std::string const& filename)
{
    if (net.type() != TypeOfNet::GRAFCET)
        return "Export GRAFCET LaTeX: the net must be of type GRAFCET";

    if (net.isEmpty())
        return "Export GRAFCET LaTeX: cannot export empty net";

    Place const* init = findInitialPlace(net);
    if (!init)
        return "Export GRAFCET LaTeX: no initial step (step with token) found";

    std::ofstream file(filename);
    if (!file)
    {
        std::stringstream error;
        error << "Failed to export the GRAFCET to '" << filename
              << "'. Reason: " << strerror(errno) << std::endl;
        return error.str();
    }

    GrafcetContext ctx(net);

    {
        std::set<std::string> visited, inStack;
        detectBackEdgesDFS(net, *init, visited, inStack,
                           ctx.backEdges, ctx.loopTargets);
    }

    for (auto const& t : net.transitions())
    {
        if (net.arcsIn(t).size() > 1)
            ctx.convergenceTransitions.insert(t.key);
    }

    generateFromPlace(*init, ctx, "");

    file << R"(%% GRAFCET diagram generated by TimedPetriNetEditor
%% Using grafcet2 package: https://github.com/nagimov/grafcet2
%%
%% To compile: pdflatex <filename>.tex
%% Note: You need grafcet2.sty in your working directory or LaTeX path
%%
\documentclass[a4paper]{article}
\usepackage{grafcet2}
\usepackage[margin=2cm]{geometry}

\begin{document}

\begin{grafcet}
)" << ctx.output.str()
       << R"(\end{grafcet}

\end{document}
)";

    return {};
}

} // namespace tpne
