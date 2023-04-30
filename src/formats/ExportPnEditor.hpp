//------------------------------------------------------------------------------
//! \brief Write int32_t as little endian
template<class T>
static void write_int32(std::ofstream& file, T const val)
{
    int32_t d = int32_t(val);

    file.put(char(d));
    file.put(char(d >> 8));
    file.put(char(d >> 16));
    file.put(char(d >> 24));
}

//------------------------------------------------------------------------------
template<class T>
static void write_float32(std::ofstream& file, T const val)
{
    float d = float(val);

    file.write(reinterpret_cast<const char*>(&d), sizeof(float));
}

//------------------------------------------------------------------------------
bool PetriNet::exportToPNEditor(std::string const& filename) const
{
    // .pns file: contains the logical contents of the petri net
    {
        std::string filename_pns(filename.substr(0, filename.find_last_of('.')) + ".pns");
        std::ofstream file(filename_pns, std::ios::out | std::ios::binary);
        if (!file)
        {
            m_message.str("");
            m_message << "Failed to export the Petri net to '" << filename_pns
                      << "'. Reason was " << strerror(errno) << std::endl;
            return false;
        }

        // Places
        write_int32(file, m_places.size());
        for (auto const& p: m_places)
        {
            write_int32(file, p.tokens);
        }

        // Transitions
        write_int32(file, m_transitions.size());
        for (auto const& t: m_transitions)
        {
            write_int32(file, t.arcsOut.size());
            for (auto const& a: t.arcsOut)
            {
                write_int32(file, a->to.id);
            }
            write_int32(file, t.arcsIn.size());
            for (auto const& a: t.arcsIn)
            {
                write_int32(file, a->from.id);
            }
        }
    }

    // .pnl file: describes the layout of the petri net
    {
        std::string filename_pnl(filename.substr(0, filename.find_last_of('.')) + ".pnl");
        std::ofstream file(filename_pnl, std::ios::out | std::ios::binary);
        if (!file)
        {
            m_message.str("");
            m_message << "Failed to export the Petri net to '" << filename_pnl
                      << "'. Reason was " << strerror(errno) << std::endl;
            return false;
        }

        for (auto const& t: m_transitions)
        {
            write_float32(file, t.x);
            write_float32(file, t.y);
        }

        for (auto const& p: m_places)
        {
            write_float32(file, p.x);
            write_float32(file, p.y);
        }
    }

    // .pnkp: list of names for all the transitions
    {
        std::string filename_pnkp(filename.substr(0, filename.find_last_of('.')) + ".pnkp");
        std::ofstream file(filename_pnkp);
        if (!file)
        {
            m_message.str("");
            m_message << "Failed to export the Petri net to '" << filename_pnkp
                      << "'. Reason was " << strerror(errno) << std::endl;
            return false;
        }

        for (auto const& p: m_places)
        {
            file << p.caption << std::endl;
        }
    }

    // .pnk: list of names for all the places
    {
        std::string filename_pnk(filename.substr(0, filename.find_last_of('.')) + ".pnk");
        std::ofstream file(filename_pnk);
        if (!file)
        {
            m_message.str("");
            m_message << "Failed to export the Petri net to '" << filename_pnk
                      << "'. Reason was " << strerror(errno) << std::endl;
            return false;
        }

        for (auto const& t: m_transitions)
        {
            file << t.caption << std::endl;
        }
    }

    return true;
}
