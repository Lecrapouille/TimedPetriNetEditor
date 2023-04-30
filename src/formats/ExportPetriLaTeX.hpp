//------------------------------------------------------------------------------
bool PetriNet::exportToPetriLaTeX(std::string const& filename,
                                  float const scale_x, float const scale_y) const
{
    std::ofstream file(filename);
    if (!file)
    {
        m_message.str("");
        m_message << "Failed to export the Petri net to '" << filename
                  << "'. Reason was " << strerror(errno) << std::endl;
        return false;
    }

    file << R"PN(\documentclass[border = 0.2cm]{standalone}
\usepackage{tikz}
\usetikzlibrary{petri,positioning}
\begin{document}
\begin{tikzpicture}
)PN";

    // Places
    file << std::endl << "% Places" << std::endl;
    for (auto const& p: m_places)
    {
        file << "\\node[place, "
             << "label=above:$" << p.caption << "$, "
             << "fill=blue!25, "
             << "draw=blue!75, "
             << "tokens=" << p.tokens << "] "
             << "(" << p.key << ") at (" << int(p.x * scale_x)
             << ", " << int(-p.y * scale_y) << ") {};"
             << std::endl;
    }

    // Transitions
    file << std::endl << "% Transitions" << std::endl;
    for (auto const& t: m_transitions)
    {
        std::string color = (t.canFire() ? "green" : "red");

        file << "\\node[transition, "
             << "label=above:$" << t.caption << "$, "
             << "fill=" << color << "!25, "
             << "draw=" << color << "!75] "
             << "(" << t.key << ") at (" << int(t.x * scale_x)
             << ", " << int(-t.y * scale_y) << ") {};"
             << std::endl;
    }

    // Arcs
    file << std::endl << "% Arcs" << std::endl;
    for (auto const& a: m_arcs)
    {
        if (a.from.type == Node::Type::Transition)
        {
            std::stringstream duration;
            duration << std::fixed << std::setprecision(2) << a.duration;
            file << "\\draw[-latex, thick] "
                 << "(" << a.from.key << ") -- "
                 << "node[midway, above right] "
                 << "{" << duration.str() << "} "
                 << "(" << a.to.key << ");"
                 << std::endl;
        }
        else
        {
            file << "\\draw[-latex, thick] "
                 << "(" << a.from.key << ") -- " << "(" << a.to.key << ");"
                 << std::endl;
        }
    }

    file << R"PN(
\end{tikzpicture}
\end{document}
)PN";

    return true;
}
