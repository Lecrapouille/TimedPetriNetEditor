//=============================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2026 Quentin Quadrat <lecrapouille@gmail.com>
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

#include "PetriNet/Grafcet.hpp"
#include "PetriNet/PetriNet.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>

namespace tpne {

using NodePtr = std::shared_ptr<Receptivity::Node>;

// *****************************************************************************
// Nodes of the abstract syntax tree.
// *****************************************************************************

//! \brief Pick the word of the given language: C or ST.
static char const* word(std::string const& lang, char const* c, char const* st)
{
    return (lang == "ST") ? st : c;
}

//! \brief Constant true or false.
class ConstNode : public Receptivity::Node
{
public:
    explicit ConstNode(bool value) : m_value(value) {}
    bool evaluate() const override { return m_value; }
    std::string translate(std::string const& lang, bool&) const override
    {
        return m_value ? word(lang, "true", "TRUE") : word(lang, "false", "FALSE");
    }
private:
    bool m_value;
};

//! \brief Input given by the host (sensor, button, robot state ...).
class InputNode : public Receptivity::Node
{
public:
    InputNode(std::string const& name, int const* value) : m_name(name), m_value(value) {}
    bool evaluate() const override { return *m_value != 0; }
    std::string translate(std::string const&, bool&) const override { return m_name; }
private:
    std::string m_name;
    int const* m_value;
};

//! \brief Activity of a step: Xn or Graph:Xn.
class StepNode : public Receptivity::Node
{
public:
    StepNode(std::string const& graph, size_t id, Place const* place)
        : m_graph(graph), m_id(id), m_place(place)
    {}
    bool evaluate() const override { return (m_place != nullptr) && (m_place->tokens > 0u); }
    std::string translate(std::string const&, bool&) const override
    {
        return (m_graph.empty() ? "" : m_graph + ".") + "X[" + std::to_string(m_id) + "]";
    }
private:
    std::string m_graph;
    size_t m_id;
    Place const* m_place;
};

//! \brief Logical not.
class NotNode : public Receptivity::Node
{
public:
    explicit NotNode(NodePtr operand) : m_operand(std::move(operand)) {}
    bool evaluate() const override { return !m_operand->evaluate(); }
    void tick(float dt) override { m_operand->tick(dt); }
    void timers(std::vector<Receptivity::Timer>& out) const override { m_operand->timers(out); }
    void endOfInstant() override { m_operand->endOfInstant(); }
    void endOfCycle() override { m_operand->endOfCycle(); }
    std::string translate(std::string const& lang, bool& supported) const override
    {
        return std::string(word(lang, "!", "NOT ")) + "(" + m_operand->translate(lang, supported) + ")";
    }
private:
    NodePtr m_operand;
};

//! \brief Logical and / or.
class BinaryNode : public Receptivity::Node
{
public:
    BinaryNode(bool is_and, NodePtr left, NodePtr right)
        : m_and(is_and), m_left(std::move(left)), m_right(std::move(right))
    {}
    bool evaluate() const override
    {
        bool const left = m_left->evaluate();
        bool const right = m_right->evaluate();
        return m_and ? (left && right) : (left || right);
    }
    void tick(float dt) override { m_left->tick(dt); m_right->tick(dt); }
    void timers(std::vector<Receptivity::Timer>& out) const override
    {
        m_left->timers(out);
        m_right->timers(out);
    }
    void endOfInstant() override { m_left->endOfInstant(); m_right->endOfInstant(); }
    void endOfCycle() override { m_left->endOfCycle(); m_right->endOfCycle(); }
    std::string translate(std::string const& lang, bool& supported) const override
    {
        char const* op = m_and ? word(lang, " && ", " AND ") : word(lang, " || ", " OR ");
        return "(" + m_left->translate(lang, supported) + op + m_right->translate(lang, supported) + ")";
    }
private:
    bool m_and;
    NodePtr m_left;
    NodePtr m_right;
};

//! \brief Rising (↑a) or falling (↓a) edge: true during the instant where the
//! operand changes. The previous value is taken at the end of each instant.
class EdgeNode : public Receptivity::Node
{
public:
    EdgeNode(bool rising, NodePtr operand) : m_rising(rising), m_operand(std::move(operand))
    {
        m_previous = m_operand->evaluate();
    }
    bool evaluate() const override
    {
        bool const now = m_operand->evaluate();
        return m_rising ? (now && !m_previous) : (!now && m_previous);
    }
    void tick(float dt) override { m_operand->tick(dt); }
    void timers(std::vector<Receptivity::Timer>& out) const override { m_operand->timers(out); }
    void endOfInstant() override
    {
        m_operand->endOfInstant();
        m_previous = m_operand->evaluate();
    }
    void endOfCycle() override { m_operand->endOfCycle(); }
    std::string translate(std::string const& lang, bool& supported) const override
    {
        supported = false;
        return std::string(m_rising ? "RISING(" : "FALLING(") + m_operand->translate(lang, supported) + ")";
    }
private:
    bool m_rising;
    NodePtr m_operand;
    bool m_previous = false;
};

//! \brief Time operator t1/a/t2 (IEC 60848): becomes true t1 seconds after a
//! became true, and becomes false t2 seconds after a became false.
class TimerNode : public Receptivity::Node
{
public:
    TimerNode(float t1, NodePtr operand, float t2)
        : m_t1(t1), m_t2(t2), m_operand(std::move(operand))
    {
        m_input = m_operand->evaluate();
    }
    bool evaluate() const override { return m_output; }
    void tick(float dt) override
    {
        m_operand->tick(dt);
        m_elapsed += dt;
        // A change made by the host (inputs) since the last cycle starts now.
        sample();
        // Small tolerance against the accumulation of float time steps.
        float const epsilon = 1e-4f;
        if (m_input && !m_output && (m_elapsed + epsilon >= m_t1))
            m_output = true;
        else if (!m_input && m_output && (m_elapsed + epsilon >= m_t2))
            m_output = false;
    }
    void endOfInstant() override { m_operand->endOfInstant(); }
    void timers(std::vector<Receptivity::Timer>& out) const override
    {
        m_operand->timers(out);
        Receptivity::Timer timer;
        timer.output = m_output;
        if (m_input && !m_output)
            timer = { std::min(m_elapsed, m_t1), m_t1, true, false };
        else if (!m_input && m_output && (m_t2 > 0.0f))
            timer = { std::min(m_elapsed, m_t2), m_t2, true, true };
        else
            timer.duration = m_input ? m_t1 : m_t2;
        out.push_back(timer);
    }
    void endOfCycle() override
    {
        // A change made by the evolution (a step activated) starts at the end
        // of the cycle.
        m_operand->endOfCycle();
        sample();
    }
    std::string translate(std::string const& lang, bool& supported) const override
    {
        supported = false;
        return "TIMER(" + std::to_string(m_t1) + ", " + m_operand->translate(lang, supported)
            + ", " + std::to_string(m_t2) + ")";
    }
private:
    void sample()
    {
        bool const input = m_operand->evaluate();
        if (input != m_input)
        {
            m_input = input;
            m_elapsed = 0.0f;
        }
    }
private:
    float m_t1;
    float m_t2;
    NodePtr m_operand;
    bool m_input = false;
    bool m_output = false;
    float m_elapsed = 0.0f;
};

//! \brief Comparison of an integer input with a number: [count >= 3].
class CompareNode : public Receptivity::Node
{
public:
    CompareNode(std::string const& name, int const* value, std::string const& op, double number)
        : m_name(name), m_value(value), m_op(op), m_number(number)
    {}
    bool evaluate() const override
    {
        double const v = double(*m_value);
        if (m_op == "=") return v == m_number;
        if (m_op == "!=") return v != m_number;
        if (m_op == "<") return v < m_number;
        if (m_op == "<=") return v <= m_number;
        if (m_op == ">") return v > m_number;
        return v >= m_number;
    }
    std::string translate(std::string const& lang, bool&) const override
    {
        std::string op = m_op;
        if (op == "=") op = word(lang, "==", "=");
        else if (op == "!=") op = word(lang, "!=", "<>");
        std::ostringstream number;
        number << m_number;
        return "(" + m_name + " " + op + " " + number.str() + ")";
    }
private:
    std::string m_name;
    int const* m_value;
    std::string m_op;
    double m_number;
};

// *****************************************************************************
//! \brief Resolver used to check or translate an expression: inputs are fake,
//! steps are unknown places (always inactive).
// *****************************************************************************
class DummyResolver : public Receptivity::Resolver
{
public:
    int const* input(std::string const& name) override
    {
        names.insert(name);
        return &m_zero;
    }
    Place const* step(std::string const&, size_t) override { return nullptr; }
    std::set<std::string> names;
private:
    int m_zero = 0;
};

// *****************************************************************************
//! \brief Recursive descent parser of receptivities. See the grammar in the
//! documentation of the class Receptivity.
// *****************************************************************************
class ReceptivityParser
{
public:

