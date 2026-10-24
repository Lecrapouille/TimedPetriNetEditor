// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// Non regression of the GRAFCET code generator: the generated code shall
// behave as the GRAFCET engine of the editor. For a document of GRAFCETs:
//   GrafcetCodegen <doc.json> <out.hpp> <engine.txt> <driver.cpp>
// writes the generated code, the trace of the engine (steps and outputs at
// each cycle, inputs toggled one after the other) and a driver printing the
// same trace with the generated code. run.sh compiles the driver and compares.

#include "IO/IO.hpp"
#include "PetriNet/GrafcetEngine.hpp"
#include "PetriNet/PetriNet.hpp"

#include <fstream>
#include <iostream>
#include <map>

using namespace tpne;

static constexpr int CYCLES = 300;

int main(int argc, char* argv[])
{
    if (argc != 5)
    {
        std::cerr << "Usage: " << argv[0] << " doc.json out.hpp engine.txt driver.cpp" << std::endl;
        return 2;
    }
    std::vector<Net> nets;
    std::string error = importAllNetsFromJSON(nets, argv[1]);
    if (error.empty())
        error = exportGrafcetsToCpp(nets, argv[2]);
    if (!error.empty())
    {
        std::cerr << error << std::endl;
        return 1;
    }

    // The engine.
    GrafcetEngine engine;
    std::vector<Net*> graphs;
    for (auto& net : nets)
        graphs.push_back(&net);
    engine.graphs(graphs);
    if (!engine.start())
    {
        std::cerr << "The engine does not start" << std::endl;
        return 1;
    }
    std::vector<std::string> inputs;
    for (auto const& it : engine.inputs())
        inputs.push_back(it.first);
    std::ofstream trace(argv[3]);
    for (int cycle = 0; cycle < CYCLES; ++cycle)
    {
        if (!inputs.empty() && (cycle % 7 == 3))
        {
            std::string const& name = inputs[size_t(cycle / 7) % inputs.size()];
            engine.setInput(name, engine.inputValue(name) == 0 ? 1 : 0);
        }
        engine.step(0.1f);
        trace << cycle << " ";
        for (auto const& net : nets)
            for (auto const& p : net.places())
                trace << (p.tokens ? '1' : '0');
        trace << " ";
        for (auto const& o : engine.outputs())
            trace << (o.second ? '1' : '0');
        trace << "\n";
    }

    // Members of the outputs, read in the generated struct Outputs.
    std::map<std::string, std::string> outputs;
    std::string space;
    {
        std::ifstream header(argv[2]);
        std::string line;
        bool in = false;
        while (std::getline(header, line))
        {
            if (space.empty() && (line.rfind("namespace ", 0) == 0))
                space = line.substr(10u, line.find(' ', 10u) - 10u);
            if (line == "struct Outputs")
                in = true;
            else if (in && (line == "};"))
                break;
            else if (in && (line.find("bool ") != std::string::npos))
            {
                size_t const a = line.find("bool ") + 5u;
                std::string const id = line.substr(a, line.find(" =", a) - a);
                size_t const c = line.find("// ");
                outputs[(c == std::string::npos) ? id : line.substr(c + 3u)] = id;
            }
        }
    }

    // The driver: the same scenario on the generated code.
    std::ofstream d(argv[4]);
    d << "#include \"" << argv[2] << "\"\n#include <cstdio>\n\nint main()\n{\n"
      << "    " << space << "::Grafcet g;\n"
      << "    for (int cycle = 0; cycle < " << CYCLES << "; ++cycle)\n    {\n";
    if (!inputs.empty())
    {
        d << "        if (cycle % 7 == 3)\n        {\n            switch ((cycle / 7) % " << inputs.size() << ")\n            {\n";
        for (size_t i = 0u; i < inputs.size(); ++i)
            d << "            case " << i << ": g.in." << inputs[i] << " = (g.in." << inputs[i] << " == 0) ? 1 : 0; break;\n";
        d << "            }\n        }\n";
    }
    d << "        g.cycle(0.1f);\n        std::printf(\"%d \", cycle);\n";
    for (size_t g = 0u; g < nets.size(); ++g)
        d << "        for (int p = 0; p < " << nets[g].places().size() << "; ++p) std::printf(\"%c\", g.X" << g << "[p] ? '1' : '0');\n";
    d << "        std::printf(\" \");\n";
    for (auto const& o : engine.outputs())
        d << "        std::printf(\"%c\", g.out." << outputs[o.first] << " ? '1' : '0');\n";
    d << "        std::printf(\"\\n\");\n    }\n}\n";
    return 0;
}
