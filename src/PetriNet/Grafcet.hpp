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

#ifndef GRAFCET_HPP
#  define GRAFCET_HPP

#  include <memory>
#  include <string>
#  include <stack>
#  include <map>
#  include <vector>
#  include <sstream>

namespace tpne {

class Net;

// ****************************************************************************
//! \brief Registry for cross-graph references in GRAFCET multi-net documents.
//! Maps net names to their Net pointers for resolving references like "Security:X5"
// ****************************************************************************
class NetRegistry
{
public:
    static NetRegistry& instance() { static NetRegistry registry; return registry; }

    void registerNet(std::string const& name, Net* net) { m_nets[name] = net; }
    void unregisterNet(std::string const& name) { m_nets.erase(name); }
    Net* findNet(std::string const& name)
    {
        auto it = m_nets.find(name);
        return (it != m_nets.end()) ? it->second : nullptr;
    }
    std::map<std::string, Net*> const& getRegistry() const { return m_nets; }
    std::map<std::string, Net*>& getRegistry() { return m_nets; }
    void clear() { m_nets.clear(); }

private:
    NetRegistry() = default;
    std::map<std::string, Net*> m_nets;
};

// ****************************************************************************
//! \brief Quick and dirty container of sensor boolean values.
//! \fixme should store analogical value.
// ****************************************************************************
class Sensors
{
public:

    static Sensors& instance() { static Sensors sensor; return sensor; }

    //! \brief Get the value of the given sensor. Throw if the sensor is unkown.
    bool get(std::string const& sensor) { return m_values.at(sensor); modified = true; }
    void set(std::string const& sensor, int const value) { m_values[sensor] = value; }
    std::map<const std::string, int>& database() { return m_values; }
    void clear() { m_values.clear(); modified = false; }

private:

    Sensors() = default;

private:

    std::map<const std::string, int> m_values;

public:
    bool modified = false;
};

// *****************************************************************************
//! \brief GRAFCET Forcing command for hierarchical control.
//! Allows a master GRAFCET to force the state of a slave GRAFCET.
//! Syntax: "TargetNet:{init}", "TargetNet:{2,8}", "TargetNet:{}", "TargetNet:{*}"
// *****************************************************************************
struct Forcing
{
    //! \brief Type of forcing command
    enum class Type
    {
        Init,   // Force to initial state: "G2:{init}"
        Freeze, // Freeze (no evolution): "G2:{*}"
        Empty,  // Deactivate all steps: "G2:{}"
        Steps   // Activate specific steps: "G2:{2,8}"
    };

    std::string targetNet;       // Name of the target GRAFCET
    Type type = Type::Init;      // Type of forcing
    std::vector<size_t> steps;   // For Type::Steps: list of steps to activate
};

//! \brief Convert Forcing::Type to string
inline const char* forcingTypeToStr(Forcing::Type t)
{
    switch (t) {
        case Forcing::Type::Init: return "init";
        case Forcing::Type::Freeze: return "*";
        case Forcing::Type::Empty: return "";
        case Forcing::Type::Steps: return "steps";
    }
    return "init";
}

//! \brief Convert string to Forcing::Type
inline Forcing::Type strToForcingType(std::string const& s)
{
    if (s == "*") return Forcing::Type::Freeze;
    if (s == "" || s == "empty") return Forcing::Type::Empty;
    if (s == "steps") return Forcing::Type::Steps;
    return Forcing::Type::Init;
}

//! \brief Parse a forcing string like "Production:{init}" or "G2:{2,8}"
inline Forcing parseForcing(std::string const& str)
{
    Forcing f;
    size_t colonPos = str.find(':');
    if (colonPos == std::string::npos)
    {
        f.targetNet = str;
        f.type = Forcing::Type::Init;
        return f;
    }

    f.targetNet = str.substr(0, colonPos);

    // Find content between { }
    size_t braceStart = str.find('{', colonPos);
    size_t braceEnd = str.find('}', colonPos);
    if (braceStart == std::string::npos || braceEnd == std::string::npos)
    {
        f.type = Forcing::Type::Init;
        return f;
    }

    std::string content = str.substr(braceStart + 1, braceEnd - braceStart - 1);

    // Trim whitespace
    while (!content.empty() && std::isspace(content.front())) content.erase(0, 1);
    while (!content.empty() && std::isspace(content.back())) content.pop_back();

    if (content == "init")
    {
        f.type = Forcing::Type::Init;
    }
    else if (content == "*")
    {
        f.type = Forcing::Type::Freeze;
    }
    else if (content.empty())
    {
        f.type = Forcing::Type::Empty;
    }
    else
    {
        // Parse step numbers: "2,8" or "2, 8"
        f.type = Forcing::Type::Steps;
        std::stringstream ss(content);
        std::string token;
        while (std::getline(ss, token, ','))
        {
            while (!token.empty() && std::isspace(token.front())) token.erase(0, 1);
            while (!token.empty() && std::isspace(token.back())) token.pop_back();
            if (!token.empty())
            {
                try {
                    f.steps.push_back(std::stoul(token));
                } catch (...) {}
            }
        }
    }
    return f;
}

//! \brief Convert Forcing to string representation
inline std::string forcingToStr(Forcing const& f)
{
    std::string result = f.targetNet + ":{";
    switch (f.type) {
        case Forcing::Type::Init:
            result += "init";
            break;
        case Forcing::Type::Freeze:
            result += "*";
            break;
        case Forcing::Type::Empty:
            break;
        case Forcing::Type::Steps:
            for (size_t i = 0; i < f.steps.size(); ++i)
            {
                if (i > 0) result += ",";
                result += std::to_string(f.steps[i]);
            }
            break;
    }
    result += "}";
    return result;
}

// *****************************************************************************
//! \brief GRAFCET Action associated with a Step (Place).
//! Actions are operations performed when a step is active.
//! Qualifiers define the behavior: N (normal), S (set), R (reset), etc.
// *****************************************************************************
struct Action
{
    //! \brief IEC 60848 Action qualifiers
    enum class Qualifier
    {
        N,   // Normal: active while step is active
        S,   // Set: latched ON at step activation
        R,   // Reset: latched OFF at step activation
        D,   // Delayed: active after delay while step active
        L,   // Limited: active for limited time while step active
        SD,  // Stored & Delayed: set after delay
        DS,  // Delayed & Stored: delayed then latched
        SL,  // Stored & Limited: set for limited time
        P    // Pulse: single pulse at step activation
    };

