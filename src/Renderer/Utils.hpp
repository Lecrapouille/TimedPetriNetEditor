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

#ifndef UTILS_HPP
#  define UTILS_HPP

#  include <math.h>
#  include <sstream>
#  include <vector>
#  include <unistd.h> // tmpPetriFile()
#  include <iomanip>  // tmpPetriFile()
#  include <pwd.h>    // tmpPetriFile()
#  include <memory>
#  include <sys/stat.h>
#  include <sys/types.h>

#  ifdef __APPLE__
#    include <CoreFoundation/CFBundle.h>
#  endif

//------------------------------------------------------------------------------
// Return the data folder.
//------------------------------------------------------------------------------
inline std::string data_path(std::string const& file)
{
    struct stat exists; // folder exists ?
    std::string path;

#  ifdef __APPLE__

    // Return the resources folder inside MacOS bundle application
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

#endif

#ifdef DATADIR
    path = std::string(DATADIR) + "/" + file;
    if (stat(path.c_str(), &exists) == 0)
    {
        return path;
    }
#endif

    path = "data/" + file;
    if (stat(path.c_str(), &exists) == 0)
    {
        return path;
    }

    return file;
}

//------------------------------------------------------------------------------
inline float random(int lower, int upper)
{
    auto const t = static_cast<unsigned int>(time(NULL));
    srand(t);
    return float(rand() % (upper - lower + 1)) + float(lower);
}

//------------------------------------------------------------------------------
inline const char* current_time()
{
    static char buffer[32];

    time_t current_time = ::time(nullptr);
    strftime(buffer, sizeof (buffer), "[%H:%M:%S] ", localtime(&current_time));
    return buffer;
}

//------------------------------------------------------------------------------
template<typename T> T convert_to(const std::string &str)
{
    std::istringstream ss(str); T num; ss >> num; return num;
}

template<typename T> T convert_to(const char* str)
{
    std::istringstream ss(str); T num; ss >> num; return num;
}

//------------------------------------------------------------------------------
inline size_t token2vector(std::string const& s, std::vector<std::string>& words)
{
    std::stringstream ss(s);
    std::string tmp;

    words.clear();
    while (getline(ss, tmp, ','))
    {
        words.push_back(tmp);
    }

    return words.size();
}

//------------------------------------------------------------------------------
//! \brief Return current date as string.
inline std::string currentDate()
{
    std::ostringstream oss;
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    oss << std::put_time(&tm, "%Y-%m-%d__%H-%M-%S");
    return oss.str();
}

//------------------------------------------------------------------------------
//! \brief Create the name of a temporary Petri file.
inline std::string tmpPetriFile()
{
    std::ostringstream oss;
    oss << getpwuid(getuid())->pw_dir; // Home folder
    oss << ".TimedPetriNetEditor/petri__" << currentDate() << ".json";
    return oss.str();
}

#endif
