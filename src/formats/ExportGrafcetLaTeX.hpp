//------------------------------------------------------------------------------
// Depth First Search
static void dfs(Node const& n, std::map<std::string, bool>& visited)
{
    visited[n.key] = true;

    std::cout << n.key << std::endl;

    if (n.arcsOut.size() == 1u) // while ()
    {
        dfs(n.arcsOut[0]->to, visited);
    }
    else
    {
        if (n.type == Node::Type::Place)
        {
            std::cout << "OU {" << std::endl;
        }
        else
        {
            std::cout << "ET {" << std::endl;
        }

        for (auto& c: n.arcsOut)
        {
            if (!visited[c->to.key])
            {
                std::cout << "Branch" << std::endl;
                // while (n.arcsOut.size() == 1u)
                dfs(c->to, visited);
            }
        }

        if (n.type == Node::Type::Place)
        {
            std::cout << "} OU" << std::endl;
        }
        else
        {
            std::cout << "}ET" << std::endl;
        }
    }
}

//------------------------------------------------------------------------------
bool PetriNet::exportToGrafcetLaTeX(std::string const& filename) const
{
    std::map<std::string, bool> visited;
    dfs(m_places[0], visited);

    return false;
}