    //! \brief LED colors for industrial visualization
    enum class LedColor
    {
        Green,   // Default for most actions
        Red,     // Alarms, errors
        Orange,  // Warnings
        Yellow,  // Caution
        Blue,    // Information
        White    // Neutral
    };

    Qualifier qualifier = Qualifier::N;
    LedColor color = LedColor::Green;  // LED color for visualization
    std::string name;        // Action name (e.g., "KM1", "Verin_A+")
    std::string script;      // Code/description of the action
    float duration = 0.0f;   // For D, L, SD, DS, SL qualifiers (seconds)

    //! \brief Forcings to apply when this action is active (GRAFCET hierarchical control)
    std::vector<Forcing> forcings;
};

//! \brief Convert Action::Qualifier to string
inline const char* qualifierToStr(Action::Qualifier q)
{
    switch (q) {
        case Action::Qualifier::N: return "N";
        case Action::Qualifier::S: return "S";
        case Action::Qualifier::R: return "R";
        case Action::Qualifier::D: return "D";
        case Action::Qualifier::L: return "L";
        case Action::Qualifier::SD: return "SD";
        case Action::Qualifier::DS: return "DS";
        case Action::Qualifier::SL: return "SL";
        case Action::Qualifier::P: return "P";
    }
    return "N";
}

//! \brief Convert string to Action::Qualifier
inline Action::Qualifier strToQualifier(std::string const& s)
{
    if (s == "S") return Action::Qualifier::S;
    if (s == "R") return Action::Qualifier::R;
    if (s == "D") return Action::Qualifier::D;
    if (s == "L") return Action::Qualifier::L;
    if (s == "SD") return Action::Qualifier::SD;
    if (s == "DS") return Action::Qualifier::DS;
    if (s == "SL") return Action::Qualifier::SL;
    if (s == "P") return Action::Qualifier::P;
    return Action::Qualifier::N;
}

//! \brief Convert Action::LedColor to string
inline const char* ledColorToStr(Action::LedColor c)
{
    switch (c) {
        case Action::LedColor::Green: return "green";
        case Action::LedColor::Red: return "red";
        case Action::LedColor::Orange: return "orange";
        case Action::LedColor::Yellow: return "yellow";
        case Action::LedColor::Blue: return "blue";
        case Action::LedColor::White: return "white";
    }
    return "green";
}

//! \brief Convert string to Action::LedColor
inline Action::LedColor strToLedColor(std::string const& s)
{
    if (s == "red") return Action::LedColor::Red;
    if (s == "orange") return Action::LedColor::Orange;
    if (s == "yellow") return Action::LedColor::Yellow;
    if (s == "blue") return Action::LedColor::Blue;
    if (s == "white") return Action::LedColor::White;
    return Action::LedColor::Green;
}

// *****************************************************************************
//! \brief Receptivity is the boolean expression stored in GRAFCET transitions
//! that make a transition fireable or not. This class allows to parse simple
//! boolean expressions and create an abstract syntax tree (AST). For simplicity
//! reasons the syntax use reversed polish notation (RPN). Therefore expression
//! such as "(a or b) and X0" will be written as "a b or X0 and".
// *****************************************************************************
class Receptivity
{
public:

    std::string compile(std::string const& code, Net& net);
    inline bool isValid() const { return m_valid; }
    inline std::string const& error() const { return m_error; }
    bool evaluate() const;

    // *************************************************************************
    //! \brief Base class of boolean expression used as AST node.
    // *************************************************************************
    class BooleanExp
    {
    public:

        virtual ~BooleanExp() = default;
        virtual bool evaluate() const = 0;
    };

