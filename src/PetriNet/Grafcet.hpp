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

#  include <cctype>
#  include <memory>
#  include <string>
#  include <map>
#  include <set>
#  include <vector>
#  include <sstream>

namespace tpne {

class Net;
class Place;

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

//! \brief Parse a forcing string like "Production:{init}" or "G2:{2,8}". The
//! colon is optional: "Production{init}" is the same order.
inline Forcing parseForcing(std::string const& str)
{
    Forcing f;
    size_t braceStart = str.find('{');
    size_t braceEnd = str.find('}', (braceStart == std::string::npos) ? 0u : braceStart);
    size_t nameEnd = (braceStart == std::string::npos) ? str.size() : braceStart;
    size_t const colonPos = str.rfind(':', nameEnd);
    if ((colonPos != std::string::npos) && (colonPos < nameEnd))
        nameEnd = colonPos;
    f.targetNet = str.substr(0, nameEnd);
    while (!f.targetNet.empty() && std::isspace(static_cast<unsigned char>(f.targetNet.back())))
        f.targetNet.pop_back();
    while (!f.targetNet.empty() && std::isspace(static_cast<unsigned char>(f.targetNet.front())))
        f.targetNet.erase(0, 1);

    // Find content between { }
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

//! \brief Rename the references to a step in a receptivity: "X3" (graph
//! empty: the step of the graph owning the receptivity) or "Production:X3"
//! (graph "Production") becomes "X5" or "Production:X5". Other text is kept.
inline std::string renameStepReference(std::string const& code, std::string const& graph,
                                       size_t const from, size_t const to)
{
    auto const ident = [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) || (c == '_') ||
               (static_cast<unsigned char>(c) >= 0x80u);
    };
    std::string const target = (graph.empty() ? "" : graph + ":") + "X" + std::to_string(from);
    std::string const replacement = (graph.empty() ? "" : graph + ":") + "X" + std::to_string(to);
    std::string out;
    size_t i = 0u;
    while (i < code.size())
    {
        bool const match = (code.compare(i, target.size(), target) == 0) &&
            ((i == 0u) || (!ident(code[i - 1u]) && (graph.empty() ? code[i - 1u] != ':' : true))) &&
            ((i + target.size() == code.size()) || !ident(code[i + target.size()]));
        if (match)
        {
            out += replacement;
            i += target.size();
        }
        else
        {
            out += code[i++];
        }
    }
    return out;
}

//! \brief Rename a step in a forcing order of the graph: "G2:{3,8}" with
//! the step 3 now numbered 5 becomes "G2:{5,8}".
inline void renameForcedStep(Forcing& forcing, std::string const& graph, size_t const from,
                             size_t const to)
{
    if ((forcing.targetNet != graph) || (forcing.type != Forcing::Type::Steps))
        return;
    for (auto& step : forcing.steps)
    {
        if (step == from)
            step = to;
    }
}

//! \brief Forcing order as drawn in the action box of its step (IEC 60848):
//! "Production{init}", "G2{2,8}", "G2{*}", "G2{}".
inline std::string forcingLabel(Forcing const& f)
{
    std::string text = forcingToStr(f);
    size_t const colon = text.find(":{");
    if (colon != std::string::npos)
        text.erase(colon, 1u);
    return text;
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
        P,   // Pulse on activation (P1): single pulse when the step activates
        P0   // Pulse on deactivation: single pulse when the step deactivates
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
    //! \brief Assignment condition (IEC 60848 conditional action): the action
    //! only runs while this receptivity-like expression is true. Empty means
    //! always true. Only for continuous actions (N, D, L).
    std::string condition;

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
        case Action::Qualifier::P: return "P1";
        case Action::Qualifier::P0: return "P0";
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
    if ((s == "P") || (s == "P1")) return Action::Qualifier::P;
    if (s == "P0") return Action::Qualifier::P0;
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
//! \brief What the operator acts on: a GRAFCET input as shown on the control
//! panel of the editor (a push button, a selector, a sensor ...). The engine
//! only sees integer values: the kind says how the panel draws the input and
//! how a click changes its value. Saved with the document.
// *****************************************************************************
struct GrafcetInput
{
    //! \brief How the input is drawn and acts.
    enum class Kind
    {
        Switch,    //!< Toggle switch: a click changes 0 <-> 1, and it stays.
        Button,    //!< Push button: 1 while pressed, during one cycle at least.
        Pulse,     //!< 1 during one cycle of the GRAFCET for each click.
        Emergency, //!< Emergency stop: pushed (1) until it is pulled back.
        Selector,  //!< One position of a selector: exclusive within its group.
        Value,     //!< Integer value of a sensor (temperature, counter ...).
        Clock      //!< Square signal: 1 during half a period, then 0.
    };

    std::string name;
    Kind kind = Kind::Switch;
    //! \brief What the input is, for the operator (tooltip of the panel).
    std::string description;
    //! \brief Value when the simulation starts.
    int initial = 0;
    //! \brief Selector: name of the selector; its inputs are exclusive.
    std::string group;
    //! \brief Button, switch: color of the cap ("green", "red", "black",
    //! "yellow", "blue", "white"). Empty: a color chosen from the kind.
    std::string color;
    //! \brief Value: range and unit of the sensor.
    int min = 0;
    int max = 100;
    std::string unit;
    //! \brief Value: a simple model of the machine. While the output
    //! `driven_by` is on, the value goes up by `rise` per second, else it
    //! goes down by `fall` per second (between min and max). Empty: the
    //! operator sets the value.
    std::string driven_by;
    float rise = 0.0f;
    float fall = 0.0f;
    //! \brief Clock: period in seconds.
    float period = 1.0f;
};

//! \brief Convert GrafcetInput::Kind to string
inline const char* inputKindToStr(GrafcetInput::Kind k)
{
    switch (k) {
        case GrafcetInput::Kind::Switch: return "switch";
        case GrafcetInput::Kind::Button: return "button";
        case GrafcetInput::Kind::Pulse: return "pulse";
        case GrafcetInput::Kind::Emergency: return "emergency";
        case GrafcetInput::Kind::Selector: return "selector";
        case GrafcetInput::Kind::Value: return "value";
        case GrafcetInput::Kind::Clock: return "clock";
    }
    return "switch";
}

//! \brief Convert string to GrafcetInput::Kind
inline GrafcetInput::Kind strToInputKind(std::string const& s)
{
    if (s == "button") return GrafcetInput::Kind::Button;
    if (s == "pulse") return GrafcetInput::Kind::Pulse;
    if (s == "emergency") return GrafcetInput::Kind::Emergency;
    if (s == "selector") return GrafcetInput::Kind::Selector;
    if (s == "value") return GrafcetInput::Kind::Value;
    if (s == "clock") return GrafcetInput::Kind::Clock;
    return GrafcetInput::Kind::Switch;
}

// *****************************************************************************
//! \brief What the GRAFCET acts on: the actuator named by actions, as shown
//! on the control panel (a lamp, a motor that turns, a heater that glows ...).
// *****************************************************************************
struct GrafcetOutput
{
    //! \brief How the output is drawn.
    enum class Kind
    {
        Light,  //!< Pilot light of the color of its actions.
        Motor,  //!< Motor: its fan turns while on.
        Heater, //!< Heating resistor: glows while on.
        Valve,  //!< Valve or cylinder: open (out) while on.
        Horn    //!< Horn or buzzer: sound waves while on.
    };

    std::string name;
    Kind kind = Kind::Light;
    //! \brief What the actuator is, for the operator.
    std::string description;
};

//! \brief Convert GrafcetOutput::Kind to string
inline const char* outputKindToStr(GrafcetOutput::Kind k)
{
    switch (k) {
        case GrafcetOutput::Kind::Light: return "light";
        case GrafcetOutput::Kind::Motor: return "motor";
        case GrafcetOutput::Kind::Heater: return "heater";
        case GrafcetOutput::Kind::Valve: return "valve";
        case GrafcetOutput::Kind::Horn: return "horn";
    }
    return "light";
}

//! \brief Convert string to GrafcetOutput::Kind
inline GrafcetOutput::Kind strToOutputKind(std::string const& s)
{
    if (s == "motor") return GrafcetOutput::Kind::Motor;
    if (s == "heater") return GrafcetOutput::Kind::Heater;
    if (s == "valve") return GrafcetOutput::Kind::Valve;
    if (s == "horn") return GrafcetOutput::Kind::Horn;
    return GrafcetOutput::Kind::Light;
}

// *****************************************************************************
//! \brief What a file says about all its nets: a text explaining them, and
//! for GRAFCET the inputs and outputs of the machine (for the control panel).
// *****************************************************************************
struct DocumentInfo
{
    //! \brief What the document is about (shown by the editor).
    std::string description;
    std::vector<GrafcetInput> inputs;
    std::vector<GrafcetOutput> outputs;
};

//! \brief Input of the given name, or null.
inline GrafcetInput* findInput(DocumentInfo& info, std::string const& name)
{
    for (auto& input : info.inputs)
        if (input.name == name)
            return &input;
    return nullptr;
}

//! \brief Output of the given name, or null.
inline GrafcetOutput* findOutput(DocumentInfo& info, std::string const& name)
{
    for (auto& output : info.outputs)
        if (output.name == name)
            return &output;
    return nullptr;
}

// *****************************************************************************
//! \brief Receptivity of a GRAFCET transition: the boolean condition that lets
//! the transition fire when all its upstream steps are active.
//!
//! The syntax follows IEC 60848 written with ASCII characters:
//!   - operators: `a.b` (and, also `a*b`), `a+b` (or), `/a` or `!a` (not),
//!     parentheses; precedence: not, and, or.
//!   - constants: `1`, `0`, `true`, `false`.
//!   - steps: `X3` (step 3 of the same graph), `Safety:X3` (another graph).
//!   - edges: `↑a` or `^a` (rising), `↓a` (falling), also `rising(a)`,
//!     `falling(a)`. An edge is true during one instant only.
//!   - time: `3s/a` (true 3 s after a became true), `3s/a/7s` (and stays
//!     true 7 s after a became false), `500ms/X2`. Units: ms, s, min, h.
//!   - comparison of an integer input: `[count >= 3]` (=, !=, <, <=, >, >=).
//!   - inputs: names like `Dcy`, `sensor_2`, or with arguments like
//!     `holding(red_cube)` or `done(Grasp(red_cube))`, so that a host
//!     application can give them a meaning.
//! An empty receptivity is always true.
// *****************************************************************************
class Receptivity
{
public:

    // *************************************************************************
    //! \brief Gives the parser the values that a receptivity reads. The
    //! returned pointers must stay valid while the receptivity is used.
    // *************************************************************************
    class Resolver
    {
    public:

        virtual ~Resolver() = default;

        //----------------------------------------------------------------------
        //! \brief Value of the input of the given name. Unknown inputs are
        //! created (value 0) by the resolver. Never null.
        //----------------------------------------------------------------------
        virtual int const* input(std::string const& name) = 0;

        //----------------------------------------------------------------------
        //! \brief Step `id` of the graph `graph` (empty name: the graph owning
        //! the receptivity). Return null if unknown.
        //----------------------------------------------------------------------
        virtual Place const* step(std::string const& graph, size_t id) = 0;
    };

    // *************************************************************************
    //! \brief State of a time operator t1/a/t2 of the receptivity, to show
    //! how long is left before it changes.
    // *************************************************************************
    struct Timer
    {
        //! \brief Time since the operand changed, up to duration.
        float elapsed = 0.0f;
        //! \brief t1 while waiting to become true, t2 to become false.
        float duration = 0.0f;
        //! \brief The timer counts (its operand changed, its output not yet).
        bool running = false;
        //! \brief Value of the operator.
        bool output = false;
    };

    // *************************************************************************
    //! \brief Node of the abstract syntax tree.
    // *************************************************************************
    class Node
    {
    public:

        virtual ~Node() = default;
        //! \brief Value at the current instant.
        virtual bool evaluate() const = 0;
        //! \brief Let time pass (timers). Called once per cycle.
        virtual void tick(float /*dt*/) {}
        //! \brief End of an instant: edges are consumed.
        virtual void endOfInstant() {}
        //! \brief End of a cycle (graphs stable): time operators see the
        //! changes made by the evolution.
        virtual void endOfCycle() {}
        //! \brief State of the time operators (3s/X2) of the expression.
        virtual void timers(std::vector<Timer>& /*out*/) const {}
        //! \brief Write the expression in C ("C") or Structured Text ("ST").
        //! Set `supported` to false for constructs with a memory (edges,
        //! timers) that have no direct translation.
        virtual std::string translate(std::string const& lang, bool& supported) const = 0;
    };

    //--------------------------------------------------------------------------
    //! \brief Parse the expression. Return false on syntax error (see error()).
    //--------------------------------------------------------------------------
    bool compile(std::string const& code, Resolver& resolver);

    //--------------------------------------------------------------------------
    //! \brief Check the syntax only. Return the error message, or empty.
    //--------------------------------------------------------------------------
    static std::string check(std::string const& code);

    //--------------------------------------------------------------------------
    //! \brief Translate the expression to "C" or "ST" (Structured Text).
    //! \param[out] supported false if the expression has edges or timers,
    //! which need a memory not generated by the translation.
    //! \throw std::runtime_error on syntax error.
    //--------------------------------------------------------------------------
    static std::string translate(std::string const& code, std::string const& lang,
                                 bool& supported);
    static std::string translate(std::string const& code, std::string const& lang)
    {
        bool supported;
        return translate(code, lang, supported);
    }

    //--------------------------------------------------------------------------
    //! \brief Names of the inputs read by the expression.
    //--------------------------------------------------------------------------
    static std::set<std::string> inputs(std::string const& code);

    inline bool isValid() const { return m_valid; }
    inline std::string const& error() const { return m_error; }
    bool evaluate() const;
    void tick(float dt) { if (m_ast) m_ast->tick(dt); }
    //! \brief State of the time operators, in the order of the expression.
    std::vector<Timer> timers() const
    {
        std::vector<Timer> out;
        if (m_ast)
            m_ast->timers(out);
        return out;
    }
    void endOfInstant() { if (m_ast) m_ast->endOfInstant(); }
    void endOfCycle() { if (m_ast) m_ast->endOfCycle(); }

private:

    //! \brief Valid or invalid syntax (invalid may also mean not parsed).
    bool m_valid = false;
    //! \brief Abstract syntax tree (null for an always true receptivity).
    std::shared_ptr<Node> m_ast;
    //! \brief Parsing error
    std::string m_error;
};

} // namespace tpne

#endif
