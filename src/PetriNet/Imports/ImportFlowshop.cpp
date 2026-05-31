//=============================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2026 Quentin Quadrat <lecrapouille@gmail.com>
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

#include "Imports.hpp"
#include "PetriNet/PetriNet.hpp"
#include "PetriNet/SafeFloat.hpp"
#include <sstream>
#include <fstream>
#include <cstring>
#include <cmath>
#include <limits>

namespace tpne {

//------------------------------------------------------------------------------
struct FlowshopData
{
    size_t npieces = 0;
    size_t nmachines = 0;
    std::vector<std::string> pieceNames;
    std::vector<std::string> machineNames;
    std::vector<size_t> np;  // tokens per piece (vertical cycle)
    std::vector<size_t> nm;  // tokens per machine (horizontal cycle)
    std::vector<std::vector<float>> PT;  // PT[machine][piece], epsilon = no operation
};

//------------------------------------------------------------------------------
//! \brief (max,+) epsilon: the neutral element of the ⊕ (max) operator, i.e.
//! Scilab's %0 == -inf (see zero<MaxPlus>() in TropicalAlgebra.hpp). A cell
//! holding epsilon means "no operation" (a hole) for that machine/piece.
static inline float epsilonMaxPlus()
{
    return safeNegInfF();
}

//------------------------------------------------------------------------------
//! \brief Test whether a processing time is the (max,+) epsilon (== %0), i.e. a
//! hole imposed in the flowshop grid. Both -inf (%0) and any NaN (legacy file
//! convention) are treated as epsilon. Uses the -ffast-math-safe helpers from
//! SafeFloat.hpp (see that file for why std::isnan/std::isinf cannot be used).
static inline bool isEpsilonMaxPlus(float value)
{
    return safeIsNegInf(value) || safeIsNaN(value);
}

//------------------------------------------------------------------------------
static std::string trim(std::string const& str)
{
    size_t first = str.find_first_not_of(" \t");
    if (first == std::string::npos)
        return "";
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, last - first + 1);
}

//------------------------------------------------------------------------------
static std::vector<std::string> splitBySpace(std::string const& str)
{
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;
    while (iss >> token)
    {
        tokens.push_back(token);
    }
    return tokens;
}

//------------------------------------------------------------------------------
static std::string parseFlowshopFile(std::ifstream& file, FlowshopData& data)
{
    std::string line;
    std::stringstream error;

    while (std::getline(file, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;

        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos)
        {
            error << "Malformed line (missing ':'): " << line << std::endl;
            return error.str();
        }

        std::string key = trim(line.substr(0, colonPos));
        std::string value = trim(line.substr(colonPos + 1));

        if (key == "npieces")
        {
            data.npieces = std::stoul(value);
        }
        else if (key == "nmachines")
        {
            data.nmachines = std::stoul(value);
        }
        else if (key == "nm")
        {
            auto tokens = splitBySpace(value);
            for (auto const& t : tokens)
            {
                data.nm.push_back(std::stoul(t));
            }
        }
        else if (key == "np")
        {
            auto tokens = splitBySpace(value);
            for (auto const& t : tokens)
            {
                data.np.push_back(std::stoul(t));
            }
        }
        else if (key == "pieces")
        {
            data.pieceNames = splitBySpace(value);
        }
        else
        {
            // Machine line: MachineName: val1 val2 val3 ...
            data.machineNames.push_back(key);
            auto tokens = splitBySpace(value);
            std::vector<float> row;
            for (auto const& t : tokens)
            {
                if ((t == "nan") || (t == "-inf") || (t == "%0") || (t == "eps"))
                {
                    // (max,+) zero %0 == -inf: marks a hole (no operation).
                    row.push_back(epsilonMaxPlus());
                }
                else
                {
                    row.push_back(std::stof(t));
                }
            }
            data.PT.push_back(row);
        }
    }

    // Validation
    if (data.npieces == 0 || data.nmachines == 0)
    {
        error << "Missing npieces or nmachines" << std::endl;
        return error.str();
    }
    if (data.nm.size() != data.nmachines)
    {
        error << "nm size (" << data.nm.size() << ") != nmachines (" << data.nmachines << ")" << std::endl;
        return error.str();
    }
    if (data.np.size() != data.npieces)
    {
        error << "np size (" << data.np.size() << ") != npieces (" << data.npieces << ")" << std::endl;
        return error.str();
    }
    if (data.pieceNames.size() != data.npieces)
    {
        error << "pieces count (" << data.pieceNames.size() << ") != npieces (" << data.npieces << ")" << std::endl;
        return error.str();
    }
    if (data.PT.size() != data.nmachines)
    {
        error << "Machine rows (" << data.PT.size() << ") != nmachines (" << data.nmachines << ")" << std::endl;
        return error.str();
    }
    for (size_t m = 0; m < data.nmachines; ++m)
    {
        if (data.PT[m].size() != data.npieces)
        {
            error << "Machine " << m << " has " << data.PT[m].size() << " values, expected " << data.npieces << std::endl;
            return error.str();
        }
    }

    return {};
}

