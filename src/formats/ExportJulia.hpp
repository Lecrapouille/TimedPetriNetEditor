//------------------------------------------------------------------------------
//! \note PetriNet shall be an event graph and to a canonical form (each places
//! have at most one token and no token at places in inputs or outputs). No
//! checks are performed and shall be done by the external caller.
//------------------------------------------------------------------------------
bool PetriNet::exportToJulia(std::string const& filename) const
{
    // Only Petri net with places having a single input and output arcs are
    // allowed.
    std::vector<Arc*> erroneous_arcs;
    if (!isEventGraph(erroneous_arcs))
        return false;

    // TODO quick test check if we have to do the canonical form, this can avoid
    // duplicating the Petri net
    // if (!cannonical) {

    // Duplicate the Petri net since we potentially modify it to transform it to
    // its canonical form.
    PetriNet canonical(type());
    toCanonicalForm(canonical);

    // TODO
    // } else { return ::exportToJulia(*this, filename); }

    // Open the file
    std::ofstream file(filename);
    if (!file)
    {
        m_message.str("");
        m_message << "Failed to export the Petri net to '" << filename
                  << "'. Reason was " << strerror(errno) << std::endl;
        return false;
    }

    // Generate the Julia header
    file << "# This file has been generated" << std::endl << std::endl;
    file << "using MaxPlus, SparseArrays" << std::endl << std::endl;

    // Count the number of inputs, outputs and states for creating matrices.
    size_t nb_states = 0u;
    size_t nb_inputs = 0u;
    size_t nb_outputs = 0u;

    file << "## Petri Transitions:" << std::endl;

    // Show and count system inputs
    for (auto& t: canonical.transitions())
    {
        if (t.isInput())
        {
            t.index = nb_inputs++;
            file << "# " << t.key << ": input (U"
                 << nb_inputs << ")" << std::endl;
        }
    }

    // Show and count system states
    for (auto& t: canonical.transitions())
    {
        if (t.isState())
        {
            t.index = nb_states++;
            file << "# " << t.key << ": state (X"
                 << nb_states << ")" << std::endl;
        }
    }

    // Show and count system outputs
    for (auto& t: canonical.transitions())
    {
        if (t.isOutput())
        {
            t.index = nb_outputs++;
            file << "# " << t.key << ": output (Y" << nb_outputs
                 << ")" << std::endl;
        }
    }

    // Graph representation. Since an event graph have all its places with a
    // single input arc and and single output arc. We can merge places and its
    // arcs into a single arc (still directing Transitions). Therefore we obtain
    // a graph holding two information: tokens and durations. Since a graph can
    // be represented by an adjacency matrix, here, we genereate two adjacency
    // matrices: one for tokens and one for durations.
    file << std::endl;
    file << "## Timed graph event depict as two graph adjacency matrices:" << std::endl;
    file << "# Nodes are Transitions." << std::endl;
    file << "# Arcs are Places and therefore have tokens and durations" << std::endl;
    SparseMatrix N; SparseMatrix T;
    bool res = canonical.toAdjacencyMatrices(N, T); assert(res == true);
    for (auto& p: canonical.places())
    {
        Transition& from = *reinterpret_cast<Transition*>(&(p.arcsIn[0]->from));
        Transition& to = *reinterpret_cast<Transition*>(&(p.arcsOut[0]->to));

        file << "# Arc " << p.key << ": " << from.key << " -> " << to.key
             << " (Duration: " << p.arcsIn[0]->duration
             << ", Tokens: " << p.tokens << ")" << std::endl;
    }
    size_t const nnodes = canonical.transitions().size();
    file << "N = sparse(" << N << ", " << nnodes << ", " << nnodes << ") # Tokens" << std::endl;
    file << "T = sparse(" << T << ", " << nnodes << ", " << nnodes << ") # Durations" << std::endl;

    // Show the event graph to its Max-Plus counter and dater form
    file << std::endl;
    file << this->showCounterForm().str();
    file << std::endl;
    file << this->showDaterForm().str();

    // Compute the syslin as Julia code using the Max-Plus package
    // X(n) = D X(n) ⨁ A X(n-1) ⨁ B U(n)
    // Y(n) = C X(n)
    SparseMatrix D; SparseMatrix A; SparseMatrix B; SparseMatrix C;
    canonical.toSysLin(D, A, B, C, nb_inputs, nb_states, nb_outputs);

    file << std::endl;
    file << "## Max-Plus implicit linear dynamic system of the dater form:" << std::endl;
    file << "# X(n) = D X(n) ⨁ A X(n-1) ⨁ B U(n)" << std::endl;
    file << "# Y(n) = C X(n)" << std::endl;
    file << "D = sparse(" << D << ", " << nb_states << ", " << nb_states << ") # States without tokens" << std::endl;
    file << "A = sparse(" << A << ", " << nb_states << ", " << nb_states << ") # States with 1 token" << std::endl;
    file << "B = sparse(" << B << ", " << nb_inputs << ", " << nb_inputs << ") # Inputs" << std::endl;
    file << "C = sparse(" << C << ", " << nb_outputs << ", " << nb_outputs << ") # Outputs" << std::endl;
    file << "S = MPSysLin(A, B, C, D)" << std::endl;

    // Semi-Howard
    file << std::endl;
    file << "# TODO" << std::endl;
    file << "l,v = semihoward(S.D, S.A)" << std::endl;

    return true;
}
