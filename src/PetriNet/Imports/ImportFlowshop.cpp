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
#include <sstream>
#include <fstream>
#include <cstring>
#include <cmath>

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
    std::vector<std::vector<float>> PT;  // PT[machine][piece], nan = no operation
};

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
                if (t == "nan" || t == "-inf")
                {
                    row.push_back(std::numeric_limits<float>::quiet_NaN());
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
static void buildFlowshopPetriNet(Net& net, FlowshopData const& data)
{
    const float dx = 300.0f;  // horizontal spacing (pieces)
    const float dy = 300.0f;  // vertical spacing (machines)
    const float margin = 50.0f;  // margin from origin
    const float loopbackOffsetX = -100.0f;  // offset for nm loopback places (left of grid)
    const float loopbackOffsetY = 100.0f;   // offset for np loopback places (below grid)

    // Grid of transitions T[machine][piece]
    // Use nullptr to indicate no transition at this position (nan in PT)
    std::vector<std::vector<Transition*>> T(data.nmachines,
        std::vector<Transition*>(data.npieces, nullptr));

    // 1. Create transitions for each valid operation (PT[m][p] != nan)
    for (size_t m = 0; m < data.nmachines; ++m)
    {
        for (size_t p = 0; p < data.npieces; ++p)
        {
            if (!std::isnan(data.PT[m][p]))
            {
                float x = margin + float(p) * dx;
                float y = margin + float(m) * dy;
                std::string caption = data.machineNames[m] + "_" + data.pieceNames[p];
                Transition& t = net.addTransition(x, y);
                t.caption = caption;
                T[m][p] = &t;
            }
        }
    }

    // 2. Horizontal arcs (machine cycle through pieces) - yellow in Scilab
    //    For each machine m: connect valid pieces in sequence, loopback with nm[m] tokens
    for (size_t m = 0; m < data.nmachines; ++m)
    {
        std::vector<size_t> validPieces;
        for (size_t p = 0; p < data.npieces; ++p)
        {
            if (T[m][p] != nullptr)
            {
                validPieces.push_back(p);
            }
        }

        if (validPieces.size() < 2)
            continue;

        // Connect consecutive valid pieces (internal arcs, no tokens)
        for (size_t i = 0; i < validPieces.size() - 1; ++i)
        {
            size_t p1 = validPieces[i];
            size_t p2 = validPieces[i + 1];
            float duration = data.PT[m][p1];
            net.addArc(*T[m][p1], *T[m][p2], 0u, duration);
        }

        // Loopback arc: last -> first with nm[m] tokens
        // Place the loopback place to the LEFT of the grid (offset in X)
        size_t pFirst = validPieces.front();
        size_t pLast = validPieces.back();
        float duration = data.PT[m][pLast];
        float loopX = margin + loopbackOffsetX;  // left of grid
        float loopY = margin + float(m) * dy;    // same row as machine m
        Place& loopPlace = net.addPlace(loopX, loopY, data.nm[m]);
        loopPlace.caption = "nm_" + data.machineNames[m];
        net.addArc(*T[m][pLast], loopPlace, duration);
        net.addArc(loopPlace, *T[m][pFirst]);
    }

    // 3. Vertical arcs (piece cycle through machines) - blue in Scilab
    //    For each piece p: connect valid machines in sequence, loopback with np[p] tokens
    for (size_t p = 0; p < data.npieces; ++p)
    {
        std::vector<size_t> validMachines;
        for (size_t m = 0; m < data.nmachines; ++m)
        {
            if (T[m][p] != nullptr)
            {
                validMachines.push_back(m);
            }
        }

        if (validMachines.size() < 2)
            continue;

        // Connect consecutive valid machines (internal arcs, no tokens)
        for (size_t i = 0; i < validMachines.size() - 1; ++i)
        {
            size_t m1 = validMachines[i];
            size_t m2 = validMachines[i + 1];
            float duration = data.PT[m1][p];
            net.addArc(*T[m1][p], *T[m2][p], 0u, duration);
        }

        // Loopback arc: last -> first with np[p] tokens
        // Place the loopback place BELOW the grid (offset in Y)
        size_t mFirst = validMachines.front();
        size_t mLast = validMachines.back();
        float duration = data.PT[mLast][p];
        float loopX = margin + float(p) * dx;  // same column as piece p
        float loopY = margin + float(data.nmachines - 1) * dy + loopbackOffsetY;  // below grid
        Place& loopPlace = net.addPlace(loopX, loopY, data.np[p]);
        loopPlace.caption = "np_" + data.pieceNames[p];
        net.addArc(*T[mLast][p], loopPlace, duration);
        net.addArc(loopPlace, *T[mFirst][p]);
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

    net.reset(TypeOfNet::TimedPetriNet);

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
