/* *****************************************************************************
** MIT License
**
** Copyright (c) 2022 Quentin Quadrat
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE.
********************************************************************************
*/

#ifndef FORCEDIRECTEDGRAPH_HPP
#  define FORCEDIRECTEDGRAPH_HPP

#  include "TimedPetriNetEditor/PetriNet.hpp"

#  include <vector>
#  include <cstdlib>
#  include <cmath>
#  include <map>

namespace tpne {

// *****************************************************************************
//! \brief Simple 2D vector without ImGui dependency.
// *****************************************************************************
struct Vec2
{
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}

    Vec2 operator+(Vec2 const& other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(Vec2 const& other) const { return {x - other.x, y - other.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2& operator+=(Vec2 const& other) { x += other.x; y += other.y; return *this; }
    Vec2& operator-=(Vec2 const& other) { x -= other.x; y -= other.y; return *this; }
};

// *****************************************************************************
//! \brief Force-directed graph drawing algorithms are a class of algorithms for
//! drawing graphs in an aesthetically-pleasing way. Their purpose is to
//! position the nodes of a graph in two-dimensional or three-dimensional space
//! so that all the edges are of more or less equal length and there are as few
//! crossing edges as possible, by assigning forces among the set of edges and
//! the set of nodes, based on their relative positions, and then using these
//! forces either to simulate the motion of the edges and nodes or to minimize
//! their energy.
//!
//! Use the spring/repulsion model of Fruchterman and Reingold (1991) with:
//! - Attractive force: af(d) = d^2 / k
//! - Repulsive force: rf(d) = -k^2 / d
//! where d is distance between two vertices and the optimal distance between
//! vertices k is defined as C * sqrt(area / num_vertices) where C is a
//! parameter we can adjust.
//!
//! For more information see this video https://youtu.be/WWm-g2nLHds
//! This code source is largely inspired by:
//! https://github.com/qdHe/Parallelized-Force-directed-Graph-Drawing
// *****************************************************************************
class ForceDirected
{
public:

    // *************************************************************************
    //! \brief Vertex is a 2D representation of a graph node.
    // *************************************************************************
    struct Vertex
    {
        explicit Vertex(Transition& tr) : node(&tr) {}
        explicit Vertex(Place& p) : node(&p) {}

        //! \brief Place or transition. Need to access to position.
        Node* node = nullptr;
        //! \brief Displacement due to attractive and repulsive forces.
        Vec2 displacement;
        //! \brief List of neighboring nodes.
        std::vector<Node*> neighbors;
    };

    using Vertices = std::vector<ForceDirected::Vertex>;

public:

    //----------------------------------------------------------------------
    //! \brief Restore initial states and start the layout algorithm.
    //! \param[in] width Canvas width in pixels.
    //! \param[in] height Canvas height in pixels.
    //! \param[in,out] net The Petri net to layout.
    //----------------------------------------------------------------------
    void reset(float width, float height, Net& net);

    //----------------------------------------------------------------------
    //! \brief Stop the layout algorithm.
    //----------------------------------------------------------------------
    void reset() { m_net = nullptr; m_temperature = 0.0f; }

    //----------------------------------------------------------------------
    //! \brief Compute one step of forces if temperature is still hot else
    //! do nothing.
    //----------------------------------------------------------------------
    void update();

    //----------------------------------------------------------------------
    //! \brief Check if the algorithm is still running.
    //! \return true if still computing layout, false if converged.
    //----------------------------------------------------------------------
    bool isRunning() const { return m_net != nullptr && m_temperature >= 0.1f; }

    //----------------------------------------------------------------------
    //! \brief Get the current temperature (convergence indicator).
    //! \return Temperature value (0 = converged, higher = still moving).
    //----------------------------------------------------------------------
    float temperature() const { return m_temperature; }

    //----------------------------------------------------------------------
    //! \brief Const getter of vertices.
    //----------------------------------------------------------------------
    Vertices const& vertices() const { return m_vertices; }

private:

    //----------------------------------------------------------------------
    //! \brief Do a single step for computing forces.
    //----------------------------------------------------------------------
    void step();

    //----------------------------------------------------------------------
    //! \brief Euclidian norm.
    //! \param[in] v 2D vector.
    //! \return The length of the vector.
    //----------------------------------------------------------------------
    float length(Vec2 const& v) const
    {
        return std::max(0.001f, sqrtf(v.x * v.x + v.y * v.y));
    }

    //----------------------------------------------------------------------
    //! \brief Compute repulsive force.
    //! \param[in] dist Distance between two nodes.
    //! \return Repulsive force magnitude.
    //----------------------------------------------------------------------
    float repulsive_force(float dist) const
    {
        return K * K / dist / float(N) / 2.0f;
    }

    //----------------------------------------------------------------------
    //! \brief Compute attractive force.
    //! \param[in] dist Distance between two connected nodes.
    //! \return Attractive force magnitude.
    //----------------------------------------------------------------------
    float attractive_force(float dist) const
    {
        return dist * dist / K / float(N);
    }

    //----------------------------------------------------------------------
    //! \brief Reduce effect of forces (simulated annealing).
    //! \return New temperature value.
    //----------------------------------------------------------------------
    float cooling()
    {
        m_temperature *= 0.98f;
        return m_temperature;
    }

private:

    //! \brief The Petri net to layout.
    Net* m_net = nullptr;
    //! \brief Collection of vertices with displacement info.
    Vertices m_vertices;
    //! \brief Canvas width.
    float m_width = 0.0f;
    //! \brief Canvas height.
    float m_height = 0.0f;
    //! \brief Temperature for simulated annealing (reduces over time).
    float m_temperature = 0.0f;
    //! \brief Force coefficient: sqrt(area / num_vertices).
    float K = 0.0f;
    //! \brief Number of vertices.
    size_t N = 0;
};

} // namespace tpne

#endif