    ReceptivityParser(std::string const& code, Receptivity::Resolver& resolver)
        : m_code(code), m_resolver(resolver)
    {}

    //! \brief Parse the whole code. Throw std::runtime_error on syntax error.
    NodePtr parse()
    {
        skipSpaces();
        if (atEnd())
            return nullptr;
        NodePtr node = parseOr();
        skipSpaces();
        if (!atEnd())
            fail("unexpected '" + m_code.substr(m_pos, 1) + "'");
        return node;
    }

private:

    [[noreturn]] void fail(std::string const& message) const
    {
        throw std::runtime_error("column " + std::to_string(m_pos + 1u) + ": " + message);
    }

    bool atEnd() const { return m_pos >= m_code.size(); }

    void skipSpaces()
    {
        while (!atEnd() && std::isspace(static_cast<unsigned char>(m_code[m_pos])))
            ++m_pos;
    }

    //! \brief Consume the given text if it is next (after spaces).
    bool accept(std::string const& text)
    {
        skipSpaces();
        if (m_code.compare(m_pos, text.size(), text) == 0)
        {
            m_pos += text.size();
            return true;
        }
        return false;
    }

    //! \brief Consume a keyword (a whole word) if it is next.
    bool acceptWord(std::string const& keyword)
    {
        skipSpaces();
        size_t const end = m_pos + keyword.size();
        if ((m_code.compare(m_pos, keyword.size(), keyword) == 0) &&
            ((end >= m_code.size()) || !isNameChar(m_code[end])))
        {
            m_pos = end;
            return true;
        }
        return false;
    }

