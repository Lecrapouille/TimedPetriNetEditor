//------------------------------------------------------------------------------
bool PetriNet::exportToJSON(std::string const& filename) const
{
    std::string separator;
    std::ofstream file(filename);

    if (isEmpty())
    {
        m_message.str("");
        m_message << "I'll not save empty net" << std::endl;
        return false;
    }

    if (!file)
    {
        m_message.str("");
        m_message << "Failed saving the Petri net in '" << filename
                  << "'. Reason was " << strerror(errno) << std::endl;
        return false;
    }

    file << "{\n  \"places\": [";
    for (auto const& p: m_places)
    {
        file << separator << "\n    " << '\"' << p.key << ',' << p.caption << ','
             << p.x << ',' << p.y << ',' << p.tokens << '\"';
        separator = ",";
    }
    file << "],\n  \"transitions\": [";
    separator = "";
    for (auto const& t: m_transitions)
    {
        file << separator << "\n    " << '\"' << t.key << ',' << t.caption << ','
             << t.x << ',' << t.y << ',' << t.angle << '\"';
        separator = ",";
    }
    file << "],\n  \"arcs\": [";
    separator = "";
    for (auto const& a: m_arcs)
    {
        file << separator << "\n    " << '\"' << a.from.key << ','
             << a.to.key << ',' << a.duration << '\"';
        separator = ",";
    }
    file << "]\n}";

    return true;
}
