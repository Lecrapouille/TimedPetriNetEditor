//------------------------------------------------------------------------------
// See http://jpquadrat.free.fr/chine.pdf flowshop_graph() function
// TODO ./build/TimedPetriEditor foo.flowshop
bool PetriNet::importFlowshop(std::string const& filename)
{
    static_assert(std::numeric_limits<double>::is_iec559, "IEEE 754 required");

    std::ifstream ifs{ filename };
    if (!ifs)
    {
        std::cerr << "Could not open matrix file '"
                  << filename << "' for reading"
                  << std::endl;
        return false;
    }

    // Dense matrix
    std::vector<std::vector<double>> matrix;
    size_t rows, columns;

    // Read the number of rows and columns and resize the vector of data
    if (!(ifs >> rows >> columns))
    {
        m_message.str("");
        m_message << "Malformed matrix dimension. Needed rows columns information"
                  << std::endl;
        return false;
    }

    // Read all data and store them into the matrix
    matrix.resize(rows, std::vector<double>(columns));
    for (std::vector<double>& row : matrix)
    {
        for (double& col : row)
        {
            std::string text;
            ifs >> text;
            col = std::stod(text.c_str());
            std::cout << ' ' << col;
        }
        std::cout << std::endl;
    }

    // Construct the flowshop
    float x, y;
    const size_t machines = rows;
    const size_t pieces = columns;
    const float SPACING = 100.0f;
    size_t id = 0u; // Place unique identifier
    size_t m, p; // iterators
    std::vector<Place*> places;

    // Add places of the matrix
    x = 2.0f * SPACING; y = SPACING - 50.0f;
    for (m = 0u; m < machines; ++m) // TODO inverser l'ordre
    {
        x = 2.0f * SPACING;
        for (p = 0u; p < pieces; ++p)
        {
            if (matrix[m][p] != -std::numeric_limits<double>::infinity())
            {
                // Place caption "m1p2" for "Machine1--Piece2"
                //std::string caption("m" + std::to_string(m) + "p" + std::to_string(p));
                std::string caption(std::to_string(id) + ": " + std::to_string(m * pieces + p));
                places.push_back(&addPlace(id++, caption, x, y, 0u));
            }
            x += SPACING;
        }
        y += SPACING;
    }

    // Link arcs between places: this will add the transitions
    for (m = 0u; m < 2u/*machines - 1u*/; ++m)
    {
        for (p = 0u; p < pieces - 1u; ++p)
        {
            size_t next = p + 1u;
            while ((next < pieces - 1u) && (matrix[m][next] == -std::numeric_limits<double>::infinity()))
            {
                next += 1u;
            }

            Node* from = findNode(places[m * pieces + p]->key);
            Node* to = findNode(places[m * pieces + next]->key);
            std::cout << "M" << m << ": " <<
            addArc(*from, *to, float(matrix[m][p]), false);
        }
    }

    // Construct the flowshop: Place the machines (inputs)
    x = SPACING; y = SPACING;
    for (size_t i = 0u; i < machines; ++i)
    {
        addPlace(p++, "Machine " + std::to_string(i), x, y, 0u); // FIXME id
        y += SPACING;
    }

    // Construct the flowshop: Place the pieces (inputs)
    x += SPACING / 2.0f;
    for (size_t i = 0u; i < pieces; ++i)
    {
        addPlace(p++, "Piece " + std::to_string(i), x, y, 0u); // FIXME id
        x += SPACING;
    }

    return true;
}
