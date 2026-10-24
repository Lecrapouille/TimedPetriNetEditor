// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// Generate the C++ code of GRAFCETs: one class running the graphs of a
// document together, with the rules of evolution of the editor (GrafcetEngine,
// IEC 60848): search of the stable situation, simultaneous firing, edges and
// time operators of the receptivities, delays of the transitions, actions
// (qualifiers, conditions), forcing orders between graphs. The code needs no
// library (no STL): a micro-controller runs it. With GRAFCET_DEBUG it
// publishes its steps and its firings for the editor (Debug > Connect): on
// ZeroMQ for a computer, on the serial line for an Arduino (bridge:
// tools/serial-bridge.py).

#include "IO/Exports/Exports.hpp"
#include "PetriNet/Grafcet.hpp"
#include "PetriNet/PetriNet.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

namespace tpne {

namespace {

//------------------------------------------------------------------------------
//! \brief A C++ identifier from the text ("Moteur Droit" -> Moteur_Droit).
std::string identifier(std::string const& text)
{
    std::string id;
    for (char const c : text)
    {
        if (std::isalnum(static_cast<unsigned char>(c)) || (c == '_'))
            id += c;
        else if (!id.empty() && (id.back() != '_'))
            id += '_';
    }
    while (!id.empty() && (id.back() == '_'))
        id.pop_back();
    if (id.empty() || std::isdigit(static_cast<unsigned char>(id[0])))
        id = "_" + id;
    return id;
}

//------------------------------------------------------------------------------
//! \brief The text in a JSON string, inside a C string literal.
std::string jsonLiteral(std::string const& text)
{
    std::string s;
    for (char const c : text)
    {
        if ((c == '"') || (c == '\\'))
            s += "\\\\";
        if (c == '"')
            s += "\\\"";
        else if (c == '\\')
            s += "\\\\";
        else if (c == '\n')
            s += "\\\\n";
        else
            s += c;
    }
    return "\\\"" + s + "\\\"";
}

//------------------------------------------------------------------------------
std::string floatLiteral(float const value)
{
    std::ostringstream os;
    os << value;
    std::string s = os.str();
    if (s.find_first_of(".eE") == std::string::npos)
        s += ".0";
    return s + "f";
}

// *****************************************************************************
//! \brief Expressions (receptivities, conditions of actions) translated for
//! the generated code: the steps are the arrays of their graphs, the edges
//! and the time operators get a memory each.
// *****************************************************************************
class Translator
{
public:

    explicit Translator(std::vector<Net const*> const& nets) : m_nets(nets) {}

    //! \brief C++ expression of the receptivity of the graph g.
    std::string translate(std::string const& code, size_t const g)
    {
        bool supported = true;
        std::string text = Receptivity::translate(code, "gen", supported);
        text = steps(text, g);
        return rewrite(text);
    }

    //! \brief Operands of the edges and of the time operators, by index.
    std::vector<std::string> const& edges() const { return m_edges; }
    struct Timer
    {
        std::string operand;
        std::string t1;
        std::string t2;
    };
    std::vector<Timer> const& timers() const { return m_timers; }
    std::string const& error() const { return m_error; }

private:

    //! \brief X[#graph#][n] -> Xg[n].
    std::string steps(std::string text, size_t const g)
    {
        size_t pos;
        while ((pos = text.find("X[#")) != std::string::npos)
        {
            size_t const end = text.find("#]", pos + 3u);
            std::string const graph = text.substr(pos + 3u, end - pos - 3u);
            size_t target = g;
            if (!graph.empty())
            {
                target = m_nets.size();
                for (size_t i = 0u; i < m_nets.size(); ++i)
                {
                    if (m_nets[i]->name == graph)
                        target = i;
                }
                if (target == m_nets.size())
                {
                    m_error = "unknown GRAFCET '" + graph + "'";
                    target = g;
                }
            }
            text.replace(pos, end + 2u - pos, "X" + std::to_string(target));
        }
        return text;
    }

    //! \brief Arguments of the call starting at the parenthesis at open,
    //! split at the commas of its level; end: after its closing parenthesis.
    static std::vector<std::string> arguments(std::string const& text, size_t const open, size_t& end)
    {
        std::vector<std::string> args(1u);
        int depth = 0;
        for (size_t i = open; i < text.size(); ++i)
        {
            char const c = text[i];
            if (c == '(')
            {
                if (depth++ == 0)
                    continue;
            }
            else if (c == ')')
            {
                if (--depth == 0)
                {
                    end = i + 1u;
                    return args;
                }
            }
            else if ((c == ',') && (depth == 1))
            {
                args.emplace_back();
                continue;
            }
            args.back() += c;
        }
        end = text.size();
        return args;
    }

