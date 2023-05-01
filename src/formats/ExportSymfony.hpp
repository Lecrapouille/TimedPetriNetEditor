//------------------------------------------------------------------------------
bool PetriNet::exportToSymfony(std::string const& filename) const
{
    std::ofstream file(filename);
    if (!file)
    {
        m_message.str("");
        m_message << "Failed to export the Petri net to '" << filename
                  << "'. Reason was " << strerror(errno) << std::endl;
        return false;
    }

    file << R"PN(framework:
    workflows:
)PN";
    file << "        " << m_name << ":";
    file << R"PN(
            type: 'workflow'
            audit_trail:
                enabled: true
            marking_store:
                type: 'method'
                property: 'currentPlace'
            initial_marking:
)PN";

    // Initial places
    for (auto const& p: m_places)
    {
        if (p.tokens > 0u)
        {
            file << "                - " << p.caption << std::endl;
        }
    }

    // Places
    file << "            places:" << std::endl;
    for (auto const& p: m_places)
    {
        file << "                - " << p.caption << std::endl;
    }

    // Transitions
    file << "            transitions:" << std::endl;
    for (auto const& t: m_transitions)
    {
        // From
        file << "                " << t.caption << ":" << std::endl;
        file << "                    from:" << std::endl;

        for (auto const& it: t.arcsIn)
        {
            file << "                        - " << it->from.caption << std::endl;
        }


        // To
        file << "                    to:" << std::endl;
        for (auto const& it: t.arcsOut)
        {
            file << "                        - " << it->to.caption << std::endl;
        }
    }
    return true;
}
