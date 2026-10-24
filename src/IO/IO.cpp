// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "IO/IO.hpp"
#include "PetriNet/PetriNet.hpp"

#include <algorithm>

namespace tpne {

//------------------------------------------------------------------------------
std::string fileExtension(std::string const& path)
{
    std::string::size_type pos = path.find_last_of(".");
    if (pos == std::string::npos)
        return {};

    std::string ext = path.substr(pos, std::string::npos);
    if (!ext.empty() && ext.back() == '~')
        ext.pop_back();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

//------------------------------------------------------------------------------
std::string saveToFile(Net const& net, std::string const& filepath)
{
    Exporter const* exporter = findExporter(filepath);
    if (exporter == nullptr)
    {
        return "Cannot export '" + filepath + "'. Reason: 'unknown file extension'\n";
    }
    return exporter->exportFct(net, filepath);
}

//------------------------------------------------------------------------------
std::string loadFromFile(Net& net, std::string const& filepath, bool& springify)
{
    Importer const* importer = findImporter(filepath);
    if (importer == nullptr)
    {
        return "Cannot import '" + filepath + "'. Reason: 'unknown file extension'\n";
    }

    // Load the file
    net.clear();
    std::string error = importer->importFct(net, filepath);
    if (!error.empty())
    {
        net.reset(net.type());
    }
    else
    {
        springify = importer->springify;
    }

    // Get a name to the net
    if (net.name == "")
    {
        size_t lastindex = filepath.find_last_of(".");
        std::string _name = filepath.substr(0, lastindex);
        lastindex = _name.find_last_of("/");
        net.name = _name.substr(lastindex + 1u);
    }

    net.modified = false;

    return error;
}

//------------------------------------------------------------------------------
std::string loadFromFile(Net& net, std::string const& filepath)
{
    bool springify;
    return loadFromFile(net, filepath, springify);
}

} // namespace tpne
