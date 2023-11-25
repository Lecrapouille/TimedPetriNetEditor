//=============================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2023 Quentin Quadrat <lecrapouille@gmail.com>
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

#ifndef EXPORTS_HPP
#  define EXPORTS_HPP

#include <string>

namespace tpne {

class Net;

std::string exportToJSON(Net const& net, std::string const& filename);
std::string exportToSymfony(Net const& net, std::string const& filename);
std::string exportToPNEditor(Net const& net, std::string const& filename);
std::string exportToDrawIO(Net const& net, std::string const& filename);
std::string exportToGraphviz(Net const& net, std::string const& filename);
std::string exportToPetriLaTeX(Net const& net, std::string const& filename);
std::string exportToJulia(Net const& net, std::string const& filename);
std::string exportToGrafcetCpp(Net const& net, std::string const& filename);

typedef std::string (*ExportFunc)(Net const&, std::string const&);

struct Exporter
{
    std::string format;
    std::string extensions;
    ExportFunc exportFct;
};

} // namespace tpne

#endif