    static bool isNameStart(char const c)
    {
        return std::isalpha(static_cast<unsigned char>(c)) || (c == '_');
    }

    static bool isNameChar(char const c)
    {
        return std::isalnum(static_cast<unsigned char>(c)) || (c == '_');
    }

    // or := and ( ('+' | '|' | 'or') and )*
    NodePtr parseOr()
    {
        NodePtr left = parseAnd();
        while (accept("+") || accept("||") || accept("|") || acceptWord("or"))
            left = std::make_shared<BinaryNode>(false, left, parseAnd());
        return left;
    }

    // and := unary ( ('.' | '*' | '&' | '·' | 'and') unary )*
    NodePtr parseAnd()
    {
        NodePtr left = parseUnary();
        while (accept(".") || accept("*") || accept("&&") || accept("&") ||
               accept("\xC2\xB7") || acceptWord("and"))
            left = std::make_shared<BinaryNode>(true, left, parseUnary());
        return left;
    }

    // unary := ('/' | '!' | '¬' | 'not') unary | ('↑' | '^') unary | '↓' unary
    //        | timer | primary
    NodePtr parseUnary()
    {
        if (accept("/") || accept("!") || accept("\xC2\xAC") || acceptWord("not"))
            return std::make_shared<NotNode>(parseUnary());
        if (accept("\xE2\x86\x91") || accept("^"))
            return std::make_shared<EdgeNode>(true, parseUnary());
        if (accept("\xE2\x86\x93"))
            return std::make_shared<EdgeNode>(false, parseUnary());
        skipSpaces();
        if (!atEnd() && std::isdigit(static_cast<unsigned char>(m_code[m_pos])))
            return parseNumberOrTimer();
        return parsePrimary();
    }

