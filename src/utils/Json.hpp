//=====================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 Quentin Quadrat <lecrapouille@gmail.com>
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
//=====================================================================

#ifndef JSON_HPP
#  define JSON_HPP

#  include <fstream>

// *****************************************************************************
//! \brief Helper class for splitting a JSON file into sub-strings that can be
//! parsed. Indeed, we do not use third part JSON library for reading saved file
//! and load Petri nets but we read it directly since the format is very basic.
// *****************************************************************************
class Spliter
{
public:

    //! \brief Open the file to be split and memorize delimiter chars.
    //! \param[in] filepath Open the file to be split
    //! \param[in] list of delimiter chars for string separation.
    Spliter(std::string const& filepath, std::string const& del)
        : is(filepath), delimiters(del)
    {}

    //! \brief Check if the stream state is fine.
    operator bool() const
    {
        return !!is;
    }

    //! \brief Return the first interesting json string element.
    //! If no element can be split return a dummy string;
    std::string const& split()
    {
        while (true)
        {
            if (pos == std::string::npos)
            {
                if (!std::getline(is, line))
                {
                    word.clear();
                    return word;
                }
                prev = 0u;
            }

            while ((pos = line.find_first_of(delimiters, prev)) != std::string::npos)
            {
                if (pos > prev)
                {
                    word = line.substr(prev, pos - prev);
                    prev = pos + 1u;
                    return word;
                }
                prev = pos + 1u;
            }

            if (prev < line.length())
            {
                word = line.substr(prev, std::string::npos);
                return word;
            }
        }

        word.clear();
        return word;
    }

    //! \brief Return the last split string.
    std::string const& str() const
    {
        return word;
    }

private:

    std::ifstream is;
    std::string delimiters;
    std::string line;
    std::string word;
    std::size_t prev = 0u;
    std::size_t pos = std::string::npos;
};

#endif
