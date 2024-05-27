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

#include "Imports.hpp"
#include "TimedPetriNetEditor/PetriNet.hpp"
#include <sstream>
#include <fstream>
#include <cstring>

namespace tpne {

//------------------------------------------------------------------------------
std::vector<std::string> splitLine(std::ifstream& file)
{
    std::string s;

    std::getline(file, s);
    std::cout << "Line " << s << std::endl;
    std::stringstream ss(s);
    std::vector<std::string> v;

    while (std::getline(ss, s))
    {
        std::cout << s << std::endl;
        v.push_back(s);
    }

    return v;
}

//------------------------------------------------------------------------------
std::string importFlowshop(Net& net, std::string const& filename)
{
    struct DataMatrix
    {
        std::vector<std::string> columnNames;
        std::vector<std::string> rowNames;
        std::vector<std::vector<float>> data;
    };

    DataMatrix matrix;
    std::stringstream error;

    // Check if file exists
    std::ifstream file(filename);
    if (!file)
    {
        error << "Failed opening '" << filename << "'. Reason was '"
            << strerror(errno) << "'" << std::endl;
        return error.str();
    }

    // Extract number of transitions and number of lines
    size_t lines, rows;
    std::string type;

    if (!(file >> type >> rows >> lines))
    {
        error << "Malformed header. Needed 'Flowshop number_transitions number_lines'"
            << std::endl;
        return error.str();
    }
    if (type != "Flowshop")
    {
        error << "Malformed token. Expected to extract token 'TimedEventGraph'"
            << std::endl;
        return error.str();
    }

    // windows screen.
    // FIXME: get the exact dimension Editor::viewSize()
    // FIXME: initial frame iteration: the screen size is not at its final size
    const size_t w = 600u; const size_t h = 600u;
    const size_t margin = 50u;
    // Since the file does not give position, we place them as square
    size_t dx = (w - 2u * margin) / rows;
    size_t dy = (h - 2u * margin) / lines;
    size_t x = margin + dx; size_t y = margin + dy;

    size_t id = 0u;
    std::string line;

    // End the current line
    getline(file, line);

    // Read the column names
    if (getline(file, line))
    {
        std::istringstream columnNamesStream(line);
        std::string columnName;
        while (columnNamesStream >> columnName)
        {
            matrix.columnNames.push_back(columnName);
        }
    }

    // Read the following lines to obtain the row names and the data
    while (getline(file, line))
    {
        std::istringstream lineStream(line);
        std::string rowName;
        if (lineStream >> rowName)
        {
            matrix.rowNames.push_back(rowName);

            std::vector<float> row;
            std::string value;
            while (lineStream >> value)
            {
                row.push_back(stof(value));

                if (value != "nan")
                {
                    net.addPlace(id, Transition::to_str(id), float(x), float(y), 0);
                    id++;
                }
                x += dx;
            }
            matrix.data.push_back(row);
        }
        else
        {
            error << "Malformed line '" << line << "'" << std::endl;
            return error.str(); 
        }
        x = margin + dx;
        y += dy;
    }

    //float ymax = float(y);
    //float xmax = float(margin + dx + dx * rows);

    // Place this code outside the getline() loop to have id of internal transitions
    // starting from 0.
    x = margin + dx - dx / 2u; y = margin;
    for (const auto& columnName : matrix.columnNames)
    {
        net.addPlace(id++, columnName, float(x), float(y), 0);
        //net.addPlace(id++, columnName, x, ymax, 0);
        x += dx;
    }

    x = margin; y = margin + dy + dy / 2u;
    for (const auto& rowName : matrix.rowNames)
    {
        net.addPlace(id++, rowName, float(x), float(y), 0);
        //net.addPlace(id++, rowName, xmax, y, 0);
        y += dy;
    }

    return {};
}

} // namespace tpne