    //! \brief RISE(e) -> rise(k), FALL(e) -> fall(k), TIMER(t1, e, t2) ->
    //! timer(k): the operands are numbered inner first.
    std::string rewrite(std::string const& text)
    {
        std::string out;
        size_t i = 0u;
        while (i < text.size())
        {
            bool const rise = (text.compare(i, 5u, "RISE(") == 0);
            bool const fall = (text.compare(i, 5u, "FALL(") == 0);
            bool const timer = (text.compare(i, 6u, "TIMER(") == 0);
            if (!rise && !fall && !timer)
            {
                out += text[i++];
                continue;
            }
            size_t end;
            std::vector<std::string> args = arguments(text, i + (timer ? 5u : 4u), end);
            if (timer && (args.size() == 3u))
            {
                std::string const operand = rewrite(args[1]);
                float const t1 = std::stof(args[0]);
                float const t2 = std::stof(args[2]);
                m_timers.push_back({ operand, floatLiteral(t1), floatLiteral(t2) });
                out += "timer(" + std::to_string(m_timers.size() - 1u) + ")";
            }
            else
            {
                std::string const operand = rewrite(args[0]);
                m_edges.push_back(operand);
                out += std::string(rise ? "rise(" : "fall(") + std::to_string(m_edges.size() - 1u) + ")";
            }
            i = end;
        }
        return out;
    }

private:

