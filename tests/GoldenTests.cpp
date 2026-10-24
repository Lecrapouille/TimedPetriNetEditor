// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "main.hpp"
#include "PetriNet/PetriNet.hpp"
#include "IO/IO.hpp"
#include "PetriNet/Engine.hpp"
#include "PetriNet/GrafcetEngine.hpp"
#include "IO/Imports/Imports.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>

// *****************************************************************************
//! \file Non regression of the examples: each net of examples/ is run
//! the same way each time (no random choice) and its journal is compared to
//! the one of tests/golden/. Set TPNE_UPDATE_GOLDEN=1 to write the journals
//! again after a wanted change of behavior (then check their diff).
// *****************************************************************************

using namespace ::tpne;
namespace fs = std::filesystem;

//------------------------------------------------------------------------------
static std::string number(double const value)
{
    char text[32];
    std::snprintf(text, sizeof(text), "%.6g", value);
    return text;
}

//------------------------------------------------------------------------------
static std::string marking(Marking const& m)
{
    std::string text;
    for (size_t const tokens : m)
        text += (text.empty() ? "" : " ") + std::to_string(tokens);
    return text;
}

//------------------------------------------------------------------------------
//! \brief Petri nets and timed nets: the engine fires every enabled transition
//! (the first declared one first) during 60 time units, at most 300 firings.
//! Untimed nets get 1 time unit on each arc (else they fire for ever at 0).
static std::string runPetri(Net const& net)
{
    std::ostringstream journal;
    Engine::Options options;
    options.policy = Engine::Policy::FirstDeclared;
    options.timed = (net.type() != TypeOfNet::PetriNet);
    options.untimed_duration = 1.0;
    options.max_firings_per_date = 200u;
    Engine engine(net, options);
    size_t firings = 0u;
    engine.onFire = [&](size_t t, double date, size_t k) {
        if (++firings <= 300u)
            journal << "t=" << number(date) << " " << net.transitions()[t].key << " k=" << k << "\n";
    };
    engine.onWarning = [&](std::string const& w) { journal << "warning: " << w << "\n"; };
    engine.reset();
    journal << "initial: " << marking(engine.marking()) << "\n";
    for (int step = 1; (step <= 600) && (firings <= 300u) && !engine.isDeadlocked(); ++step)
        engine.advanceTo(0.1 * step);
    journal << "final t=" << number(engine.time()) << ": " << marking(engine.marking())
            << (engine.isDeadlocked() ? " (deadlock)" : "") << "\n";
    return journal.str();
}

//------------------------------------------------------------------------------
//! \brief GRAFCET: 120 cycles of 0.1 s; every 10 cycles the next input (by
//! name) toggles.
static std::string runGrafcet(std::vector<Net>& nets)
{
    std::ostringstream journal;
    GrafcetEngine engine;
    std::vector<Net*> graphs;
    for (auto& net : nets)
        graphs.push_back(&net);
    engine.graphs(graphs);
    if (!engine.start())
    {
        for (auto const& error : engine.errors())
            journal << "error: " << error << "\n";
        return journal.str();
    }
    std::vector<std::string> inputs;
    for (auto const& it : engine.inputs())
        inputs.push_back(it.first);
    for (int cycle = 0; cycle < 120; ++cycle)
    {
        if (!inputs.empty() && (cycle % 10 == 5))
        {
            std::string const& name = inputs[size_t(cycle / 10) % inputs.size()];
            engine.setInput(name, engine.inputValue(name) == 0 ? 1 : 0);
            journal << "c=" << cycle << " " << name << "=" << engine.inputValue(name) << "\n";
        }
        engine.step(0.1f);
        // Firings of the cycle, with their count (unstable graphs fire
        // in loop until the engine gives up).
        std::map<std::string, size_t> fired;
        for (auto const& firing : engine.firings())
            ++fired[nets[firing.graph].name + ":" + nets[firing.graph].transitions()[firing.transition].key];
        for (auto const& it : fired)
            journal << "c=" << cycle << " " << it.first << (it.second > 1u ? " x" + std::to_string(it.second) : "") << "\n";
    }
    for (auto const& net : nets)
        journal << "final " << net.name << ": " << marking(net.tokens()) << "\n";
    return journal.str();
}

//------------------------------------------------------------------------------
static std::string run(fs::path const& file)
{
    std::vector<Net> nets;
    std::string error;
    if (file.extension() == ".json")
    {
        error = importAllNetsFromJSON(nets, file.string());
    }
    else
    {
        nets.emplace_back();
        bool springify;
        error = loadFromFile(nets.back(), file.string(), springify);
    }
    if (!error.empty())
        return "error: " + error + "\n";
    if (nets.front().type() == TypeOfNet::GRAFCET)
        return runGrafcet(nets);

    std::string journal;
    for (auto const& net : nets)
        journal += "net " + net.name + "\n" + runPetri(net);
    return journal;
}

//------------------------------------------------------------------------------
TEST(TestGolden, Examples)
{
    bool const update = (std::getenv("TPNE_UPDATE_GOLDEN") != nullptr);
    std::vector<fs::path> files;
    for (auto const& entry : fs::recursive_directory_iterator("../examples"))
    {
        auto const ext = entry.path().extension();
        if ((ext == ".json") || (ext == ".teg") || (ext == ".flowshop"))
            files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end());
    ASSERT_GT(files.size(), 20u);

    fs::create_directories("golden");
    for (auto const& file : files)
    {
        std::string const journal = run(file);
        fs::path const golden = fs::path("golden") / (file.filename().string() + ".log");
        if (update || !fs::exists(golden))
        {
            std::ofstream(golden) << journal;
            EXPECT_TRUE(update) << "New golden file " << golden << ": check it and commit it";
            continue;
        }
        std::ifstream in(golden);
        std::string const expected((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        EXPECT_EQ(journal, expected) << "Journal of " << file << " differs from " << golden;
    }
}
