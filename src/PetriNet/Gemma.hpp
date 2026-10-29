// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef TPNE_GEMMA_HPP
#  define TPNE_GEMMA_HPP

#  include <cstddef>
#  include <map>
#  include <string>
#  include <vector>

namespace tpne {

class Net;

// *****************************************************************************
//! \brief A GEMMA (Guide d'Étude des Modes de Marches et d'Arrêts, ADEPA
//! 1981): the procedures (modes) a machine keeps among the 16 of the guide,
//! and the links between them with their conditions. It is the GMMA (graph
//! of the run and stop modes) of the machine. Saved with the document; the
//! editor makes from it the GRAFCETs Safety, Control and Production.
// *****************************************************************************
struct Gemma
{
    //! \brief What the production does while the procedure is active: Control
    //! forces Production to it.
    enum class Production
    {
        Run,    //!< No forcing: the production runs.
        Init,   //!< Production{init}: kept in its initial situation.
        Freeze  //!< Production{*}: frozen where it is.
    };

    //! \brief A procedure kept: its code ("A1" ... "D3").
    struct State
    {
        std::string code;
        Production production = Production::Init;
    };

    //! \brief A link between two procedures: a transition of Control.
    struct Link
    {
        std::string from;
        std::string to;
        //! \brief Receptivity of the transition ("Start", "Production:X0").
        std::string condition;
    };

    //! \brief Names of the procedures: "fr" or "en".
    std::string language = "en";
    std::vector<State> states;
    std::vector<Link> links;

    //! \brief The procedure of this code, or null when not kept.
    State* find(std::string const& code);
    State const* find(std::string const& code) const;
};

// *****************************************************************************
//! \brief A procedure of the guide.
// *****************************************************************************
struct GemmaProcedure
{
    //! \brief "A1" ... "D3".
    char const* code;
    //! \brief Its name in French and in English.
    char const* fr;
    char const* en;
    //! \brief What it is for, in English and in French.
    char const* help_en;
    char const* help_fr;
    //! \brief What the production does there by default.
    Gemma::Production production;
};

//! \brief The 16 procedures of the guide, by family: A1 ... A7, F1 ... F6,
//! D1 ... D3.
std::vector<GemmaProcedure> const& gemmaProcedures();

//! \brief The procedure of this code, or null.
GemmaProcedure const* gemmaProcedure(std::string const& code);

//! \brief Name of the procedure in the language ("fr" or "en").
std::string gemmaName(std::string const& code, std::string const& language);

//! \brief The arrows of the form of the guide (doc/gemma/GEMMA.drawio), with a
//! condition to start from.
std::vector<Gemma::Link> const& gemmaStandardLinks();

//! \brief "run", "init", "freeze" and back.
char const* to_str(Gemma::Production production);
Gemma::Production gemmaProductionFromStr(std::string const& text);

//! \brief A GEMMA to start from: A1, F1, A2 (normal start and stop), D1, A5,
//! A6 (emergency stop and restart), linked.
Gemma defaultGemma();

// *****************************************************************************
//! \brief A fault of a GEMMA found by checkGemma().
// *****************************************************************************
struct GemmaIssue
{
    //! \brief An error makes wrong GRAFCETs; a warning may be wanted.
    bool error = true;
    //! \brief The procedure concerned ("" for the whole GEMMA).
    std::string code;
    //! \brief The link concerned (index in Gemma::links), or -1.
    int link = -1;
    std::string message;
};

//------------------------------------------------------------------------------
//! \brief Check the GEMMA: A1 kept, links between kept procedures with a
//! valid condition, each procedure reached from A1 (D1 is reached from
//! everywhere by the emergency stop), each one with a way back to A1, two
//! links leaving one procedure with the same condition.
//------------------------------------------------------------------------------
std::vector<GemmaIssue> checkGemma(Gemma const& gemma);

//------------------------------------------------------------------------------
//! \brief Make the GRAFCETs of the GEMMA (without positions: arrange them).
//! Control: one step per procedure (A1 initial), named "<code> <name>", one
//! transition per link, the forcing of Production of each procedure.
//! Safety: "watch", and the emergency stop that forces Control to D1 (to its
//! initial situation without D1) and Production to its initial situation
//! until the emergency stop is released and Rearm pushed.
//------------------------------------------------------------------------------
void generateGemma(Gemma const& gemma, Net& safety, Net& control);

//------------------------------------------------------------------------------
//! \brief A Production to complete, when the document has none: it waits
//! for F1 (Control), runs a cycle (5 s to change), and comes back.
//------------------------------------------------------------------------------
void gemmaProductionTemplate(Gemma const& gemma, Net const& control, Net& production);

//! \brief Step of each procedure kept in the Control generateGemma() makes:
//! A1 is step 0, the others follow in the order of the guide.
std::map<std::string, size_t> gemmaStepNumbers(Gemma const& gemma);

//! \brief Step of each procedure in a Control made by generateGemma() (from
//! the code at the start of the captions).
std::map<std::string, size_t> gemmaSteps(Net const& control);

//------------------------------------------------------------------------------
//! \brief A Control made again numbers its steps again: change the steps of
//! Control read (Control:X3) or forced (Control{3}) by another graph, from
//! the old steps to the new ones of the same procedures.
//------------------------------------------------------------------------------
void renumberControlSteps(Net& net, std::map<std::string, size_t> const& before,
                          std::map<std::string, size_t> const& after);

//------------------------------------------------------------------------------
//! \brief The actions the user added to the steps of an old Control (not the
//! forcing of Production made by generateGemma()) go to the steps of the
//! same procedures in the new one.
//------------------------------------------------------------------------------
void keepControlActions(Net const& before, Net& after);

//! \brief Names read by the conditions of the links (inputs, other graphs).
std::vector<std::string> gemmaInputNames(Gemma const& gemma);

} // namespace tpne

#endif
