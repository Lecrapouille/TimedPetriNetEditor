//------------------------------------------------------------------------------
bool PetriNet::exportToGraphviz(std::string const& filename) const
{
    // Update arcs in/out for all transitions to be sure to generate the correct
    // net.
    //generateArcsInArcsOut();

    std::ofstream file(filename);
    if (!file)
    {
        m_message.str("");
        m_message << "Failed to export the Petri net to '" << filename
                  << "'. Reason was " << strerror(errno) << std::endl;
        return false;
    }

    file << "digraph G {" << std::endl;

    // Places
    file << "node [shape=circle, color=blue]" << std::endl;
    for (auto const& p: m_places)
    {
        file << "  " << p.key << " [label=\"" << p.caption;
        if (p.tokens > 0u)
        {
            file << "\\n" << p.tokens << "&bull;";
        }
        file << "\"];" << std::endl;
    }

    // Transitions
    file << "node [shape=box, color=red]" << std::endl;
    for (auto const& t: m_transitions)
    {
        if (t.canFire())
        {
            file << "  " << t.key << " [label=\""
                 << t.caption << "\", color=green];"
                 << std::endl;
        }
        else
        {
            file << "  " << t.key << " [label=\""
                 << t.caption << "\"];"
                 << std::endl;
        }
    }

    // Arcs
    file << "edge [style=\"\"]" << std::endl;
    for (auto const& a: m_arcs)
    {
        file << "  " << a.from.key << " -> " << a.to.key;
        if (a.from.type == Node::Type::Transition)
        {
            file << " [label=\"" << a.duration << "\"]";
        }
        file << ";" << std::endl;
    }

    file << "}" << std::endl;
    return true;
}
