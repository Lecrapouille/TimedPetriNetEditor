// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef PETRI_FILE_EXPORTERS_HPP
#  define PETRI_FILE_EXPORTERS_HPP

#include <string>
#include <vector>

namespace tpne {

class Net;
struct DocumentInfo;

//! \brief JSON is the main format used for saving Petri by this editor.
std::string exportToJSON(Net const& net, std::string const& filename);
//! \brief Export multiple nets to a JSON file (for multi-net documents),
//! with the description of the document and of its GRAFCET inputs and
//! outputs when info is not null.
std::string exportAllNetsToJSON(std::vector<Net> const& nets, std::string const& filename,
                                DocumentInfo const* info = nullptr);
//! \brief Import http://www.cmap.polytechnique.fr/~gaubert/HOWARD2.html
std::string exportToTimedEventGraph(Net const& net, std::string const& filename);
//! \brief
std::string exportToSymfony(Net const& net, std::string const& filename);
//! \brief Import https://gitlab.com/porky11/pn-editor
std::string exportToPNEditor(Net const& net, std::string const& filename);
std::string exportToDrawIO(Net const& net, std::string const& filename);
std::string exportToGraphviz(Net const& net, std::string const& filename);
std::string exportToPetriLaTeX(Net const& net, std::string const& filename);
std::string exportToGrafcetLaTeX(Net const& net, std::string const& filename);
std::string exportToGrafcetCpp(Net const& net, std::string const& filename);
//! \brief The GRAFCETs of a document in one C++ class running them together
//! (cross references, forcing orders), with the rules of the editor; no
//! library needed (micro-controllers). GRAFCET_DEBUG: it publishes its steps
//! for the editor.
std::string exportGrafcetsToCpp(std::vector<Net> const& nets, std::string const& filename);
std::string exportToPNML(Net const& net, std::string const& filename);
//! \brief GRML: the format of GreatSPN (CosyVerif), read by Symmetri.
std::string exportToGRML(Net const& net, std::string const& filename);
//! \brief A vector picture of the net at the positions of the editor.
std::string exportToSVG(Net const& net, std::string const& filename);
//! \brief PNML read by Symmetri: one <initialMarking> per token, the node
//! identifiers are their captions.
std::string exportToSymmetriPNML(Net const& net, std::string const& filename);
//! \brief A C++ program for the runtime: the net built in the code and one
//! function to fill per transition (TPNE_DEBUG: it publishes its journal).
std::string exportToRuntimeCpp(Net const& net, std::string const& filename);

//! \brief Interface for exporting a Petri file.
//! \param[in] net the net we are exporting.
//! \param[in] filepath the path of the file to import.
//! \return a dummy string in case of success, else return the error message.
typedef std::string (*ExportFunc)(Net const&, std::string const&);

//! \brief
struct Exporter
{
    //! \brief Name of the file format (i.e. "JSON")
    std::string format;
    //! \brief file extension with the dot (i.e. ".json")
    std::string extensions;
    //! \brief the pointer function for importing (i.e. importFromJSON)
    ExportFunc exportFct;
    //! \brief For a document of several nets (GRAFCETs running together):
    //! the function exporting them all, else null (the active net only).
    std::string (*exportAllFct)(std::vector<Net> const&, std::string const&) = nullptr;
    //! \brief The nets this format can write (the editor proposes it for
    //! them only), null: all.
    bool (*accepts)(Net const&) = nullptr;
};

//! \brief For Exporter::accepts: GRAFCET only, Petri nets and timed nets only,
//! timed event graphs only.
bool isGrafcet(Net const& net);
bool isNotGrafcet(Net const& net);
bool isTimedEventGraph(Net const& net);

//! \brief Container of file formats we can export the net to (LaTeX, Symfony, Dot ...).
std::vector<Exporter> const& exporters();

//! \brief Add a file format to exporters() (formats of other libraries, as the
//! Julia export of the (max,+) library). Ignored if the format is known.
void addExporter(Exporter const& exporter);

//! \brief The exporter of the extension (".json"), or null.
Exporter const* getExporter(std::string const& extension);

//! \brief The exporter of the file, by the longest of its extensions it
//! ends with ("net.drawio.xml": Draw.io), or null.
Exporter const* findExporter(std::string const& filepath);

} // namespace tpne

#endif
