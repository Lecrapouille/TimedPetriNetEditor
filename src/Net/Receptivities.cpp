//=============================================================================
// TimedNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2023 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of TimedNetEditor.
//
// TimedNetEditor is free software: you can redistribute it and/or modify it
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

#include "Net/Receptivities.hpp"
#include "TimedPetriNetEditor/PetriNet.hpp"
#include <cassert>

namespace tpne {

//-----------------------------------------------------------------------------
Receptivity::StepExp::StepExp(Net& net, std::string const& name)
    : m_net(net)
{
    assert((name[0] == 'X') && "Incorrect place identifier");
    m_id = std::stoul(&name[1]); // FIXME better error management
}

//-----------------------------------------------------------------------------
bool Receptivity::StepExp::evaluate() const
{
    Place* p = m_net.findPlace(m_id);
    assert((p != nullptr) && "Unknown place id");
    return !!(p->tokens); // size_t to boolean conversion
}

//-----------------------------------------------------------------------------
Receptivity::CrossGraphStepExp::CrossGraphStepExp(std::string const& graph_name, std::string const& step_name)
    : m_graph_name(graph_name)
{
    assert((step_name[0] == 'X') && "Incorrect place identifier in cross-graph ref");
    m_step_id = std::stoul(&step_name[1]);
}

//-----------------------------------------------------------------------------
bool Receptivity::CrossGraphStepExp::evaluate() const
{
    Net* net = NetRegistry::instance().findNet(m_graph_name);
    if (net == nullptr)
    {
        // Graph not found, return false
        return false;
    }
    Place* p = net->findPlace(m_step_id);
    if (p == nullptr)
    {
        // Step not found in graph, return false
        return false;
    }
    return !!(p->tokens);
}

//-----------------------------------------------------------------------------
bool Receptivity::VariableExp::evaluate() const
{
    try {
        return Sensors::instance().get(m_name);
    }
    catch (...) {
        assert(false && "Unkown variable");
        return false;
    }
}

//-----------------------------------------------------------------------------
Receptivity::ConstExp::ConstExp(std::string const& operand)
{
    if (operand == "true") {
        m_operand = true;
    } else if (operand == "false") {
        m_operand = false;
    } else {
        assert(false && "Unkown const operand");
    }
}

//-----------------------------------------------------------------------------
static bool findToken(std::vector<std::string> const& tokens,
                      std::string const& token)
{
    for (auto const& it: tokens)
    {
        if (token == it)
            return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
bool Receptivity::Parser::isBinaryOperator(std::string const& token)
{
    static const std::vector<std::string> operators = { ".", "+" };
    return findToken(operators, token);
}

//-----------------------------------------------------------------------------
bool Receptivity::Parser::isUnitaryOperator(std::string const& token)
{
    static const std::vector<std::string> operators = { "!" };
    return findToken(operators, token);
}

//-----------------------------------------------------------------------------
bool Receptivity::Parser::isConst(std::string const& token)
{
    static const std::vector<std::string> operators = { "true", "false" };
    return findToken(operators, token);
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
// Check for cross-graph reference syntax: "GraphName:Xn"
bool Receptivity::Parser::isCrossGraphState(std::string const& token)
{
    size_t colon_pos = token.find(':');
    if (colon_pos == std::string::npos || colon_pos == 0)
        return false;

    // Check graph name part (before colon)
    std::string graph_name = token.substr(0, colon_pos);
    if (!isalpha(graph_name[0]))
        return false;
    for (size_t i = 1u; i < graph_name.length(); i++)
    {
        if (!isalnum(graph_name[i]) && graph_name[i] != '_')
            return false;
    }

    // Check step part (after colon) - must be Xn format
    std::string step_name = token.substr(colon_pos + 1);
    return isState(step_name);
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
        else if (isCrossGraphState(it))
        {
            // Cross-graph reference: "GraphName:Xn" -> "GraphName.X[n]"
            size_t colon_pos = it.find(':');
            std::string graph_name = it.substr(0, colon_pos);
            std::string step_part = it.substr(colon_pos + 2); // Skip ":X"
            std::string state(graph_name + ".X[" + step_part + "]");
            exprs.push(state);
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
std::shared_ptr<Receptivity::BooleanExp> Receptivity::Parser::compile(
    std::string const& code, Net& net, std::string& error)
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
        else if (isCrossGraphState(it))
        {
            // Parse cross-graph reference: "GraphName:Xn"
            size_t colon_pos = it.find(':');
            std::string graph_name = it.substr(0, colon_pos);
            std::string step_name = it.substr(colon_pos + 1);
            exprs.push(std::make_shared<CrossGraphStepExp>(graph_name, step_name));
        }
        else if (isState(it))
        {
            exprs.push(std::make_shared<StepExp>(net, it));
        }
        else if (isVariable(it))
        {
            Sensors::instance().set(it, false); // Default value
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
std::string Receptivity::compile(std::string const& code, Net& net)
{
    m_error.clear();
    m_ast = Receptivity::Parser::compile(code, net, m_error);
    m_valid = m_error.empty();
    return m_error;
}

//-----------------------------------------------------------------------------
bool Receptivity::evaluate()
{
    // Invalid syntaxt
    if (!m_valid)
        return false;

    // No expression means always TRUE receptivity
    if (m_ast == nullptr)
        return true;

    return m_ast->evaluate();
}

} // namespace tpne