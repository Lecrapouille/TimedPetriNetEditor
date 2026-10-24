// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// The producer and the consumers run by the runtime, then restarted after a
// crash. The journal is written line by line in a file (Options::journal) as
// the instance runs. The program "crashes" while a consumer uses a part: the
// journal has the Scheduled and Started of this Consume, not its Completed.
// A second run reads the journal, rebuilds the instance with restore() (the
// marking is replayed from the initial tokens) and calls Consume again
// (Recovery::Restart: the function shall be idempotent), then finishes the
// work. No part is lost, none is used twice.
//   ./ProducerConsumer [ProducerConsumer.json]

#include "IO/IO.hpp"
#include "Runtime/Journal.hpp"
#include "Runtime/Runtime.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

using namespace tpne;

//! \brief The journal of the instance, kept on the disk.
static char const* const JOURNAL = "build/examples/ProducerConsumer.jsonl";
//! \brief Parts the producer makes.
static constexpr int PARTS = 6;

// *****************************************************************************
//! \brief The program: its functions, counting the parts made and used.
// *****************************************************************************
struct Workshop
{
    //! \brief Produce fired (the guard), parts made and used.
    int asked = 0;
    std::atomic<int> made{ 0 };
    std::atomic<int> used{ 0 };
    //! \brief The program is dead: its functions do nothing more.
    std::atomic<bool> crashed{ false };
    std::mutex print;

    //! \brief The functions of the transitions on a pool of threads, the
    //! journal appended to the file as it is written.
    std::unique_ptr<Runtime> start(Net const& net, std::ofstream& journal)
    {
        Runtime::Options options;
        options.journal = [this, &net, &journal](LogEntry const& entry) {
            journal << toJsonLine(entry, net.colors) << std::endl; // flushed: it survives a crash
            count(entry);
        };
        auto runtime = std::make_unique<Runtime>(net, std::make_shared<ThreadPoolExecutor>(3u), options);
        // Two parts can be made at once (two tokens in the loop of the
        // producer): the guard counts the firings, not the parts made.
        runtime->guard("Produce", [this](Context const&) { return asked < PARTS; });
        runtime->on("Produce", [this]() { work(20, made, "producer: part %d made"); });
        runtime->on("Consume", [this]() { work(50, used, "consumer: part %d used"); });
        return runtime;
    }

    //! \brief A function: some work, then a part more.
    void work(int const ms, std::atomic<int>& parts, char const* what)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        std::lock_guard<std::mutex> lock(print);
        if (crashed)
            return;
        std::printf("  ");
        std::printf(what, ++parts);
        std::printf("\n");
    }

    //! \brief Counters of the program, from the lines of its journal (they
    //! are written by the thread of step()).
    void count(LogEntry const& entry)
    {
        bool const produce = (entry.transition == "Produce");
        if ((entry.event == LogEntry::Event::Scheduled) && produce)
            ++asked;
    }

    //! \brief The same from the journal of a previous run: the Produce fired,
    //! the functions that ended.
    void recount(std::vector<LogEntry> const& entries)
    {
        for (auto const& entry : entries)
        {
            count(entry);
            if (entry.event != LogEntry::Event::Completed)
                continue;
            made += (entry.transition == "Produce") ? 1 : 0;
            used += (entry.transition == "Consume") ? 1 : 0;
        }
    }
};

//------------------------------------------------------------------------------
//! \brief Index of the place of this caption.
static size_t place(Net const& net, std::string const& caption)
{
    for (auto const& p : net.places())
    {
        if (p.caption == caption)
            return p.id;
    }
    return size_t(-1);
}

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    char const* file = (argc > 1) ? argv[1] : "examples/Petri-Nets/ProducerConsumer/ProducerConsumer.json";
    Net net;
    std::string const error = loadFromFile(net, file);
    if (!error.empty())
    {
        std::cerr << error << std::endl;
        return EXIT_FAILURE;
    }
    size_t const buffer = place(net, "Buffer");
    size_t const accepted = place(net, "Accepted");
    size_t const ready = place(net, "Ready");

    // First run: it crashes while the 4th part is used.
    {
        std::cout << "First run" << std::endl;
        std::ofstream journal(JOURNAL);
        Workshop workshop;
        auto runtime = workshop.start(net, journal);
        auto line = runtime->instantiate("line");
        int started = 0;
        while (runtime->step(*line) && (started < 4))
        {
            started = 0;
            for (auto const& entry : line->log())
                started += ((entry.transition == "Consume") && (entry.event == LogEntry::Event::Started)) ? 1 : 0;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        // The program dies here: no more step(), the end of the running
        // functions is never written in the journal.
        workshop.crashed = true;
        std::printf("  crash! (while the parts %d to %d are used)\n", workshop.used.load() + 1, started);
    }

    // Second run: the journal rebuilds the instance and the counters.
    std::cout << "Second run" << std::endl;
    std::ifstream saved(JOURNAL);
    std::stringstream text;
    text << saved.rdbuf();
    std::vector<LogEntry> entries;
    std::string const bad = fromJsonLines(text.str(), net.colors, entries);
    if (!bad.empty())
    {
        std::cerr << bad << std::endl;
        return EXIT_FAILURE;
    }
    std::ofstream journal(JOURNAL, std::ios::app);
    Workshop workshop;
    workshop.recount(entries);
    std::printf("  journal: %zu lines, %d parts made, %d used\n", entries.size(), workshop.made.load(),
                workshop.used.load());
    auto runtime = workshop.start(net, journal);
    auto line = runtime->restore("line", entries, Runtime::Recovery::Restart);
    // Consume started and not completed: called again.
    int running = 0;
    for (auto const& entry : entries)
    {
        if (entry.transition == "Consume")
            running += (entry.event == LogEntry::Event::Started) ? 1 : (entry.event == LogEntry::Event::Completed) ? -1 : 0;
    }
    std::printf("  restored: Buffer %zu, Accepted %zu, Ready %zu; Consume running at the crash: %d, called again\n",
                line->marking().count(buffer), line->marking().count(accepted), line->marking().count(ready),
                running);

    // The work is done when the parts are made and the consumers are free.
    auto busy = [&]() {
        return (workshop.made < PARTS) || (line->marking().count(buffer) > 0u) ||
               (line->marking().count(ready) < net.places()[ready].tokens);
    };
    while (runtime->step(*line) && busy())
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    runtime->cancel(*line);
    runtime->run(*line);

    // 3 parts in Buffer at the start, 1 ready to be sent, PARTS made.
    int const expected = int(net.places()[buffer].tokens) + 1 + PARTS;
    std::printf("Done: %d parts made, %d used (expected %d)\n", workshop.made.load(), workshop.used.load(), expected);
    return ((workshop.made == PARTS) && (workshop.used == expected)) ? EXIT_SUCCESS : EXIT_FAILURE;
}
