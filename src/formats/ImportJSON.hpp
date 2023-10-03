//------------------------------------------------------------------------------
bool PetriNet::importFromJSON(std::string const& filename)
{
    // Check if file exists
    std::ifstream file(filename);
    if (!file)
    {
        m_message.str("");
        m_message << "Failed opening '" << filename << "'. Reason was '"
                  << strerror(errno) << "'" << std::endl;
        return false;
    }

    // Load the JSON content into dictionaries
    nlohmann::json json;
    try
    {
        file >> json;
    }
    catch (std::exception const& e)
    {
        m_message.str("");
        m_message << "Failed parsing '" << filename << "'. Reason was '"
                  << e.what() << "'" << std::endl;
        return false;
    }

    std::vector<std::string> words;

    nlohmann::json const& net = json["nets"][0];
    m_name = std::string(net["name"]);
    std::string type = std::string(net["type"]);
    if (type == "GRAFCET")
        m_type = PetriNet::Type::GRAFCET;
    else if (type == "Petri net")
        m_type = PetriNet::Type::Petri;
    else if (type == "Timed Petri net")
        m_type = PetriNet::Type::TimedPetri;
    else if (type == "Timed event graph")
        m_type = PetriNet::Type::TimedEventGraph;
    else
    {
        m_message.str("");
        m_message << "Failed parsing '" << filename << "'. Reason was '"
                  << "Unknown type of net: " << type << "'" << std::endl;
        return false;
    }

    // Places
    for (nlohmann::json const& p : net["places"])
        addPlace(p["id"], p["caption"], p["x"], p["y"], p["tokens"]);

    // Transitions
    for (nlohmann::json const& t : net["transitions"])
        addTransition(t["id"], t["caption"], t["x"], t["y"], t["angle"]);

    // Arcs
    for (nlohmann::json const& a : net["arcs"])
    {
        Node* from = findNode(a["from"]);
        Node* to = findNode(a["to"]);
        if ((from == nullptr) || (to == nullptr))
        {
            m_message.str("");
            m_message << "Failed parsing '" << filename << "'. Reason was 'Arc "
                      << words[0] << " -> " << words[1] << " refer to unknown nodes'"
                      << std::endl;
            return false;
        }

        float duration = NAN;
        auto const& it = a.find("duration");
        if (it != a.end())
        {
            duration = *it;
            if (duration < 0.0f)
            {
                m_message.str("");
                m_message << "Failed parsing '" << filename << "'. Reason was 'Arc "
                        << words[0] << " -> " << words[1] << " has negative duration'"
                        << std::endl;
                return false;
            }
        }
        if (!addArc(*from, *to, duration))
        {
            m_message.str("");
            m_message << "Failed loading " << filename
                      << ". Arc " << from->key << " -> " << to->key
                      << " is badly formed" << std::endl;
            return false;
        }
    }

    return true;
}
