// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "IO/Exports/Exports.hpp"
#include "PetriNet/PetriNet.hpp"
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
    // pn-editor knows neither inhibitor nor reset arcs.
    if (features(net) & (Feature::Inhibitors | Feature::Resets))
    {
        return "Failed to export the Petri net to '" + filename +
               "'. Reason: 'PN-Editor has no inhibitor nor reset arcs'\n";
    }

    // .pns file: contains the logical contents of the petri net
    {
        std::string filename_pns(filename.substr(0, filename.find_last_of('.')) + ".pns");
        std::ofstream file(filename_pns, std::ios::out | std::ios::binary);
        if (!file)
        {
            std::stringstream error;
            error << "Failed to export the Petri net to '" << filename_pns
                  << "'. Reason: " << strerror(errno) << std::endl;
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
            // An arc of weight w is the place written w times.
            size_t count = 0u;
            for (auto const& a: net.arcsOut(t))
                count += a.weight;
            write_int32(file, count);
            for (auto const& a: net.arcsOut(t))
            {
                for (size_t w = 0u; w < a.weight; ++w)
                    write_int32(file, a.to.id);
            }
            count = 0u;
            for (auto const& a: net.arcsIn(t))
                count += (a.kind == ArcKind::Normal) ? a.weight : 0u;
            write_int32(file, count);
            for (auto const& a: net.arcsIn(t))
            {
                for (size_t w = 0u; (a.kind == ArcKind::Normal) && (w < a.weight); ++w)
                    write_int32(file, a.from.id);
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
                  << "'. Reason: " << strerror(errno) << std::endl;
            return error.str();
        }

        for (auto const& t: net.transitions())
        {
            write_float32(file, net.position(t).x);
            write_float32(file, net.position(t).y);
        }

        for (auto const& p: net.places())
        {
            write_float32(file, net.position(p).x);
            write_float32(file, net.position(p).y);
        }
    }

    // .pnkp: list of names for all the places
    {
        std::string filename_pnkp(filename.substr(0, filename.find_last_of('.')) + ".pnkp");
        std::ofstream file(filename_pnkp);
        if (!file)
        {
            std::stringstream error;
            error << "Failed to export the Petri net to '" << filename_pnkp
                  << "'. Reason: " << strerror(errno) << std::endl;
            return error.str();
        }

        for (auto const& p: net.places())
        {
            file << p.caption << std::endl;
        }
    }

    // .pnk: list of names for all the transitions
    {
        std::string filename_pnk(filename.substr(0, filename.find_last_of('.')) + ".pnk");
        std::ofstream file(filename_pnk);
        if (!file)
        {
            std::stringstream error;
            error <<  "Failed to export the Petri net to '" << filename_pnk
                  << "'. Reason: " << strerror(errno) << std::endl;
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
