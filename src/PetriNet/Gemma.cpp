// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "PetriNet/Gemma.hpp"
#include "PetriNet/PetriNet.hpp"

#include <algorithm>
#include <cctype>
#include <set>

namespace tpne {

//------------------------------------------------------------------------------
Gemma::State* Gemma::find(std::string const& code)
{
    for (auto& state : states)
    {
        if (state.code == code)
            return &state;
    }
    return nullptr;
}

//------------------------------------------------------------------------------
Gemma::State const* Gemma::find(std::string const& code) const
{
    return const_cast<Gemma*>(this)->find(code);
}

//------------------------------------------------------------------------------
std::vector<GemmaProcedure> const& gemmaProcedures()
{
    using P = Gemma::Production;
    static std::vector<GemmaProcedure> const procedures = {
        { "A1", "Arrêt dans l'état initial", "Stop in the initial state",
          "The machine is stopped in its initial state, ready to start.",
          "La machine est arrêtée dans son état initial, prête à démarrer.", P::Init },
        { "A2", "Arrêt demandé en fin de cycle", "Stop asked at the end of the cycle",
          "The stop is asked: the cycle goes on to its end.",
          "L'arrêt est demandé : le cycle continue jusqu'à sa fin.", P::Run },
        { "A3", "Arrêt demandé dans un état déterminé", "Stop asked in a given state",
          "The stop is asked: the machine goes on to a given state, not the end of the cycle.",
          "L'arrêt est demandé : la machine continue jusqu'à un état déterminé, autre que la fin du cycle.",
          P::Run },
        { "A4", "Arrêt obtenu", "Stop obtained",
          "The machine is stopped in another state than the initial one.",
          "La machine est arrêtée dans un autre état que l'état initial.", P::Freeze },
        { "A5", "Préparation pour remise en route après défaillance",
          "Preparation of the restart after a failure",
          "After a failure: clear, repair, check before the restart.",
          "Après une défaillance : dégager, réparer, vérifier avant de repartir.", P::Init },
        { "A6", "Mise P.O. dans l'état initial", "Setting the operative part in its initial state",
          "The operative part goes back to its initial state (by hand or automatically).",
          "La partie opérative revient à son état initial (à la main ou automatiquement).", P::Init },
        { "A7", "Mise P.O. dans un état déterminé", "Setting the operative part in a given state",
          "The operative part goes to a given state, to restart from it.",
          "La partie opérative va dans un état déterminé, pour repartir de là.", P::Init },
        { "F1", "Production normale", "Normal production",
          "The reason the machine exists: it produces.",
          "La raison d'être de la machine : elle produit.", P::Run },
        { "F2", "Marches de préparation", "Preparation run",
          "Before the production: heating, filling, setting.",
          "Avant la production : chauffe, remplissage, réglage.", P::Init },
        { "F3", "Marches de clôture", "Closing run",
          "After the production: emptying, cleaning.",
          "Après la production : vidange, nettoyage.", P::Init },
        { "F4", "Marches de vérification dans le désordre", "Verification run out of order",
          "Each movement by hand, in any order.",
          "Chaque mouvement à la main, dans n'importe quel ordre.", P::Init },
        { "F5", "Marches de vérification dans l'ordre", "Verification run in order",
          "The cycle step by step, in its order.",
          "Le cycle pas à pas, dans son ordre.", P::Run },
        { "F6", "Marches de test", "Test run",
          "Calibration, tests of the sensors.",
          "Étalonnage, tests des capteurs.", P::Init },
        { "D1", "Arrêt d'urgence", "Emergency stop",
          "Everything that is dangerous stops at once (forced by Safety).",
          "Tout ce qui est dangereux s'arrête aussitôt (forcé par Safety).", P::Init },
        { "D2", "Diagnostic et/ou traitement de défaillance", "Diagnosis and treatment of the failure",
          "A failure is found: the machine stops and waits for the operator.",
          "Une défaillance est détectée : la machine s'arrête et attend l'opérateur.", P::Freeze },
        { "D3", "Production tout de même", "Production despite the failure",
          "Degraded mode: the production goes on despite the failure.",
          "Marche dégradée : la production continue malgré la défaillance.", P::Run },
    };
    return procedures;
}

//------------------------------------------------------------------------------
GemmaProcedure const* gemmaProcedure(std::string const& code)
{
    for (auto const& procedure : gemmaProcedures())
    {
        if (code == procedure.code)
            return &procedure;
    }
    return nullptr;
}

//------------------------------------------------------------------------------
std::string gemmaName(std::string const& code, std::string const& language)
{
    GemmaProcedure const* procedure = gemmaProcedure(code);
    if (procedure == nullptr)
        return code;
    return (language == "fr") ? procedure->fr : procedure->en;
}

//------------------------------------------------------------------------------
std::vector<Gemma::Link> const& gemmaStandardLinks()
{
    // The arrows of the form (doc/gemma/GEMMA.drawio).
    static std::vector<Gemma::Link> const links = {
        { "A1", "F1", "Start" },
        { "A1", "F2", "Start" },
        { "A1", "F4", "Manual" },
        { "A1", "F5", "StepByStep" },
        { "A1", "F6", "Test" },
        { "A2", "A1", "Production:X0" },
        { "A3", "A4", "Production:X0" },
        { "A4", "F1", "Start" },
        { "A5", "A6", "Ack" },
        { "A5", "A7", "Resume" },
        { "A6", "A1", "InitialPosition" },
        { "A7", "A4", "GivenPosition" },
        { "D1", "A5", "Safety:X0" },
        { "D1", "D2", "Safety:X0 . Diagnose" },
        { "D1", "F6", "Safety:X0 . Test" },
        { "D2", "A5", "Ack" },
        { "D2", "F6", "Test" },
        { "D3", "A2", "Stop" },
        { "D3", "A3", "Pause" },
        { "D3", "D2", "Fault" },
        { "F1", "A2", "Stop" },
        { "F1", "A3", "Pause" },
        { "F1", "D1", "EStop" },
        { "F1", "D2", "Fault" },
        { "F1", "D3", "Degraded" },
        { "F1", "F3", "Close" },
        { "F1", "F4", "Manual" },
        { "F1", "F5", "StepByStep" },
        { "F1", "F6", "Test" },
        { "F2", "F1", "Ready" },
        { "F3", "A1", "Closed" },
        { "F4", "A6", "/Manual" },
    };
    return links;
}

//------------------------------------------------------------------------------
char const* to_str(Gemma::Production const production)
{
    switch (production)
    {
    case Gemma::Production::Run: return "run";
    case Gemma::Production::Freeze: return "freeze";
    case Gemma::Production::Init: return "init";
    }
    return "init";
}

//------------------------------------------------------------------------------
Gemma::Production gemmaProductionFromStr(std::string const& text)
{
    if (text == "run")
        return Gemma::Production::Run;
    if (text == "freeze")
        return Gemma::Production::Freeze;
    return Gemma::Production::Init;
}

//------------------------------------------------------------------------------
Gemma defaultGemma()
{
    Gemma gemma;
    for (char const* code : { "A1", "F1", "A2", "D1", "A5", "A6" })
        gemma.states.push_back({ code, gemmaProcedure(code)->production });
    for (auto const& link : gemmaStandardLinks())
    {
        // F1 -> D1: Safety already forces D1 from every state.
        if ((gemma.find(link.from) != nullptr) && (gemma.find(link.to) != nullptr) &&
            !((link.from == std::string("F1")) && (link.to == std::string("D1"))))
            gemma.links.push_back(link);
    }
    return gemma;
}

//------------------------------------------------------------------------------
//! \brief The procedures kept, in the order of the guide.
static std::vector<std::string> keptCodes(Gemma const& gemma)
{
    std::vector<std::string> codes;
    for (auto const& procedure : gemmaProcedures())
    {
        if (gemma.find(procedure.code) != nullptr)
            codes.push_back(procedure.code);
    }
    return codes;
}

//------------------------------------------------------------------------------
std::vector<GemmaIssue> checkGemma(Gemma const& gemma)
{
    std::vector<GemmaIssue> issues;
    if (gemma.find("A1") == nullptr)
    {
        issues.push_back({ true, "A1", -1, "A1 is not kept: it is the initial step of Control" });
        return issues;
    }
    if (gemma.find("D1") == nullptr)
        issues.push_back({ false, "D1", -1, "No D1: the emergency stop forces Control to its initial step" });

    // The links: between kept procedures, with a valid condition.
    std::map<std::string, std::vector<std::string>> next;
    std::map<std::string, std::vector<std::string>> previous;
    for (size_t i = 0u; i < gemma.links.size(); ++i)
    {
        Gemma::Link const& link = gemma.links[i];
        std::string const name = link.from + " -> " + link.to;
        if ((gemma.find(link.from) == nullptr) || (gemma.find(link.to) == nullptr))
        {
            issues.push_back({ false, "", int(i), name + ": a procedure is not kept, the link is ignored" });
            continue;
        }
        if (link.condition.empty())
            issues.push_back({ true, link.from, int(i), name + ": no condition" });
        else
        {
            std::string const error = Receptivity::check(link.condition);
            if (!error.empty())
                issues.push_back({ true, link.from, int(i), name + ": " + error });
        }
        next[link.from].push_back(link.to);
        previous[link.to].push_back(link.from);
        for (size_t j = 0u; j < i; ++j)
        {
            Gemma::Link const& other = gemma.links[j];
            if ((other.from == link.from) && (other.condition == link.condition) && (other.to != link.to))
                issues.push_back({ false, link.from, int(i), name + " and " + other.from + " -> " + other.to +
                                   ": same condition, the two would fire together" });
        }
    }

    // Reached from A1, and from everywhere for D1 (the emergency stop).
    auto const walk = [](std::string const& start, std::map<std::string, std::vector<std::string>> const& edges,
                         std::set<std::string>& seen) {
        std::vector<std::string> todo = { start };
        while (!todo.empty())
        {
            std::string const code = todo.back();
            todo.pop_back();
            if (!seen.insert(code).second)
                continue;
            auto const it = edges.find(code);
            if (it != edges.end())
                todo.insert(todo.end(), it->second.begin(), it->second.end());
        }
    };
    std::set<std::string> reached;
    walk("A1", next, reached);
    if (gemma.find("D1") != nullptr)
        walk("D1", next, reached);
    std::set<std::string> back;
    walk("A1", previous, back);
    for (std::string const& code : keptCodes(gemma))
    {
        if ((next.count(code) == 0u) && (code != "A1"))
            issues.push_back({ true, code, -1, code + " has no way out" });
        else if (reached.count(code) == 0u)
            issues.push_back({ true, code, -1, code + " is not reached from A1" });
        else if (back.count(code) == 0u)
            issues.push_back({ true, code, -1, code + " has no way back to A1" });
    }
    if (next.count("A1") == 0u)
        issues.push_back({ true, "A1", -1, "A1 has no way out: the machine never starts" });
    return issues;
}

//------------------------------------------------------------------------------
//! \brief The procedures kept, A1 first, then in the order of the guide: the
//! order of the steps of Control.
static std::vector<std::string> stepOrder(Gemma const& gemma)
{
    std::vector<std::string> codes = keptCodes(gemma);
    std::stable_partition(codes.begin(), codes.end(), [](std::string const& c) { return c == "A1"; });
    return codes;
}

//------------------------------------------------------------------------------
std::map<std::string, size_t> gemmaStepNumbers(Gemma const& gemma)
{
    std::map<std::string, size_t> steps;
    for (std::string const& code : stepOrder(gemma))
        steps.emplace(code, steps.size());
    return steps;
}

//------------------------------------------------------------------------------
//! \brief A step with its caption and its tokens.
static size_t step(Net& net, std::string const& caption, bool initial)
{
    return net.addPlace(initial ? 1u : 0u, caption).id;
}

//------------------------------------------------------------------------------
//! \brief A transition from a step to another one.
static void link(Net& net, size_t from, size_t to, std::string const& condition)
{
    Transition& t = net.addTransition(condition);
    net.addArc(net.places()[from], t);
    net.addArc(t, net.places()[to]);
}

//------------------------------------------------------------------------------
//! \brief An action holding only forcing orders.
static void force(Net& net, size_t place, std::vector<Forcing> const& forcings, std::string const& text)
{
    Action action;
    action.script = text;
    action.forcings = forcings;
    net.places()[place].actions.push_back(action);
}

//------------------------------------------------------------------------------
void generateGemma(Gemma const& gemma, Net& safety, Net& control)
{
    bool const fr = (gemma.language == "fr");

    // Control: one step per procedure kept, A1 first (initial).
    control.reset(TypeOfNet::GRAFCET);
    control.name = "Control";
    control.description = fr
        ? "GMMA : les modes de marche et d'arrêt gardés du GEMMA. Chaque étape force Production selon le mode."
        : "GMMA: the run and stop modes kept from the GEMMA. Each step forces Production as the mode asks.";
    std::map<std::string, size_t> steps;
    std::vector<std::string> const codes = stepOrder(gemma);
    for (std::string const& code : codes)
        steps[code] = step(control, code + " " + gemmaName(code, gemma.language), code == "A1");
    for (auto const& l : gemma.links)
    {
        auto const from = steps.find(l.from);
        auto const to = steps.find(l.to);
        if ((from != steps.end()) && (to != steps.end()))
            link(control, from->second, to->second, l.condition);
    }
    for (std::string const& code : codes)
    {
        Gemma::Production const production = gemma.find(code)->production;
        if (production == Gemma::Production::Run)
            continue;
        Forcing forcing;
        forcing.targetNet = "Production";
        forcing.type = (production == Gemma::Production::Freeze) ? Forcing::Type::Freeze : Forcing::Type::Init;
        force(control, steps[code], { forcing }, (production == Gemma::Production::Freeze)
              ? (fr ? "Production figée" : "Production frozen")
              : (fr ? "Production dans sa situation initiale" : "Production in its initial situation"));
    }

    // Safety: watch, and the emergency stop.
    safety.reset(TypeOfNet::GRAFCET);
    safety.name = "Safety";
    safety.description = fr
        ? "Sécurité : tant que l'arrêt d'urgence est enfoncé, puis jusqu'au réarmement (Rearm), Control est forcé en D1 et Production dans sa situation initiale."
        : "Safety: while the emergency stop is pushed, then until the rearm (Rearm), Control is forced to D1 and Production to its initial situation.";
    size_t const watch = step(safety, fr ? "Surveillance" : "Watch", true);
    size_t const emergency = step(safety, fr ? "Arrêt d'urgence" : "Emergency stop", false);
    link(safety, watch, emergency, "EStop");
    link(safety, emergency, watch, "/EStop . Rearm");
    Forcing to_control;
    to_control.targetNet = "Control";
    auto const d1 = steps.find("D1");
    if (d1 != steps.end())
    {
        to_control.type = Forcing::Type::Steps;
        to_control.steps = { d1->second };
    }
    Forcing to_production;
    to_production.targetNet = "Production";
    force(safety, emergency, { to_control, to_production }, (d1 != steps.end())
          ? (fr ? "Control en D1, Production dans sa situation initiale"
                : "Control in D1, Production in its initial situation")
          : (fr ? "Control et Production dans leur situation initiale"
                : "Control and Production in their initial situation"));
}

//------------------------------------------------------------------------------
void gemmaProductionTemplate(Gemma const& gemma, Net const& control, Net& production)
{
    bool const fr = (gemma.language == "fr");
    std::map<std::string, size_t> const steps = gemmaSteps(control);
    auto const f1 = steps.find("F1");
    production.reset(TypeOfNet::GRAFCET);
    production.name = "Production";
    production.description = fr
        ? "Production normale (F1), à compléter : elle attend F1, fait un cycle, et revient."
        : "Normal production (F1), to complete: it waits for F1, runs a cycle, and comes back.";
    size_t const wait = step(production, fr ? "Attente" : "Wait", true);
    size_t const cycle = step(production, fr ? "Cycle (à compléter)" : "Cycle (to complete)", false);
    link(production, wait, cycle, (f1 != steps.end()) ? "Control:X" + std::to_string(f1->second) : "Start");
    link(production, cycle, wait, "5s/X" + std::to_string(cycle));
}

//------------------------------------------------------------------------------
std::map<std::string, size_t> gemmaSteps(Net const& control)
{
    std::map<std::string, size_t> steps;
    for (auto const& place : control.places())
    {
        std::string const code = place.caption.substr(0u, place.caption.find(' '));
        if (gemmaProcedure(code) != nullptr)
            steps[code] = place.id;
    }
    return steps;
}

//------------------------------------------------------------------------------
void renumberControlSteps(Net& net, std::map<std::string, size_t> const& before,
                          std::map<std::string, size_t> const& after)
{
    // Old step -> new step of the same procedure (none: kept as it is).
    std::map<size_t, size_t> renumber;
    for (auto const& [code, old] : before)
    {
        auto const it = after.find(code);
        if (it != after.end())
            renumber[old] = it->second;
    }
    auto const to = [&](size_t old) {
        auto const it = renumber.find(old);
        return (it == renumber.end()) ? old : it->second;
    };

    // Control:X<n> in the receptivities.
    std::string const key = "Control:X";
    for (auto& transition : net.transitions())
    {
        std::string& text = transition.caption;
        size_t at = 0u;
        while ((at = text.find(key, at)) != std::string::npos)
        {
            size_t const first = at + key.size();
            size_t last = first;
            while ((last < text.size()) && std::isdigit(static_cast<unsigned char>(text[last])))
                ++last;
            if (last > first)
            {
                std::string const number = std::to_string(to(std::stoul(text.substr(first, last - first))));
                text.replace(first, last - first, number);
                last = first + number.size();
            }
            at = last;
        }
    }
    // Control{n} in the forcing orders.
    for (auto& place : net.places())
    {
        for (auto& action : place.actions)
        {
            for (auto& forcing : action.forcings)
            {
                if ((forcing.targetNet == "Control") && (forcing.type == Forcing::Type::Steps))
                {
                    for (auto& s : forcing.steps)
                        s = to(s);
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
void keepControlActions(Net const& before, Net& after)
{
    std::map<std::string, size_t> const steps = gemmaSteps(after);
    for (auto const& place : before.places())
    {
        auto const it = steps.find(place.caption.substr(0u, place.caption.find(' ')));
        if (it == steps.end())
            continue;
        for (auto const& action : place.actions)
        {
            // Made by generateGemma(): only the forcing of Production.
            bool const made = action.name.empty() && (action.forcings.size() == 1u) &&
                              (action.forcings[0].targetNet == "Production");
            if (!made)
                after.places()[it->second].actions.push_back(action);
        }
    }
}

//------------------------------------------------------------------------------
std::vector<std::string> gemmaInputNames(Gemma const& gemma)
{
    // Identifiers, without the graphs (Name:...), their steps (X3) and the
    // time operators (3s/X1).
    std::set<std::string> names;
    for (auto const& l : gemma.links)
    {
        std::string const& text = l.condition;
        size_t i = 0u;
        while (i < text.size())
        {
            unsigned char const c = static_cast<unsigned char>(text[i]);
            if (!std::isalnum(c) && (c != '_'))
            {
                ++i;
                continue;
            }
            size_t const first = i;
            while ((i < text.size()) && (std::isalnum(static_cast<unsigned char>(text[i])) || (text[i] == '_')))
                ++i;
            std::string const word = text.substr(first, i - first);
            bool const after_graph = (first > 0u) && (text[first - 1u] == ':');
            bool const graph = (i < text.size()) && (text[i] == ':');
            bool const number = std::isdigit(static_cast<unsigned char>(word[0])) != 0;
            bool const step_ref = (word.size() > 1u) && (word[0] == 'X') &&
                std::all_of(word.begin() + 1, word.end(), [](char d) { return std::isdigit(static_cast<unsigned char>(d)) != 0; });
            if (!after_graph && !graph && !number && !step_ref)
                names.insert(word);
        }
    }
    return { names.begin(), names.end() };
}

} // namespace tpne
