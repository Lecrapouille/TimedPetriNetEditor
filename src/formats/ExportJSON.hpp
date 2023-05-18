//------------------------------------------------------------------------------
bool PetriNet::exportToJSON(std::string const& filename) const
{
    std::string separator("\n");

    //if (isEmpty())
    //{
    //    m_message.str("");
    //    m_message << "I'll not save empty net" << std::endl;
    //    return false;
    //}

    std::ofstream file(filename);
    if (!file)
    {
        m_message.str("");
        m_message << "Failed saving the Petri net in '" << filename
                  << "'. Reason was " << strerror(errno) << std::endl;
        return false;
    }

    // TODO sensors

    file << "{" << std::endl;
    file << "  \"revision\": 2," << std::endl;
    file << "  \"nets\": [\n    {" << std::endl;
    file << "       \"name\": \"" << name() << "\"," << std::endl;
    file << "       \"type\": \"" << to_str(m_type) << "\"," << std::endl;

    // Places
    file << "       \"places\": [";
    for (auto const& p: m_places)
    {
        file << separator; separator = ",\n";
        file << "            { \"id\": " << p.id << ", \"caption\": \"" << p.caption
             << "\", \"tokens\": " << p.tokens << ", \"x\": " << p.x
             << ", \"y\": " << p.y << " }";
    }

    // Transitions
    separator = "\n";
    file << "\n       ],\n       \"transitions\": [";
    for (auto const& t: m_transitions)
    {
        file << separator; separator = ",\n";
        file << "            { \"id\": " << t.id << ", \"caption\": \"" << t.caption << "\", \"x\": "
             << t.x << ", \"y\": " << t.y << ", \"angle\": " << t.angle << " }";
    }

    // Arcs
    separator = "\n";
    file << "\n       ],\n       \"arcs\": [";
    for (auto const& a: m_arcs)
    {
        file << separator; separator = ",\n";
        file << "            { \"from\": \"" << a.from.key << "\", " << "\"to\": \"" << a.to.key
             << "\"";
        if (a.from.type == Node::Type::Transition)
            file << ", \"duration\": " << a.duration;
        file << " }";
    }
    file << "\n       ]" << std::endl;
    file << "    }" << std::endl;
    file << "  ]" << std::endl;
    file << "}" << std::endl;

    return true;
}