//------------------------------------------------------------------------------
// Faithful port of ScicosLab flowshop_graph.sci (and of its Julia port
// MaxPlus.jl src/flowshop.jl `flowshop_graph`). Every node `nd` of the Scilab
// event graph becomes a Transition, and every arc (hc -> nd) carrying a date
// T(hc,nd) and a token count N(hc,nd) becomes a Place between the two
// transitions (created by net.addArc(Transition&, Transition&, tokens,
// duration)). Token/duration follow the Scilab convention: %1 -> 0 (one
// neutral element), the buffer feeds carry p(i)/m(j) tokens.
//
// Node numbering is 1-based (like Scilab). PT[machine][piece], %0 (-inf, or the
// legacy "nan") = epsilon = no task.
static void buildFlowshopPetriNet(Net& net, FlowshopData const& data)
{
    const size_t nmach = data.nmachines;
    const size_t npiece = data.npieces;

    // Layout: operations on a machine(row) x piece(col) grid; buffers around it.
    const float dx = 220.0f;     // horizontal spacing (pieces / columns)
    const float dy = 160.0f;     // vertical spacing (machines / rows)
    const float margin = 120.0f; // margin from origin (room for buffers)

    // Push the input (buffer) and output (closing) nodes off the operation grid
    // lines so the long loop-back arcs (out_* -> buffer) do not run over the
    // internal operation nodes. A piece loop-back is vertical, so its endpoints
    // are shifted along X; a machine loop-back is horizontal, so its endpoints
    // are shifted along Y. The input and output of the same piece/machine share
    // the same shift, hence stay aligned with each other.
    const float loopX = 0.40f * dx; // piece buffer/closing: clears the column
    const float loopY = 0.45f * dy; // machine buffer/closing: clears the row

    // A[machine][piece] = 1-based node id of the operation, 0 if ε / none.
    std::vector<std::vector<size_t>> A(nmach, std::vector<size_t>(npiece, 0u));
    std::vector<size_t> l(nmach, 0u);   // ε run-length on the current machine row
    std::vector<size_t> d(npiece, 0u);  // ε run-length on the current piece column
    std::vector<size_t> bp(npiece, 0u); // piece buffer node id
    std::vector<size_t> bm(nmach, 0u);  // machine buffer node id

    struct NodeInfo { float x; float y; std::string caption; };
    struct Edge { size_t from; size_t to; float duration; size_t tokens; };
    std::vector<NodeInfo> nodes; // index = (node id - 1)
    std::vector<Edge> edges;
    size_t nd = 0u;

    auto newNode = [&](float x, float y, std::string const& caption) -> size_t {
        nodes.push_back({ x, y, caption });
        return ++nd; // 1-based id
    };

    for (size_t i = 0; i < npiece; ++i)
    {
        for (size_t j = 0; j < nmach; ++j)
        {
            if (isEpsilonMaxPlus(data.PT[j][i]))
            {
                A[j][i] = 0u;
                if (l[j] != 0u) l[j] += 1u;
                if (d[i] != 0u) d[i] += 1u;
            }
            else
            {
                size_t cur = newNode(margin + float(i) * dx,
                                     margin + float(j) * dy,
                                     data.machineNames[j] + "_" + data.pieceNames[i]);
                A[j][i] = cur;

                // Horizontal predecessor (same machine, previous used piece).
                if (l[j] != 0u)
                {
                    size_t hc = A[j][i - l[j]];
                    edges.push_back({ hc, cur, data.PT[j][i - l[j]], 0u });
                }
                // Vertical predecessor (same piece, previous used machine).
                if (d[i] != 0u)
                {
                    size_t hc = A[j - d[i]][i];
                    edges.push_back({ hc, cur, data.PT[j - d[i]][i], 0u });
                }
                // First operation of this piece: create its buffer feed (np
                // tokens). Shifted by loopX off the operation column (kept
                // aligned with the closing node "out_<piece>").
                if (d[i] == 0u)
                {
                    size_t b = newNode(margin + float(i) * dx - loopX,
                                       margin - dy,
                                       "np_" + data.pieceNames[i]);
                    bp[i] = b;
                    edges.push_back({ b, cur, 0.0f, data.np[i] });
                }
                d[i] = 1u;
                // First operation of this machine: create its buffer feed (nm
                // tokens). Shifted by loopY off the operation row (kept aligned
                // with the closing node "out_<machine>").
                if (l[j] == 0u)
                {
                    size_t b = newNode(margin - dx,
                                       margin + float(j) * dy - loopY,
                                       "nm_" + data.machineNames[j]);
                    bm[j] = b;
                    edges.push_back({ b, cur, 0.0f, data.nm[j] });
                }
                l[j] = 1u;
            }
        }

        // Closing node of piece i: last_op -> closing -> buffer (loopback).
        if (d[i] != 0u)
        {
            size_t lastMachine = nmach - d[i]; // 0-based: Scilab A(nmach-d(i)+1, i)
            size_t hc = A[lastMachine][i];
            size_t cp = newNode(margin + float(i) * dx - loopX,
                                margin + float(nmach) * dy,
                                "out_" + data.pieceNames[i]);
            edges.push_back({ hc, cp, data.PT[lastMachine][i], 0u });
            edges.push_back({ cp, bp[i], 0.0f, 0u });
        }
    }

    for (size_t j = 0; j < nmach; ++j)
    {
        // Closing node of machine j: last_op -> closing -> buffer (loopback).
        if (l[j] != 0u)
        {
            size_t lastPiece = npiece - l[j]; // 0-based: Scilab A(j, npiece-l(j)+1)
            size_t hc = A[j][lastPiece];
            size_t cm = newNode(margin + float(npiece) * dx,
                                margin + float(j) * dy - loopY,
                                "out_" + data.machineNames[j]);
            edges.push_back({ hc, cm, data.PT[j][lastPiece], 0u });
            edges.push_back({ cm, bm[j], 0.0f, 0u });
        }
    }

    // Materialize the graph: one Transition per node, one Place per edge.
    std::vector<Transition*> trans(nd + 1u, nullptr); // 1-based indexing
    for (size_t k = 0; k < nd; ++k)
    {
        Transition& t = net.addTransition(nodes[k].x, nodes[k].y);
        t.caption = nodes[k].caption;
        trans[k + 1u] = &t;
    }
    for (auto const& e : edges)
    {
        if ((e.from == 0u) || (e.to == 0u))
            continue; // defensive: skip degenerate edges
        net.addArc(*trans[e.from], *trans[e.to], e.tokens, e.duration);
    }
}

//------------------------------------------------------------------------------
std::string importFlowshop(Net& net, std::string const& filename)
{
    std::stringstream error;

    std::ifstream file(filename);
    if (!file)
    {
        error << "Failed opening '" << filename << "'. Reason: '"
            << strerror(errno) << "'" << std::endl;
        return error.str();
    }

    net.reset(TypeOfNet::TimedEventGraph);

    FlowshopData data;
    std::string parseError = parseFlowshopFile(file, data);
    if (!parseError.empty())
    {
        return parseError;
    }

    buildFlowshopPetriNet(net, data);

    return {};
}

} // namespace tpne