    //! \brief Duration unit after a number: ms, s, min, h. Return the factor
    //! to seconds, or 0 if there is no unit.
    float parseUnit()
    {
        if (acceptWord("ms")) return 0.001f;
        if (acceptWord("s")) return 1.0f;
        if (acceptWord("min")) return 60.0f;
        if (acceptWord("h")) return 3600.0f;
        return 0.0f;
    }

    //! \brief Number made of digits and at most one decimal point followed by
    //! a digit (a point not followed by a digit is the and operator).
    double parseNumber()
    {
        skipSpaces();
        size_t const start = m_pos;
        while (!atEnd() && std::isdigit(static_cast<unsigned char>(m_code[m_pos])))
            ++m_pos;
        if ((m_pos + 1u < m_code.size()) && (m_code[m_pos] == '.') &&
            std::isdigit(static_cast<unsigned char>(m_code[m_pos + 1u])))
        {
            ++m_pos;
            while (!atEnd() && std::isdigit(static_cast<unsigned char>(m_code[m_pos])))
                ++m_pos;
        }
        if (start == m_pos)
            fail("a number is expected");
        return std::stod(m_code.substr(start, m_pos - start));
    }

    // timer := DURATION '/' unary ( '/' DURATION )?   or the constants 0 and 1.
    NodePtr parseNumberOrTimer()
    {
        double const value = parseNumber();
        float const unit = parseUnit();
        if (unit == 0.0f)
        {
            if (value == 0.0) return std::make_shared<ConstNode>(false);
            if (value == 1.0) return std::make_shared<ConstNode>(true);
            fail("only 0 and 1 are boolean constants; a duration needs a unit (ms, s, min, h)");
        }
        if (!accept("/"))
            fail("'/' is expected after a duration (as in 3s/X2)");
        NodePtr operand = parseUnary();
        float t2 = 0.0f;
        size_t const save = m_pos;
        if (accept("/"))
        {
            skipSpaces();
            if (!atEnd() && std::isdigit(static_cast<unsigned char>(m_code[m_pos])))
            {
                double const v = parseNumber();
                float const unit2 = parseUnit();
                if (unit2 == 0.0f)
                    fail("a duration needs a unit (ms, s, min, h)");
                t2 = float(v) * unit2;
            }
            else
            {
                m_pos = save;
            }
        }
        return std::make_shared<TimerNode>(float(value) * unit, operand, t2);
    }

    //! \brief Name, with optional arguments in balanced parentheses.
    std::string parseName()
    {
        skipSpaces();
        size_t const start = m_pos;
        if (atEnd() || !isNameStart(m_code[m_pos]))
            fail(atEnd() ? "unexpected end, an operand is expected"
                         : "unexpected '" + m_code.substr(m_pos, 1) + "', an operand is expected");
        while (!atEnd() && isNameChar(m_code[m_pos]))
            ++m_pos;
        std::string name = m_code.substr(start, m_pos - start);
        if (!atEnd() && (m_code[m_pos] == '('))
        {
            // Arguments: kept as text, without spaces, for the host.
            int depth = 0;
            do
            {
                char const c = m_code[m_pos++];
                if (c == '(') ++depth;
                else if (c == ')') --depth;
                if (!std::isspace(static_cast<unsigned char>(c)))
                    name += c;
            } while (!atEnd() && (depth > 0));
            if (depth != 0)
                fail("missing ')'");
        }
        return name;
    }

    //! \brief Step number after X, if the name is Xn.
    static bool isStep(std::string const& name, size_t& id)
    {
        if ((name.size() < 2u) || (name[0] != 'X'))
            return false;
        for (size_t i = 1u; i < name.size(); ++i)
        {
            if (!std::isdigit(static_cast<unsigned char>(name[i])))
                return false;
        }
        id = std::stoul(name.substr(1));
        return true;
    }

