//=====================================================================
// MyLogger: A basic logger.
// Copyright 2018 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of MyLogger.
//
// MyLogger is free software: you can redistribute it and/or modify it
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
// along with MyLogger.  If not, see <http://www.gnu.org/licenses/>.
//=====================================================================

#include "Utils/Path.hpp"
#include "Utils/Utils.hpp"
#include <sstream>
#include <sys/stat.h>

//------------------------------------------------------------------------------
Path::Path(std::string const& path, char const delimiter)
  : m_delimiter(delimiter)
{
    add(path);
}

//------------------------------------------------------------------------------
void Path::add(std::string const& path)
{
    if (!path.empty())
    {
        split(path);
    }
}

//------------------------------------------------------------------------------
void Path::reset(std::string const& path)
{
    m_search_paths.clear();
    split(path);
}

//------------------------------------------------------------------------------
void Path::clear()
{
    m_search_paths.clear();
    m_string_path.clear();
    m_dirty = false;
}

//------------------------------------------------------------------------------
void Path::remove(std::string const& path)
{
    m_search_paths.remove(path);
    m_dirty = true;
}

//------------------------------------------------------------------------------
bool Path::exist(std::string const& path) const
{
    struct stat buffer;
    return stat(path.c_str(), &buffer) == 0;
}

//------------------------------------------------------------------------------
std::pair<std::string, bool> Path::find(std::string const& filename) const
{
    if (Path::exist(filename))
        return std::make_pair(filename, true);

    for (auto const& it: m_search_paths)
    {
        std::string file(it + filename);
        if (Path::exist(file))
            return std::make_pair(file, true);
    }

    // Not found
    return std::make_pair(std::string(), false);
}

//------------------------------------------------------------------------------
std::string Path::expand(std::string const& filename) const
{
    for (auto const& it: m_search_paths)
    {
        std::string file(it + filename);
        if (Path::exist(file))
            return file;
    }

    return filename;
}

//------------------------------------------------------------------------------
bool Path::open(std::string& filename, std::ifstream& ifs, std::ios_base::openmode mode) const
{
    ifs.open(filename.c_str(), mode);
    if (ifs)
        return true;

    for (auto const& it: m_search_paths)
    {
        std::string file(it + filename);
        ifs.open(file.c_str(), mode);
        if (ifs)
        {
            filename = file;
            return true;
        }
    }

    // Not found
    return false;
}

//------------------------------------------------------------------------------
bool Path::open(std::string& filename, std::ofstream& ofs, std::ios_base::openmode mode) const
{
    ofs.open(filename.c_str(), mode);
    if (ofs)
        return true;

    for (auto const& it: m_search_paths)
    {
        std::string file(it + filename);
        ofs.open(file.c_str(), mode);
        if (ofs)
        {
            filename = file;
            return true;
        }
    }

    // Not found
    return false;
}

//------------------------------------------------------------------------------
bool Path::open(std::string& filename, std::fstream& fs, std::ios_base::openmode mode) const
{
    fs.open(filename.c_str(), mode);
    if (fs)
        return true;

    for (auto const& it: m_search_paths)
    {
        std::string file(it + filename);
        fs.open(filename.c_str(), mode);
        if (fs)
        {
            filename = file;
            return true;
        }
    }

    // Not found
    return false;
}

//------------------------------------------------------------------------------
std::string const& Path::toString()
{
    update();
    return m_string_path;
}

//------------------------------------------------------------------------------
void Path::update()
{
    if (m_dirty)
    {
        m_string_path.clear();
        m_string_path += ".";
        m_string_path += m_delimiter;

        for (auto const& it: m_search_paths)
        {
            m_string_path += it;
            m_string_path.pop_back(); // Remove the '/' char
            m_string_path += m_delimiter;
        }
        m_dirty = false;
    }
}

//------------------------------------------------------------------------------
void Path::split(std::string const& path)
{
    std::stringstream ss(path);
    std::string directory;

    while (std::getline(ss, directory, m_delimiter))
    {
        if (directory.empty())
            continue ;

        if ((*directory.rbegin() == '\\') || (*directory.rbegin() == '/'))
            m_search_paths.push_back(directory);
        else
            m_search_paths.push_back(directory + "/");
    }
    m_dirty = true;
}
