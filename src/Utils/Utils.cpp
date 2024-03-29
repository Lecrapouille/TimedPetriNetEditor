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

#include "Utils/Utils.hpp"
#include "project_info.hpp" // generated by MyMakefile
#include <sys/stat.h>

#ifdef __APPLE__
#  include <CoreFoundation/CFBundle.h>
#endif

//------------------------------------------------------------------------------
#ifdef __APPLE__
std::string osx_get_resources_dir(std::string const& file)
{
    struct stat exists; // folder exists ?
    std::string path;

    CFURLRef resourceURL = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
    char resourcePath[PATH_MAX];
    if (CFURLGetFileSystemRepresentation(resourceURL, true,
                                         reinterpret_cast<UInt8 *>(resourcePath),
                                         PATH_MAX))
    {
        if (resourceURL != NULL)
        {
            CFRelease(resourceURL);
        }

        path = std::string(resourcePath) + "/" + file;
        if (stat(path.c_str(), &exists) == 0)
        {
            return path;
        }
    }

    path = "data/" + file;
    if (stat(path.c_str(), &exists) == 0)
    {
        return path;
    }

    return file;
}
#endif

namespace tpne {

//------------------------------------------------------------------------------
float random(int lower, int upper)
{
    auto const t = static_cast<unsigned int>(time(NULL));
    srand(t);
    return float(rand() % (upper - lower + 1)) + float(lower);
}

} // namespace tpne