    std::vector<Net const*> const& m_nets;
    std::vector<std::string> m_edges;
    std::vector<Timer> m_timers;
    std::string m_error;
};

//------------------------------------------------------------------------------
char const* qualifierName(Action::Qualifier const q)
{
    return qualifierToStr(q);
}

} // namespace

//------------------------------------------------------------------------------
std::string exportGrafcetsToCpp(std::vector<Net> const& document, std::string const& filename)
{
    std::vector<Net const*> nets;
    for (auto const& net : document)
    {
        if (net.type() == TypeOfNet::GRAFCET)
            nets.push_back(&net);
    }
    if (nets.empty())
        return "No GRAFCET to export";

    // Translate the receptivities and the conditions of the actions.
    Translator translator(nets);
    std::vector<std::vector<std::string>> receptivities(nets.size());
    std::vector<std::vector<std::vector<std::string>>> conditions(nets.size());
    std::map<std::string, int> inputs;
    for (size_t g = 0u; g < nets.size(); ++g)
    {
        Net const& net = *nets[g];
        for (auto const& it : net.inputs)
            inputs.emplace(it.first, it.second);
        for (auto const& t : net.transitions())
        {
            std::string const error = Receptivity::check(t.caption);
            if (!error.empty())
                return net.name + ": " + t.key + " '" + t.caption + "': " + error;
            if (net.arcsIn(t).empty())
                return net.name + ": " + t.key + " has no upstream step";
            receptivities[g].push_back(translator.translate(t.caption, g));
            for (auto const& name : Receptivity::inputs(t.caption))
                inputs.emplace(name, 0);
        }
        conditions[g].resize(net.places().size());
        for (auto const& p : net.places())
        {
            for (auto const& a : p.actions)
            {
                std::string const error = Receptivity::check(a.condition);
                if (!error.empty())
                    return net.name + ": " + p.key + ": action " + a.name + ": " + error;
                conditions[g][p.id].push_back(translator.translate(a.condition, g));
                for (auto const& name : Receptivity::inputs(a.condition))
                    inputs.emplace(name, 0);
            }
        }
    }
    if (!translator.error().empty())
        return translator.error();
    for (auto const& input : inputs)
    {
        if (identifier(input.first) != input.first)
            return "Cannot export the input '" + input.first + "': not a C identifier";
    }

    // Outputs (actions without forcing orders) and stored actions, by name.
    std::map<std::string, std::string> outputs; // name -> member
    std::set<std::string> members;
    for (Net const* net : nets)
    {
        for (auto const& p : net->places())
        {
            for (auto const& a : p.actions)
            {
                // Forcing orders act on the graphs: they are not outputs.
                if (!a.forcings.empty() || (outputs.count(a.name) != 0u))
                    continue;
                std::string id = identifier(a.name);
                while (members.count(id) != 0u)
                    id += "_";
                members.insert(id);
                outputs[a.name] = id;
            }
        }
    }
    auto graphOf = [&](std::string const& name) -> size_t {
        for (size_t i = 0u; i < nets.size(); ++i)
        {
            if (nets[i]->name == name)
                return i;
        }
        return nets.size();
    };

    std::ofstream file(filename);
    if (!file)
    {
        std::stringstream error;
        error << "Failed to export the GRAFCET to '" << filename
              << "'. Reason: " << strerror(errno) << std::endl;
        return error.str();
    }

    std::string const base = filename.substr(filename.find_last_of("/\\") + 1u);
    std::string space = identifier(nets.size() == 1u ? nets[0]->name : base.substr(0u, base.find('.')));
    std::transform(space.begin(), space.end(), space.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    std::string guard = space;
    std::transform(guard.begin(), guard.end(), guard.begin(), [](unsigned char c) { return char(std::toupper(c)); });
    size_t const edges = translator.edges().size();
    size_t const timers = translator.timers().size();

    //==========================================================================
    // Header
    //==========================================================================
    file << "// Generated by TimedPetriNetEditor: the GRAFCET";
    for (size_t g = 0u; g < nets.size(); ++g)
        file << (g ? ", " : " ") << nets[g]->name << " (X" << g << ")";
    file << ".\n"
         << "// Same rules of evolution as the editor (IEC 60848). No library needed: it\n"
         << "// runs on a micro-controller. Each period, the program writes the inputs, calls\n"
         << "// cycle(dt) and reads the outputs:\n"
         << "//\n"
         << "//   " << space << "::Grafcet grafcet;\n"
         << "//   void loop() {\n";
    if (!inputs.empty())
        file << "//       grafcet.in." << inputs.begin()->first << " = digitalRead(2);\n";
    file << "//       grafcet.cycle(0.01f); // seconds since the last cycle\n";
    if (!outputs.empty())
        file << "//       digitalWrite(3, grafcet.out." << outputs.begin()->second << ");\n";
    file << "//       delay(10);\n"
         << "//   }\n"
         << "//\n"
         << "// Debug: build with -DGRAFCET_DEBUG and the editor shows the steps and the\n"
         << "// firings as they come (Debug > Connect, tcp://localhost:5556):\n"
         << "//   - on a computer the code publishes on ZeroMQ (link with -lzmq;\n"
         << "//     GRAFCET_DEBUG_ENDPOINT changes the endpoint);\n"
         << "//   - on an Arduino it writes them on Serial: tools/serial-bridge.py publishes\n"
         << "//     them on ZeroMQ;\n"
         << "//   - else define GRAFCET_DEBUG_SEND(text) to send each line yourself.\n"
         << "// Test on a computer: g++ -std=c++11 -x c++ -DGRAFCET_STANDALONE_TEST " << base << "\n\n"
         << "#ifndef GRAFCET_" << guard << "_HPP\n#define GRAFCET_" << guard << "_HPP\n\n"
         << "#ifdef GRAFCET_DEBUG\n"
         << "#  if !defined(GRAFCET_DEBUG_SEND) && defined(ARDUINO)\n"
         << "#    include <Arduino.h>\n"
         << "#    define GRAFCET_DEBUG_SEND(text) Serial.println(text)\n"
         << "#  elif !defined(GRAFCET_DEBUG_SEND)\n"
         << "#    include <zmq.h>\n"
         << "#    ifndef GRAFCET_DEBUG_ENDPOINT\n"
         << "#      define GRAFCET_DEBUG_ENDPOINT \"tcp://*:5556\"\n"
         << "#    endif\n"
         << "#    define GRAFCET_DEBUG_ZEROMQ\n"
         << "#  endif\n"
         << "#endif\n\n"
         << "// Firing of a transition: given to the debugger.\n"
         << "#ifdef GRAFCET_DEBUG\n#  define GRAFCET_FIRED(g, t) fired(g, t)\n#else\n#  define GRAFCET_FIRED(g, t)\n#endif\n\n"
         << "namespace " << space << " {\n\n";

    // Inputs and outputs.
    file << "//! \\brief Inputs read by the receptivities: written by the program.\n"
         << "struct Inputs\n{\n";
    for (auto const& it : inputs)
        file << "    int " << it.first << " = " << it.second << ";\n";
    if (inputs.empty())
        file << "    int none = 0;\n";
    file << "};\n\n"
         << "//! \\brief Outputs of the actions: read by the program.\n"
         << "struct Outputs\n{\n";
    for (auto const& it : outputs)
    {
        // Forcing orders are not outputs, but a name may be both.
        file << "    bool " << it.second << " = false;";
        if (it.second != it.first)
            file << " // " << it.first;
        file << "\n";
    }
    if (outputs.empty())
        file << "    bool none = false;\n";
    file << "};\n\n";

    //==========================================================================
    // Class
    //==========================================================================
    file << "class Grafcet\n{\npublic:\n\n"
         << "    Inputs in;\n"
         << "    Outputs out;\n"
         << "    //! \\brief Active steps of each graph.\n";
    for (size_t g = 0u; g < nets.size(); ++g)
        file << "    bool X" << g << "[" << std::max(size_t(1u), nets[g]->places().size()) << "] = {}; // " << nets[g]->name << "\n";
    file << "\n    Grafcet() { reset(); }\n\n";

    // reset()
    file << "    //! \\brief The initial situation (initial values of the inputs).\n"
         << "    void reset()\n    {\n"
         << "        m_time = 0.0f;\n";
    for (size_t g = 0u; g < nets.size(); ++g)
    {
        for (auto const& p : nets[g]->places())
            file << "        X" << g << "[" << p.id << "] = " << (p.tokens ? "true" : "false") << ";\n";
    }
    for (auto const& it : outputs)
        file << "        out." << it.second << " = false;\n";
    file << "        for (int i = 0; i < " << std::max(size_t(1u), nets.size()) << "; ++i) m_forced[i] = false;\n";
    if (edges > 0u)
        file << "        for (int k = 0; k < " << edges << "; ++k) m_edge[k] = edgeInput(k);\n";
    if (timers > 0u)
        file << "        for (int k = 0; k < " << timers << "; ++k) { m_timer[k].in = timerInput(k); m_timer[k].out = false; m_timer[k].elapsed = 0.0f; }\n";
    for (size_t g = 0u; g < nets.size(); ++g)
    {
        for (auto const& t : nets[g]->transitions())
        {
            if (t.delay > 0.0f)
                file << "        m_delay" << g << "_" << t.id << " = 0.0f;\n";
        }
        for (auto const& p : nets[g]->places())
        {
            file << "        m_was" << g << "[" << p.id << "] = false; m_step_time" << g << "[" << p.id << "] = 0.0f;\n";
            for (size_t a = 0u; a < p.actions.size(); ++a)
                file << "        m_action" << g << "_" << p.id << "_" << a << " = Action();\n";
        }
    }
    std::set<std::string> stored;
    for (Net const* net : nets)
    {
        for (auto const& p : net->places())
        {
            for (auto const& a : p.actions)
            {
                if (a.forcings.empty() &&
                    ((a.qualifier == Action::Qualifier::S) || (a.qualifier == Action::Qualifier::R) ||
                     (a.qualifier == Action::Qualifier::DS) || (a.qualifier == Action::Qualifier::SD)))
                    stored.insert(a.name);
            }
        }
    }
    for (auto const& name : stored)
        file << "        m_stored_" << outputs[name] << " = false;\n";
    file << "#ifdef GRAFCET_DEBUG\n        m_fired = 0; m_published = -1.0f;\n#endif\n"
         << "    }\n\n";

    // cycle()
    file << "    //! \\brief One cycle, dt seconds after the previous one: time passes,\n"
         << "    //! forcing orders, evolution until a stable situation, actions.\n"
         << "    //! \\return false when no stable situation is reached (a loop of\n"
         << "    //! transitions always true).\n"
         << "    bool cycle(float const dt)\n    {\n"
         << "        m_time += dt;\n";
    if (timers > 0u)
    {
        file << "        // Time operators t1/a/t2.\n"
             << "        for (int k = 0; k < " << timers << "; ++k)\n        {\n"
             << "            m_timer[k].elapsed += dt;\n"
             << "            sample(k);\n"
             << "            if (m_timer[k].in && !m_timer[k].out && (m_timer[k].elapsed + 1e-4f >= T1[k]))\n"
             << "                m_timer[k].out = true;\n"
             << "            else if (!m_timer[k].in && m_timer[k].out && (m_timer[k].elapsed + 1e-4f >= T2[k]))\n"
             << "                m_timer[k].out = false;\n"
             << "        }\n";
    }
    for (size_t g = 0u; g < nets.size(); ++g)
    {
        for (auto const& t : nets[g]->transitions())
        {
            if (t.delay <= 0.0f)
                continue;
            file << "        // Delay of " << nets[g]->name << " " << t.key << "\n"
                 << "        m_delay" << g << "_" << t.id << " = (";
            for (auto const& a : nets[g]->arcsIn(t))
                file << "X" << g << "[" << a.from.id << "] && ";
            file << "R" << g << "_" << t.id << "()) ? (m_delay" << g << "_" << t.id << " + dt) : 0.0f;\n";
        }
    }
    file << "        forcings();\n"
         << "        bool const stable = evolve();\n"
         << "        actions(dt);\n"
         << "#ifdef GRAFCET_DEBUG\n        publish();\n#endif\n"
         << "        return stable;\n    }\n\n"
         << "    //! \\brief Seconds since reset().\n"
         << "    float time() const { return m_time; }\n\n"
         << "private:\n\n";

    // Receptivities and conditions.
    file << "    // Receptivities.\n";
    for (size_t g = 0u; g < nets.size(); ++g)
    {
        for (auto const& t : nets[g]->transitions())
        {
            file << "    bool R" << g << "_" << t.id << "() { return " << receptivities[g][t.id] << "; } // "
                 << nets[g]->name << " " << t.key << ": " << t.caption << "\n";
        }
    }
    file << "    // Conditions of the actions.\n";
    for (size_t g = 0u; g < nets.size(); ++g)
    {
        for (auto const& p : nets[g]->places())
        {
            for (size_t a = 0u; a < p.actions.size(); ++a)
            {
                file << "    bool C" << g << "_" << p.id << "_" << a << "() { return "
                     << conditions[g][p.id][a] << "; }";
                if (!p.actions[a].condition.empty())
                    file << " // " << p.actions[a].condition;
                file << "\n";
            }
        }
    }
    file << "\n";

    // Edges and timers.
    if (edges > 0u)
    {
        file << "    // Edges: the operand now, true while it changes.\n"
             << "    bool edgeInput(int const k)\n    {\n        switch (k)\n        {\n";
        for (size_t k = 0u; k < edges; ++k)
            file << "        case " << k << ": return " << translator.edges()[k] << ";\n";
        file << "        default: return false;\n        }\n    }\n"
             << "    bool rise(int const k) { return edgeInput(k) && !m_edge[k]; }\n"
             << "    bool fall(int const k) { return !edgeInput(k) && m_edge[k]; }\n\n";
    }
    if (timers > 0u)
    {
        file << "    // Time operators t1/a/t2: true t1 s after a became true, false t2 s\n"
             << "    // after it became false.\n"
             << "    bool timerInput(int const k)\n    {\n        switch (k)\n        {\n";
        for (size_t k = 0u; k < timers; ++k)
            file << "        case " << k << ": return " << translator.timers()[k].operand << ";\n";
        file << "        default: return false;\n        }\n    }\n"
             << "    bool timer(int const k) const { return m_timer[k].out; }\n"
             << "    void sample(int const k)\n    {\n"
             << "        bool const v = timerInput(k);\n"
             << "        if (v != m_timer[k].in) { m_timer[k].in = v; m_timer[k].elapsed = 0.0f; }\n"
             << "    }\n\n";
    }

    // Forcing orders.
    file << "    // Forcing orders of the active steps (read from the situation at the\n"
         << "    // beginning of the cycle): a forced graph does not evolve.\n"
         << "    void forcings()\n    {\n"
         << "        for (int i = 0; i < " << std::max(size_t(1u), nets.size()) << "; ++i) m_forced[i] = false;\n";
    for (size_t g = 0u; g < nets.size(); ++g)
    {
        for (auto const& p : nets[g]->places())
        {
            for (size_t a = 0u; a < p.actions.size(); ++a)
            {
                Action const& action = p.actions[a];
                if (action.forcings.empty())
                    continue;
                file << "        if (X" << g << "[" << p.id << "] && C" << g << "_" << p.id << "_" << a << "())\n        {\n";
                for (Forcing const& f : action.forcings)
                {
                    size_t const target = graphOf(f.targetNet);
                    if (target == nets.size())
                    {
                        file << "            // " << f.targetNet << ": unknown GRAFCET\n";
                        continue;
                    }
                    Net const& tn = *nets[target];
                    std::string const x = "X" + std::to_string(target);
                    file << "            m_forced[" << target << "] = true; // " << f.targetNet << ": "
                         << forcingTypeToStr(f.type) << "\n";
                    switch (f.type)
                    {
                    case Forcing::Type::Init:
                        for (auto const& q : tn.places())
                            file << "            " << x << "[" << q.id << "] = " << (q.tokens ? "true" : "false") << ";\n";
                        break;
                    case Forcing::Type::Freeze:
                        break;
                    case Forcing::Type::Empty:
                    case Forcing::Type::Steps:
                        file << "            for (int i = 0; i < " << tn.places().size() << "; ++i) " << x << "[i] = false;\n";
                        if (f.type == Forcing::Type::Steps)
                        {
                            for (size_t id : f.steps)
                            {
                                if (id < tn.places().size())
                                    file << "            " << x << "[" << id << "] = true;\n";
                            }
                        }
                        break;
                    }
                }
                file << "        }\n";
            }
        }
    }
    file << "    }\n\n";

    // Evolution.
    file << "    // Fire the transitions that can, all graphs together, until none can.\n"
         << "    bool evolve()\n    {\n"
         << "        for (int evolution = 0; evolution < 256; ++evolution)\n        {\n"
         << "            bool any = false;\n";
    for (size_t g = 0u; g < nets.size(); ++g)
    {
        Net const& net = *nets[g];
        if (net.transitions().empty())
            continue;
        file << "            bool F" << g << "[" << net.transitions().size() << "];\n";
        for (auto const& t : net.transitions())
        {
            file << "            F" << g << "[" << t.id << "] = R" << g << "_" << t.id << "()";
            for (auto const& a : net.arcsIn(t))
                file << " && X" << g << "[" << a.from.id << "]";
            if (t.delay > 0.0f)
                file << " && (m_delay" << g << "_" << t.id << " >= " << floatLiteral(t.delay) << ")";
            file << " && !m_forced[" << g << "];\n"
                 << "            any |= F" << g << "[" << t.id << "];\n";
        }
    }
    if (edges > 0u)
        file << "            // The instant is over: the edges are consumed.\n"
             << "            for (int k = 0; k < " << edges << "; ++k) m_edge[k] = edgeInput(k);\n";
    file << "            if (!any)\n                return true;\n"
         << "            // Deactivate the upstream steps, then activate the downstream ones.\n";
    for (size_t g = 0u; g < nets.size(); ++g)
    {
        Net const& net = *nets[g];
        for (auto const& t : net.transitions())
        {
            file << "            if (F" << g << "[" << t.id << "]) {";
            for (auto const& a : net.arcsIn(t))
                file << " X" << g << "[" << a.from.id << "] = false;";
            file << " }\n";
        }
    }
    for (size_t g = 0u; g < nets.size(); ++g)
    {
        Net const& net = *nets[g];
        for (auto const& t : net.transitions())
        {
            file << "            if (F" << g << "[" << t.id << "]) {";
            for (auto const& a : net.arcsOut(t))
                file << " X" << g << "[" << a.to.id << "] = true;";
            if (t.delay > 0.0f)
                file << " m_delay" << g << "_" << t.id << " = 0.0f;";
            file << " GRAFCET_FIRED(" << g << ", " << t.id << "); }\n";
        }
    }
    file << "        }\n        return false;\n    }\n\n";

    // Actions.
    file << "    // Actions of the steps (IEC 60848 qualifiers, conditions).\n"
         << "    struct Action\n    {\n        bool active = false;\n        float timer = 0.0f;\n        bool pending = false;\n    };\n\n"
         << "    void actions(float const dt)\n    {\n";
    for (auto const& it : outputs)
        file << "        out." << it.second << " = false;\n";
    for (size_t g = 0u; g < nets.size(); ++g)
    {
        Net const& net = *nets[g];
        for (auto const& p : net.places())
        {
            std::string const x = "X" + std::to_string(g) + "[" + std::to_string(p.id) + "]";
            std::string const was = "m_was" + std::to_string(g) + "[" + std::to_string(p.id) + "]";
            std::string const st = "m_step_time" + std::to_string(g) + "[" + std::to_string(p.id) + "]";
            file << "        // " << net.name << " X" << p.id << (p.caption.empty() ? "" : " " + p.caption) << "\n"
                 << "        " << st << " = (" << x << " && " << was << ") ? (" << st << " + dt) : 0.0f;\n";
            for (size_t a = 0u; a < p.actions.size(); ++a)
            {
                Action const& action = p.actions[a];
                std::string const s = "m_action" + std::to_string(g) + "_" + std::to_string(p.id) + "_" + std::to_string(a);
                std::string const c = "C" + std::to_string(g) + "_" + std::to_string(p.id) + "_" + std::to_string(a) + "()";
                std::string const d = floatLiteral(action.duration);
                std::string const rising = "(" + x + " && !" + was + ")";
                file << "        { // " << qualifierName(action.qualifier) << " " << action.name << "\n"
                     << "            bool const condition = " << c << ";\n";
                if (!action.forcings.empty())
                {
                    // Forcing orders: applied by forcings() at the next cycle.
                    file << "            " << s << ".active = " << x << " && condition;\n        }\n";
                    continue;
                }
                std::string const stored_var = "m_stored_" + outputs[action.name];
                switch (action.qualifier)
                {
                case Action::Qualifier::N:
                    file << "            " << s << ".active = " << x << " && condition;\n";
                    break;
                case Action::Qualifier::S:
                    file << "            if (" << rising << " && condition) " << stored_var << " = true;\n"
                         << "            " << s << ".active = " << x << " && " << stored_var << ";\n";
                    break;
                case Action::Qualifier::R:
                    file << "            if (" << x << " && condition) " << stored_var << " = false;\n"
                         << "            " << s << ".active = false;\n";
                    break;
                case Action::Qualifier::D:
                    file << "            " << s << ".timer = " << x << " ? " << st << " : 0.0f;\n"
                         << "            " << s << ".active = " << x << " && condition && (" << s << ".timer >= " << d << ");\n";
                    break;
                case Action::Qualifier::L:
                    file << "            " << s << ".timer = " << x << " ? " << st << " : 0.0f;\n"
                         << "            " << s << ".active = " << x << " && condition && (" << s << ".timer < " << d << ");\n";
                    break;
                case Action::Qualifier::DS:
                    file << "            " << s << ".timer = " << x << " ? " << st << " : 0.0f;\n"
                         << "            if (" << x << " && condition && (" << s << ".timer >= " << d << ")) " << stored_var << " = true;\n"
                         << "            " << s << ".active = " << stored_var << ";\n";
                    break;
                case Action::Qualifier::SD:
                    file << "            if (" << rising << " && condition) { " << s << ".timer = 0.0f; " << s << ".pending = true; }\n"
                         << "            else if (" << s << ".pending) " << s << ".timer += dt;\n"
                         << "            if (" << s << ".pending && (" << s << ".timer >= " << d << ")) { " << stored_var << " = true; " << s << ".pending = false; }\n"
                         << "            " << s << ".active = " << stored_var << ";\n";
                    break;
                case Action::Qualifier::SL:
                    file << "            if (" << rising << " && condition) { " << s << ".timer = 0.0f; " << s << ".pending = true; }\n"
                         << "            else if (" << s << ".pending) " << s << ".timer += dt;\n"
                         << "            if (" << s << ".pending && (" << s << ".timer >= " << d << ")) " << s << ".pending = false;\n"
                         << "            " << s << ".active = " << s << ".pending;\n";
                    break;
                case Action::Qualifier::P:
                    file << "            " << s << ".active = " << rising << " && condition;\n";
                    break;
                case Action::Qualifier::P0:
                    file << "            " << s << ".active = (!" << x << " && " << was << ") && condition;\n";
                    break;
                }
                if (action.forcings.empty() && (action.qualifier != Action::Qualifier::R))
                    file << "            if (" << s << ".active) out." << outputs[action.name] << " = true;\n";
                file << "        }\n";
            }
            file << "        " << was << " = " << x << ";\n";
        }
    }
    for (auto const& name : stored)
        file << "        if (m_stored_" << outputs[name] << ") out." << outputs[name] << " = true;\n";
    if (edges > 0u)
        file << "        for (int k = 0; k < " << edges << "; ++k) m_edge[k] = edgeInput(k);\n";
    if (timers > 0u)
        file << "        // The time operators see the changes of the evolution.\n"
             << "        for (int k = 0; k < " << timers << "; ++k) sample(k);\n";
    file << "    }\n\n";

    //==========================================================================
    // Debug: the protocol of Runtime/JournalPublisher.hpp
    //==========================================================================
    file << "#ifdef GRAFCET_DEBUG\n"
         << "    // A line of JSON, without the standard library.\n"
         << "    struct Line\n    {\n"
         << "        char text[" << std::max<size_t>(256u, 64u + 8u * std::max<size_t>(inputs.size() + outputs.size(), 1u) * 4u) << "];\n"
         << "        unsigned n = 0u;\n"
         << "        void put(char const* s) { while (*s && (n + 1u < sizeof(text))) text[n++] = *s++; text[n] = '\\0'; }\n"
         << "        void number(long v)\n        {\n"
         << "            char digits[12]; int i = 0;\n"
         << "            if (v < 0) { put(\"-\"); v = -v; }\n"
         << "            do { digits[i++] = char('0' + (v % 10)); v /= 10; } while (v > 0);\n"
         << "            while (i > 0) { char c[2] = { digits[--i], '\\0' }; put(c); }\n"
         << "        }\n"
         << "        void date(float t)\n        {\n"
         << "            long const ms = long(t * 1000.0f + 0.5f);\n"
         << "            number(ms / 1000); put(\".\");\n"
         << "            long const f = ms % 1000; if (f < 100) put(\"0\"); if (f < 10) put(\"0\"); number(f);\n"
         << "        }\n"
         << "    };\n\n"
         << "    void send(Line const& line)\n    {\n"
         << "#  ifdef GRAFCET_DEBUG_ZEROMQ\n"
         << "        static void* context = nullptr;\n"
         << "        static void* socket = nullptr;\n"
         << "        if (context == nullptr)\n        {\n"
         << "            context = zmq_ctx_new();\n"
         << "            socket = zmq_socket(context, ZMQ_PUB);\n"
         << "            zmq_bind(socket, GRAFCET_DEBUG_ENDPOINT);\n"
         << "        }\n"
         << "        zmq_send(socket, line.text, line.n, ZMQ_DONTWAIT);\n"
         << "#  else\n"
         << "        GRAFCET_DEBUG_SEND(line.text);\n"
         << "#  endif\n"
         << "    }\n\n";

    // Firings of the cycle.
    file << "    void fired(int g, int t) { if (m_fired < 64) { m_fired_graph[m_fired] = g; m_fired_transition[m_fired] = t; ++m_fired; } }\n\n";

    // publish()
    file << "    //! \\brief The firings of the cycle, the steps and the inputs and outputs\n"
         << "    //! when they changed, the names of the nodes every second.\n"
         << "    void publish()\n    {\n"
         << "        static char const* const graphs[] = {";
    for (size_t g = 0u; g < nets.size(); ++g)
        file << (g ? ", " : " ") << "\"" << jsonLiteral(nets[g]->name).substr(2u, jsonLiteral(nets[g]->name).size() - 4u) << "\"";
    file << " };\n"
         << "        bool const names = (m_published < 0.0f) || (m_time - m_published >= 1.0f);\n"
         << "        if (names)\n        {\n"
         << "            m_published = m_time;\n";
    for (size_t g = 0u; g < nets.size(); ++g)
    {
        Net const& net = *nets[g];
        std::string manifest = "{\\\"type\\\":\\\"manifest\\\",\\\"net\\\":" + jsonLiteral(net.name) + ",\\\"places\\\":[";
        for (auto const& p : net.places())
            manifest += std::string(p.id ? "," : "") + jsonLiteral(p.key);
        manifest += "],\\\"transitions\\\":[";
        for (auto const& t : net.transitions())
            manifest += std::string(t.id ? "," : "") + jsonLiteral(t.key);
        manifest += "],\\\"unregistered\\\":[]}";
        file << "            { Line l; l.put(\"" << manifest << "\"); send(l); }\n";
    }
    file << "        }\n"
         << "        for (int i = 0; i < m_fired; ++i)\n        {\n"
         << "            for (int e = 0; e < 2; ++e)\n            {\n"
         << "                Line l;\n"
         << "                l.put(\"{\\\"instance\\\":\\\"\"); l.put(graphs[m_fired_graph[i]]);\n"
         << "                l.put(\"\\\",\\\"transition\\\":\\\"T\"); l.number(m_fired_transition[i]);\n"
         << "                l.put(e ? \"\\\",\\\"event\\\":\\\"Completed\\\",\\\"result\\\":\\\"Success\\\",\\\"date\\\":\" : \"\\\",\\\"event\\\":\\\"Scheduled\\\",\\\"date\\\":\");\n"
         << "                l.date(m_time); l.put(\"}\");\n"
         << "                send(l);\n"
         << "            }\n"
         << "        }\n"
         << "        bool const changed = (m_fired > 0) || names;\n"
         << "        m_fired = 0;\n"
         << "        if (changed)\n        {\n";
    for (size_t g = 0u; g < nets.size(); ++g)
    {
        file << "            { Line l; l.put(\"{\\\"type\\\":\\\"marking\\\",\\\"instance\\\":\\\"\"); l.put(graphs[" << g
             << "]); l.put(\"\\\",\\\"date\\\":\"); l.date(m_time); l.put(\",\\\"tokens\\\":[\");\n"
             << "              for (int p = 0; p < " << nets[g]->places().size() << "; ++p) { if (p) l.put(\",\"); l.put(X" << g
             << "[p] ? \"1\" : \"0\"); }\n"
             << "              l.put(\"]}\"); send(l); }\n";
    }
    file << "        }\n"
         << "        // Inputs and outputs, when they changed.\n"
         << "        Line l;\n"
         << "        l.put(\"{\\\"type\\\":\\\"io\\\",\\\"inputs\\\":{\");\n";
    bool first = true;
    for (auto const& it : inputs)
    {
        file << "        l.put(\"" << (first ? "" : ",") << "\\\"" << it.first << "\\\":\"); l.number(in." << it.first << ");\n";
        first = false;
    }
    file << "        l.put(\"},\\\"outputs\\\":{\");\n";
    first = true;
    for (auto const& it : outputs)
    {
        file << "        l.put(\"" << (first ? "" : ",") << jsonLiteral(it.first) << ":\"); l.put(out." << it.second
             << " ? \"true\" : \"false\");\n";
        first = false;
    }
    file << "        l.put(\"}}\");\n"
         << "        bool same = (l.n == m_io.n);\n"
         << "        for (unsigned i = 0u; same && (i < l.n); ++i) same = (l.text[i] == m_io.text[i]);\n"
         << "        if (!same || names) { m_io = l; send(l); }\n"
         << "    }\n\n"
         << "    int m_fired = 0;\n"
         << "    int m_fired_graph[64];\n"
         << "    int m_fired_transition[64];\n"
         << "    float m_published = -1.0f;\n"
         << "    Line m_io;\n"
         << "#endif\n\n";

    // Members.
    file << "    float m_time = 0.0f;\n"
         << "    bool m_forced[" << std::max(size_t(1u), nets.size()) << "];\n";
    if (edges > 0u)
        file << "    bool m_edge[" << edges << "];\n";
    if (timers > 0u)
    {
        file << "    struct Timer { bool in; bool out; float elapsed; } m_timer[" << timers << "];\n"
             << "    float const T1[" << timers << "] = {";
        for (size_t k = 0u; k < timers; ++k)
            file << (k ? ", " : " ") << translator.timers()[k].t1;
        file << " };\n    float const T2[" << timers << "] = {";
        for (size_t k = 0u; k < timers; ++k)
            file << (k ? ", " : " ") << translator.timers()[k].t2;
        file << " };\n";
    }
    for (size_t g = 0u; g < nets.size(); ++g)
    {
        size_t const places = std::max(size_t(1u), nets[g]->places().size());
        file << "    bool m_was" << g << "[" << places << "];\n"
             << "    float m_step_time" << g << "[" << places << "];\n";
        for (auto const& t : nets[g]->transitions())
        {
            if (t.delay > 0.0f)
                file << "    float m_delay" << g << "_" << t.id << " = 0.0f;\n";
        }
        for (auto const& p : nets[g]->places())
        {
            for (size_t a = 0u; a < p.actions.size(); ++a)
                file << "    Action m_action" << g << "_" << p.id << "_" << a << ";\n";
        }
    }
    for (auto const& name : stored)
        file << "    bool m_stored_" << outputs[name] << " = false;\n";
    file << "};\n\n"
         << "#undef GRAFCET_FIRED\n\n"
         << "} // namespace " << space << "\n\n";

    //==========================================================================
    // Test on a computer
    //==========================================================================
    file << "#ifdef GRAFCET_STANDALONE_TEST\n"
         << "#include <chrono>\n#include <cstdio>\n#include <thread>\n\n"
         << "// Runs the cycles every 100 ms and prints the active steps when they change.\n"
         << "int main()\n{\n"
         << "    " << space << "::Grafcet grafcet;\n"
         << "    for (int cycle = 0;; ++cycle)\n    {\n"
         << "        if (!grafcet.cycle(0.1f))\n"
         << "            std::printf(\"No stable situation at t = %.1f s\\n\", double(grafcet.time()));\n"
         << "        if (cycle % 10 == 0)\n        {\n"
         << "            std::printf(\"t = %.1f s:\", double(grafcet.time()));\n";
    for (size_t g = 0u; g < nets.size(); ++g)
        file << "            for (int p = 0; p < " << nets[g]->places().size() << "; ++p) if (grafcet.X" << g
             << "[p]) std::printf(\" " << identifier(nets[g]->name) << ":X%d\", p);\n";
    file << "            std::printf(\"\\n\");\n"
         << "        }\n"
         << "        std::this_thread::sleep_for(std::chrono::milliseconds(100));\n"
         << "    }\n}\n"
         << "#endif // GRAFCET_STANDALONE_TEST\n\n"
         << "#endif // GRAFCET_" << guard << "_HPP\n";
    return {};
}

//------------------------------------------------------------------------------
std::string exportToGrafcetCpp(Net const& net, std::string const& filename)
{
    return exportGrafcetsToCpp({ net }, filename);
}

} // namespace tpne