    NodePtr makeStep(std::string const& graph, size_t const id)
    {
        Place const* place = m_resolver.step(graph, id);
        return std::make_shared<StepNode>(graph, id, place);
    }

    // primary := '(' or ')' | '[' NAME CMP NUMBER ']' | 'true' | 'false'
    //          | 'rising' '(' or ')' | 'falling' '(' or ')'
    //          | GRAPH ':' 'X' DIGITS | 'X' DIGITS | NAME
    NodePtr parsePrimary()
    {
        if (accept("("))
        {
            NodePtr node = parseOr();
            if (!accept(")"))
                fail("missing ')'");
            return node;
        }
        if (accept("["))
        {
            std::string const name = parseName();
            std::string op;
            for (char const* candidate : { "<=", ">=", "!=", "<>", "==", "=", "<", ">" })
            {
                if (accept(candidate)) { op = candidate; break; }
            }
            if (op.empty())
                fail("a comparison operator is expected (=, !=, <, <=, >, >=)");
            if (op == "<>") op = "!=";
            if (op == "==") op = "=";
            bool const negative = accept("-");
            double number = parseNumber();
            if (negative) number = -number;
            if (!accept("]"))
                fail("missing ']'");
            return std::make_shared<CompareNode>(name, m_resolver.input(name), op, number);
        }
        if (acceptWord("true")) return std::make_shared<ConstNode>(true);
        if (acceptWord("false")) return std::make_shared<ConstNode>(false);
        for (bool rising : { true, false })
        {
            size_t const save = m_pos;
            if (acceptWord(rising ? "rising" : "falling") && accept("("))
            {
                NodePtr operand = parseOr();
                if (!accept(")"))
                    fail("missing ')'");
                return std::make_shared<EdgeNode>(rising, operand);
            }
            m_pos = save;
        }

        std::string const name = parseName();
        size_t id;
        if ((m_pos < m_code.size()) && (m_code[m_pos] == ':'))
        {
            ++m_pos;
            std::string const step = parseName();
            if (!isStep(step, id))
                fail("a step like X3 is expected after '" + name + ":'");
            return makeStep(name, id);
        }
        if (isStep(name, id))
            return makeStep("", id);
        return std::make_shared<InputNode>(name, m_resolver.input(name));
    }

private:

    std::string const& m_code;
    Receptivity::Resolver& m_resolver;
    size_t m_pos = 0u;
};

//-----------------------------------------------------------------------------
bool Receptivity::compile(std::string const& code, Resolver& resolver)
{
    m_error.clear();
    m_ast = nullptr;
    try
    {
        m_ast = ReceptivityParser(code, resolver).parse();
        m_valid = true;
    }
    catch (std::exception const& e)
    {
        m_error = "Syntax error in '" + code + "' at " + e.what();
        m_valid = false;
    }
    return m_valid;
}

//-----------------------------------------------------------------------------
std::string Receptivity::check(std::string const& code)
{
    DummyResolver resolver;
    Receptivity receptivity;
    receptivity.compile(code, resolver);
    return receptivity.error();
}

//-----------------------------------------------------------------------------
std::string Receptivity::translate(std::string const& code, std::string const& lang,
                                   bool& supported)
{
    supported = true;
    DummyResolver resolver;
    NodePtr ast = ReceptivityParser(code, resolver).parse();
    if (ast == nullptr)
        return word(lang, "true", "TRUE");
    return ast->translate(lang, supported);
}

//-----------------------------------------------------------------------------
std::set<std::string> Receptivity::inputs(std::string const& code)
{
    DummyResolver resolver;
    Receptivity receptivity;
    receptivity.compile(code, resolver);
    return resolver.names;
}

//-----------------------------------------------------------------------------
bool Receptivity::evaluate() const
{
    // Invalid syntax
    if (!m_valid)
        return false;

    // No expression means always TRUE receptivity
    if (m_ast == nullptr)
        return true;

    return m_ast->evaluate();
}

} // namespace tpne
