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

    // Places
    for (nlohmann::json const& p : json["places"])
    {
        token2vector(p, words);
        if (words[0][0] != 'P')
        {
            m_message.str("");
            m_message << "Failed parsing '" << filename << "'. Reason was '"
                      << words[0] << " is not a place'" << std::endl;
            return false;
        }
        if (words.size() != 5u)
        {
            m_message.str("");
            m_message << "Failed parsing '" << filename << "'. Reason was '"
                      << p << " shall have 5 fields'" << std::endl;
            return false;
        }
        addPlace(convert_to<size_t>(&words[0][1]),  // id
                 words[1],                          // caption
                 convert_to<float>(words[2]),       // x
                 convert_to<float>(words[3]),       // y
                 convert_to<size_t>(words[4]));     // tokens
    }

    // Transitions
    for (nlohmann::json const& t : json["transitions"])
    {
        token2vector(t, words);
        if (words[0][0] != 'T')
        {
            m_message.str("");
            m_message << "Failed parsing '" << filename << "'. Reason was '"
                      << words[0] << " is not a transition'" << std::endl;
            return false;
        }
        if (words.size() != 5u)
        {
            m_message.str("");
            m_message << "Failed parsing '" << filename << "'. Reason was '"
                      << t << " shall have 5 fields'" << std::endl;
            return false;
        }
        addTransition(convert_to<size_t>(&words[0][1]),  // id
                      words[1],                          // caption
                      convert_to<float>(words[2]),       // x
                      convert_to<float>(words[3]),       // y
                      convert_to<int>(words[4]));        // angle
    }

    // Arcs
    for (nlohmann::json const& a : json["arcs"])
    {
        token2vector(a, words);
        if (words.size() != 3u)
        {
            m_message.str("");
            m_message << "Failed parsing '" << filename << "'. Reason was '"
                      << a << " shall have 3 fields'" << std::endl;
            return false;
        }
        Node* from = findNode(words[0]);
        Node* to = findNode(words[1]);
        if ((from == nullptr) || (to == nullptr))
        {
            m_message.str("");
            m_message << "Failed parsing '" << filename << "'. Reason was 'Arc "
                      << words[0] << " -> " << words[1] << " refer to unknown nodes'"
                      << std::endl;
            return false;
        }

        float duration = convert_to<float>(words[2]);
        if (duration < 0.0f)
        {
            m_message.str("");
            m_message << "Failed parsing '" << filename << "'. Reason was 'Arc "
                      << words[0] << " -> " << words[1] << " has negative duration'"
                      << std::endl;
            return false;
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