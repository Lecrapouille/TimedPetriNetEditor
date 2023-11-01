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

#include "Exports.hpp"
#include "TimedPetriNetEditor/PetriNet.hpp"
#include <fstream>
#include <cstring>

namespace tpne {

//------------------------------------------------------------------------------
//! \brief Write int32_t as little endian
template<class T>
static void write_int32(std::ofstream& file, T const val)
{
    int32_t d = int32_t(val);

    file.put(char(d));
    file.put(char(d >> 8));
    file.put(char(d >> 16));
    file.put(char(d >> 24));
}

//------------------------------------------------------------------------------
template<class T>
static void write_float32(std::ofstream& file, T const val)
{
    float d = float(val);

    file.write(reinterpret_cast<const char*>(&d), sizeof(float));
}

//------------------------------------------------------------------------------
std::string exportToPNEditor(Net const& net, std::string const& filename)
{
    // .pns file: contains the logical contents of the petri net
    {
        std::string filename_pns(filename.substr(0, filename.find_last_of('.')) + ".pns");
        std::ofstream file(filename_pns, std::ios::out | std::ios::binary);
        if (!file)
        {
            std::stringstream error;
            error << "Failed to export the Petri net to '" << filename_pns
                  << "'. Reason was " << strerror(errno) << std::endl;
            return error.str();
        }

        // Places
        write_int32(file, net.places().size());
        for (auto const& p: net.places())
        {
            write_int32(file, p.tokens);
        }

        // Transitions
        write_int32(file, net.transitions().size());
        for (auto const& t: net.transitions())
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
            std::stringstream error;
            error << "Failed to export the Petri net to '" << filename_pnl
                  << "'. Reason was " << strerror(errno) << std::endl;
            return error.str();
        }

        for (auto const& t: net.transitions())
        {
            write_float32(file, t.x);
            write_float32(file, t.y);
        }

        for (auto const& p: net.places())
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
            std::stringstream error;
            error << "Failed to export the Petri net to '" << filename_pnkp
                  << "'. Reason was " << strerror(errno) << std::endl;
            return error.str();
        }

        for (auto const& p: net.places())
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
            std::stringstream error;
            error <<  "Failed to export the Petri net to '" << filename_pnk
                  << "'. Reason was " << strerror(errno) << std::endl;
            return error.str();
        }

        for (auto const& t: net.transitions())
        {
            file << t.caption << std::endl;
        }
    }

    return {};
}

} // namespace tpne