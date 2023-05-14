//=============================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2022 Quentin Quadrat <lecrapouille@gmail.com>
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

#include "Receptivities.hpp"

//-----------------------------------------------------------------------------
void Receptivity::code(std::string const& code_)
{
    m_code = code_;
    m_expression = parse();
}

//-----------------------------------------------------------------------------
bool Receptivity::isOperator(std::string const& token)
{
    static const std::vector<std::string> operators = {
        ".", "+"
    };

    for (auto const& it: operators)
    {
        if (token == it)
            return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
std::string Receptivity::convert(std::string const& token, std::string const& lang)
{
    // <Forth symbole> -> map(<destination language>, <destination symbole>)
    static std::map<std::string, std::map<std::string, std::string>> translations = {
        { ".", { { "C", "&" }, { "ST", "AND" } } },
        { "+", { { "C", "|" }, { "ST", "OR" } } },
    };

    // Not token found => return directly
    auto const& it = translations.find(token);
    if (it == translations.end())
        return token;

    // Not language found => return directly
    auto const& itt = (*it).second.find(lang);
    if (itt == (*it).second.end())
        return token;

    return (*itt).second;
}

//-----------------------------------------------------------------------------
std::vector<std::string> Receptivity::tokenizer(std::string const& s, std::string const& delimiter)
{
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
    {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(s.substr (pos_start));
    return res;
}

//-----------------------------------------------------------------------------
std::string Receptivity::translate(std::string const& lang)
{
    std::stack<std::string> exprs;
    std::vector<std::string> tokens = tokenizer(m_code, " ");

    for (auto const& it: tokens)
    {
        if (isOperator(it))
        {
            if (exprs.size() < 2u)
            {
                std::cerr << "Bad expression" <<std::endl;
                return {};
            }

            std::string operand1 = exprs.top();
            exprs.pop();

            std::string operand2 = exprs.top();
            exprs.pop();

            exprs.push("(" + operand2 + " " + convert(it, lang) + " " + operand1 + ")");
        }
        else
        {
            exprs.push(it);
        }
    }
    return exprs.top();
}

//-----------------------------------------------------------------------------
std::shared_ptr<Receptivity::BooleanExp> Receptivity::parse()
{
    std::stack<std::shared_ptr<BooleanExp>> exprs;
    std::vector<std::string> tokens = tokenizer(m_code, " ");

    for (auto const& it: tokens)
    {
        if (isOperator(it))
        {
            if (exprs.size() < 2u)
            {
                std::cerr << "Bad expression" <<std::endl;
                return nullptr;
            }

            std::shared_ptr<BooleanExp> operand1 = exprs.top();
            exprs.pop();

            std::shared_ptr<BooleanExp> operand2 = exprs.top();
            exprs.pop();

            if (it == ".")
                exprs.push(std::make_shared<AndExp>(operand1, operand2));
            else if (it == "+")
                exprs.push(std::make_shared<OrExp>(operand1, operand2));
        }
        else
        {
            exprs.push(std::make_shared<VariableExp>(it));
        }
    }
    return exprs.top();
}