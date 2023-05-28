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

#include "Receptivities.hpp"
#include "PetriNet.hpp"
#include <cassert>

//-----------------------------------------------------------------------------
Receptivity::StepExp::StepExp(PetriNet& net, std::string const& token)
    : m_net(net)
{
    assert(token[0] == 'X' && "Incorrect state identifier");
    m_id = std::stoi(&token[1]);
}

//-----------------------------------------------------------------------------
bool Receptivity::StepExp::evaluate(Sensors const& sensors) const
{
    Place* p = m_net.findPlace(m_id);
    if (p == nullptr)
        return false;
    return !!(p->tokens);
}

//-----------------------------------------------------------------------------
bool Receptivity::Parser::isBinaryOperator(std::string const& token)
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
bool Receptivity::Parser::isUnitaryOperator(std::string const& token)
{
    static const std::vector<std::string> operators = {
        "!"
    };

    for (auto const& it: operators)
    {
        if (token == it)
            return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
bool Receptivity::Parser::isConst(std::string const& token)
{
    static const std::vector<std::string> operators = {
        "true", "false"
    };

    for (auto const& it: operators)
    {
        if (token == it)
            return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
bool Receptivity::Parser::isState(std::string const& token)
{
    if ((token.size() <= 1u) || (token[0] != 'X'))
        return false;

    for (size_t i = 1u; i < token.length(); i++)
    {
        if (!isdigit(token[i]))
            return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
bool Receptivity::Parser::isVariable(std::string const& token)
{
    if (!isalpha(token[0]))
        return false;

    for (size_t i = 1u; i < token.length(); i++)
    {
        if (!isalnum(token[i]))
            return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
std::string Receptivity::Parser::convert(std::string const& token, std::string const& lang)
{
    // <Forth symbole> -> map(<destination language>, <destination symbole>)
    static std::map<std::string, std::map<std::string, std::string>> translations = {
        { ".", { { "C", "&" }, { "ST", "AND" } } },
        { "+", { { "C", "|" }, { "ST", "OR" } } },
        { "!", { { "C", "!" }, { "ST", "NOT" } } },
        { "true", { { "C", "true" }, { "ST", "TRUE" } } },
        { "false", { { "C", "false" }, { "ST", "FALSE" } } },
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
std::vector<std::string> Receptivity::Parser::tokenizer(std::string const& s, std::string const& delimiter)
{
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
    {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        if (!token.empty())
            res.push_back(token);
    }

    token = s.substr(pos_start);
    if (!token.empty())
        res.push_back(token);
    return res;
}

//-----------------------------------------------------------------------------
std::string Receptivity::Parser::translate(std::string const& code, std::string const& lang)
{
    if (code.empty())
        return convert("true", lang);

    std::stack<std::string> exprs;
    std::vector<std::string> tokens = tokenizer(code, " ");

    for (auto const& it: tokens)
    {
        if (isUnitaryOperator(it))
        {
            if (exprs.size() < 1u)
            {
                throw "Bad expression";
            }

            std::string operand1 = exprs.top();
            exprs.pop();

            exprs.push("(" + convert(it, lang) + " " + operand1 + ")");

        }
        else if (isBinaryOperator(it))
        {
            if (exprs.size() < 2u)
            {
                throw "Bad expression";
            }

            std::string operand1 = exprs.top();
            exprs.pop();

            std::string operand2 = exprs.top();
            exprs.pop();

            exprs.push("(" + operand2 + " " + convert(it, lang) + " " + operand1 + ")");
        }
        else if (isConst(it))
        {
            exprs.push(convert(it, lang));
        }
        else if (isState(it))
        {
            std::string state("X[");
            state += &it[1];
            state += "]";
            exprs.push(state);
        }
        else if (isVariable(it))
        {
            exprs.push(it);
        }
        else
        {
            throw "Bad expression";
        }
    }
    return exprs.top();
}

//-----------------------------------------------------------------------------
std::shared_ptr<Receptivity::BooleanExp> Receptivity::Parser::parse(PetriNet& net, std::string const& code, std::string& error)
{
    error.clear();

    // Implicity dummy expression means always true receptivities
    if (code.empty())
        return nullptr;

    std::stack<std::shared_ptr<BooleanExp>> exprs;
    std::vector<std::string> tokens = tokenizer(code, " ");

    for (std::string const& it: tokens)
    {
        if (isUnitaryOperator(it))
        {
            if (exprs.size() < 1u)
            {
                error = "Parse error: stack underflow with operator " + it;
                return std::make_shared<ConstExp>("false");
            }

            std::shared_ptr<BooleanExp> operand1 = exprs.top();
            exprs.pop();

            if (it == "!")
                exprs.push(std::make_shared<NotExp>(operand1));
            else
                assert(false && "Missing operator");

        }
        else if (isBinaryOperator(it))
        {
            if (exprs.size() < 2u)
            {
                error = "Parse error: stack underflow with operator " + it;
                return std::make_shared<ConstExp>("false");
            }

            std::shared_ptr<BooleanExp> operand1 = exprs.top();
            exprs.pop();

            std::shared_ptr<BooleanExp> operand2 = exprs.top();
            exprs.pop();

            if (it == ".")
                exprs.push(std::make_shared<AndExp>(operand1, operand2));
            else if (it == "+")
                exprs.push(std::make_shared<OrExp>(operand1, operand2));
            else
                assert(false && "Missing operator");
        }
        else if (isConst(it))
        {
            exprs.push(std::make_shared<ConstExp>(it));
        }
        else if (isState(it))
        {
            exprs.push(std::make_shared<StepExp>(net, it));
        }
        else if (isVariable(it))
        {
            net.m_sensors.assign(it, false); // Default value
            exprs.push(std::make_shared<VariableExp>(it));
        }
        else
        {
            error = "Parse error: invalid token " + it;
            return std::make_shared<ConstExp>("false");
        }
    }

    return exprs.top();
}

//-----------------------------------------------------------------------------
bool Receptivity::Parser::evaluate(Receptivity const recept, Sensors const& sensors)
{
    // No expression means always TRUE receptivity
    if (recept.expression == nullptr)
        return recept.valid;

    return recept.expression->evaluate(sensors);
}