    // *************************************************************************
    //! \brief Expression for GRAFCET place state. In GRAFCET places are named
    //! steps and are named X0, X1 ... (while inside the editor places ID are
    //! named P0, P1, ...) we are following the GRAFCET standard.
    //! \note the GRAFCET net shall no remove the place !
    // *************************************************************************
    class StepExp : public BooleanExp
    {
    public:

        //! \param[in] name Step (aka place) name, i.e. "X0".
        StepExp(Net& net, std::string const& name);
        bool evaluate() const override;

    private:

        Net& m_net;
        //! \brief Place ID. The GRAFCET net shall no remove the place to let this
        //! ID valid.
        size_t m_id;
    };

    // *************************************************************************
    //! \brief Expression for cross-graph step references in GRAFCET.
    //! Syntax: "GraphName:Xn" where GraphName is the net name and n is step id.
    //! Example: "Security:X5" refers to step X5 in the "Security" graph.
    // *************************************************************************
    class CrossGraphStepExp : public BooleanExp
    {
    public:

        //! \param[in] graph_name Name of the target graph/net.
        //! \param[in] step_name Step name (e.g. "X5").
        CrossGraphStepExp(std::string const& graph_name, std::string const& step_name);
        bool evaluate() const override;

    private:

        std::string m_graph_name;
        size_t m_step_id;
    };

    // *************************************************************************
    //! \brief Expression for sensor variable name (i.e. "foo")
    // *************************************************************************
    class VariableExp : public BooleanExp
    {
    public:

        explicit VariableExp(std::string const& name)
            : m_name(name)
        {}

        bool evaluate() const override;

    private:

        std::string m_name;
    };

    // *************************************************************************
    //! \brief Expression for constant (i.e. "true" and "false")
    // *************************************************************************
    class ConstExp : public BooleanExp
    {
    public:

        explicit ConstExp(std::string const& operand);
        bool evaluate() const override
        {
            return m_operand;
        }

    private:

        bool m_operand;
    };

    // *************************************************************************
    //! \brief Expression for negation (i.e. "foo !")
    // *************************************************************************
    class NotExp : public BooleanExp
    {
    public:

        explicit NotExp(std::shared_ptr<BooleanExp> operand)
            : m_operand(operand)
        {}

        bool evaluate() const override
        {
            return !m_operand->evaluate();
        }

    private:

        std::shared_ptr<BooleanExp> m_operand;
    };

    // *************************************************************************
    //! \brief Expression for logical AND (i.e. "a b .")
    // *************************************************************************
    class AndExp : public BooleanExp
    {
    public:

        explicit AndExp(std::shared_ptr<BooleanExp> op1, std::shared_ptr<BooleanExp> op2)
            : m_operand1(op1), m_operand2(op2)
        {}

        bool evaluate() const override
        {
            return m_operand1->evaluate() && m_operand2->evaluate();
        }

    private:

        std::shared_ptr<BooleanExp> m_operand1;
        std::shared_ptr<BooleanExp> m_operand2;
    };

    // *************************************************************************
    //! \brief Expression for logical OR (i.e. "a b +")
    // *************************************************************************
    class OrExp : public BooleanExp
    {
    public:

        OrExp(std::shared_ptr<BooleanExp> op1, std::shared_ptr<BooleanExp> op2)
            : m_operand1(op1), m_operand2(op2)
        {}

        bool evaluate() const override
        {
            return m_operand1->evaluate() || m_operand2->evaluate();
        }

    private:

        std::shared_ptr<BooleanExp> m_operand1;
        std::shared_ptr<BooleanExp> m_operand2;
    };

    // *****************************************************************************
    //! \brief Quick Reverse Polish Notation parser of boolean expression set inside
    //! GRAFCET transitions (i.e. "a b . X0 + !" for "not((a and b) or X0)").
    //! \fixme Currently management of bad syntax is not made (assert and throw).
    // *****************************************************************************
    class Parser
    {
    public:

        //--------------------------------------------------------------------------
        //! \brief Translate the given RPN code of the receptivity to a desired
        //! language. Currently supported languages are:
        //!  - C language "C"
        //!  - Structure Text language "ST".
        //--------------------------------------------------------------------------
        static std::string translate(std::string const& code, std::string const& lang);

        //--------------------------------------------------------------------------
        //! \brief Parse a postfix notation into an AST.
        //--------------------------------------------------------------------------
        static std::shared_ptr<BooleanExp> compile(std::string const& code, Net& net, std::string& error);

    private:

        static bool isBinaryOperator(std::string const& token);
        static bool isUnitaryOperator(std::string const& token);
        static bool isConst(std::string const& token);
        static bool isState(std::string const& token);
        static bool isCrossGraphState(std::string const& token);
        static bool isVariable(std::string const& token);
        static std::vector<std::string> tokenizer(std::string const& s, std::string const& delimiter);
        static std::string convert(std::string const& token, std::string const& lang);
    };

private:

    //! \brief Valid or invalid syntax (invalid may also mean not parsed).
    bool m_valid = false;
    //! \brief Abstract syntax tree of the boolean expression (GRAFCET transitivity).
    std::shared_ptr<BooleanExp> m_ast;
    //! \brief Parsing error
    std::string m_error;
};

} // namespace tpne

#endif
