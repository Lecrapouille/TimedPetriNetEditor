// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "PetriNet/Color.hpp"

#include <cstdio>

namespace tpne {

//------------------------------------------------------------------------------
std::string toHex(std::uint32_t const rgb)
{
    char text[8];
    std::snprintf(text, sizeof(text), "#%06x", unsigned(rgb & 0xFFFFFFu));
    return text;
}

//------------------------------------------------------------------------------
std::optional<std::uint32_t> fromHex(std::string const& text)
{
    if ((text.size() != 7u) || (text[0] != '#'))
        return std::nullopt;
    std::uint32_t value = 0u;
    for (size_t i = 1u; i < text.size(); ++i)
    {
        char const c = text[i];
        std::uint32_t digit;
        if ((c >= '0') && (c <= '9'))
            digit = std::uint32_t(c - '0');
        else if ((c >= 'a') && (c <= 'f'))
            digit = std::uint32_t(c - 'a' + 10);
        else if ((c >= 'A') && (c <= 'F'))
            digit = std::uint32_t(c - 'A' + 10);
        else
            return std::nullopt;
        value = (value << 4u) | digit;
    }
    return value;
}

} // namespace tpne
