//------------------------------------------------------------------------------
//! \note this a quick and dirty but simple JSON parsing since I do not want to
//! depend on a huge library.
bool PetriNet::importFromJSON(std::string const& filename)
{
    std::vector<std::string> words(5);

    Splitter s(filename);
    if (!s)
    {
        m_message.str("");
        m_message << "Failed opening '" << filename << "'. Reason was '"
                  << strerror(errno) << "'" << std::endl;
        return false;
    }

    // Expect '{' as first token
    if (s.split(" \t\n", " \t\n") != "{")
    {
        m_message.str("");
        m_message << "Failed loading " << filename
                  << ". Token { missing. Bad JSON file" << std::endl;
        return false;
    }

    clear();
    while (s)
    {
        // Split for tokens "places : [" or "transitions : [" or "arcs : ["
        std::string token(s.split(" \t\n\"", " \t\n\""));
        if ((token == "places") || (token == "transitions") || (token == "arcs"))
        {
            // Split and check the presence of tokens ": ["
            if ((s.split(" \t\n\"", " \t\n")[0] != ':') || (s.split(" \t\n", " ]\t\n\"")[0] != '['))
            {
                m_message.str("");
                m_message << "Failed parsing" << std::endl;
                return false;
            }

            // Parse Petri place "P0,Caption,146,250,1"
            if (token == "places")
            {
                while (s.split(" \t\n\"[", "\"")[0] != ']')
                {
                    if (s.str()[0] == ',')
                        continue ;
                    if (token2vector(s.str(), words) != 5u)
                    {
                        m_message.str("");
                        m_message << "Failed parsing Place" << std::endl;
                        return false;
                    }
                    addPlace(convert_to<size_t>(&words[0][1]),  // id
                             words[1],                      // caption
                             convert_to<float>(words[2]),   // x
                             convert_to<float>(words[3]),   // y
                             convert_to<size_t>(words[4])); // tokens
                }
            }
            // Parse Petri transition "T0,Caption,272,173,315"
            else if (token == "transitions")
            {
                while (s.split(" \t\n\"[", "\"")[0] != ']')
                {
                    if (s.str()[0] == ',')
                        continue ;
                    if (token2vector(s.str(), words) != 5u)
                    {
                        m_message.str("");
                        m_message << "Failed parsing Transition" << std::endl;
                        return false;
                    }
                    addTransition(convert_to<size_t>(&words[0][1]),  // id
                                  words[1],                      // caption
                                  convert_to<float>(words[2]),   // x
                                  convert_to<float>(words[3]),   // y
                                  convert_to<int>(words[4]));    // angle
                }
            }
            // Parse Petri arcs "P0,T0,nan"
            else
            {
                while (s.split(" \t\n\"[", "\"}")[0] != ']')
                {
                    if (s.str()[0] == ',')
                        continue ;
                    if (token2vector(s.str(), words) != 3u)
                    {
                        m_message.str("");
                        m_message << "Failed parsing Arc" << std::endl;
                        return false;
                    }
                    Node* from = findNode(words[0]);
                    if (!from)
                    {
                        m_message << "Failed loading " << filename
                                  << ". Origin node " << words[0]
                                  << " not found" << std::endl;
                        return false;
                    }

                    Node* to = findNode(words[1]);
                    if (!to)
                    {
                        m_message << "Failed loading " << filename
                                  << ". Destination node " << words[1]
                                  << " not found" << std::endl;
                        return false;
                    }

                    float duration = convert_to<float>(words[2]);
                    if (duration < 0.0f)
                    {
                        m_message.str("");
                        m_message << "Failed loading " << filename
                                  << ". Duration " << words[2]
                                  << " shall be > 0" << std::endl;
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
            }
        }
        else if (s.str() == "}")
        {
            // End of the file
            return true;
        }
        else if (token != "")
        {
            m_message.str("");
            m_message << "Failed loading " << filename
                      << ". Key " << s.str() << " is not a valid token"
                      << std::endl;
            return false;
        }
    }

    return true;
}
