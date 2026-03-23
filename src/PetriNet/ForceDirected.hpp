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

#  include <vector>
#  include <unordered_map>
#  include <unordered_set>
#  include <cstdlib>
#  include <cmath>
#  include <random>

namespace tpne {

class Net;
class Node;

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
//! \brief Force-directed graph drawing algorithm (Fruchterman-Reingold 1991).
//!
//! Positions nodes of a graph so that edges have similar lengths and crossings
//! are minimized. Uses spring/repulsion model with simulated annealing.
//!
//! Forces:
//! - Attractive force: af(d) = d^2 / k (connected nodes attract)
//! - Repulsive force: rf(d) = -k^2 / d (all nodes repel)
//! - Gravity force: pulls nodes toward center (keeps graph compact)
//!
//! Where k = C * sqrt(area / num_vertices) is the optimal distance.
//!
//! Reference: https://youtu.be/WWm-g2nLHds
//! Inspired by: https://github.com/qdHe/Parallelized-Force-directed-Graph-Drawing
// *****************************************************************************
class ForceDirected
{
public:

    // *************************************************************************
    //! \brief Vertex is a 2D representation of a graph node.
    // *************************************************************************
    struct Vertex
    {
        explicit Vertex(Node* n) : node(n) {}

        Node* node = nullptr;              //!< Place or Transition pointer
        Vec2 displacement;                 //!< Accumulated force displacement
        std::vector<Node*> neighbors;      //!< Connected nodes (no duplicates)
    };

    using Vertices = std::vector<Vertex>;

    //----------------------------------------------------------------------
    //! \brief Algorithm parameters (can be tuned).
    //----------------------------------------------------------------------
    struct Parameters
    {
        float gravity = 0.1f;       //!< Gravity force toward center (0 = none)
        float cooling_rate = 0.95f; //!< Temperature decay per step (0.9-0.99)
        int steps_per_frame = 3;    //!< Iterations per update() call
        float min_temperature = 0.5f; //!< Stop when temperature below this
    };

    Parameters params;

public:

    //----------------------------------------------------------------------
    //! \brief Start the layout algorithm.
    //! \param[in] width Canvas width in pixels.
    //! \param[in] height Canvas height in pixels.
    //! \param[in,out] net The Petri net to layout.
    //! \param[in] randomize If true, randomize positions of overlapping nodes.
    //----------------------------------------------------------------------
    void reset(float width, float height, Net& net, bool randomize = true);

    //----------------------------------------------------------------------
    //! \brief Stop the layout algorithm.
    //----------------------------------------------------------------------
    void reset() { m_net = nullptr; m_temperature = 0.0f; }

    //----------------------------------------------------------------------
    //! \brief Compute multiple steps of forces.
    //! Does nothing if temperature has cooled below threshold.
    //----------------------------------------------------------------------
    void update();

    //----------------------------------------------------------------------
    //! \brief Check if the algorithm is still running.
    //----------------------------------------------------------------------
    bool isRunning() const { return m_net != nullptr && m_temperature >= params.min_temperature; }

    //----------------------------------------------------------------------
    //! \brief Get the current temperature (convergence indicator).
    //----------------------------------------------------------------------
    float temperature() const { return m_temperature; }

    //----------------------------------------------------------------------
    //! \brief Get the canvas center point.
    //----------------------------------------------------------------------
    Vec2 center() const { return {m_width / 2.0f, m_height / 2.0f}; }

    //----------------------------------------------------------------------
    //! \brief Const getter of vertices.
    //----------------------------------------------------------------------
    Vertices const& vertices() const { return m_vertices; }

private:

    void step();
    void initializePositions(bool randomize);

    float length(Vec2 const& v) const
    {
        return std::max(0.001f, sqrtf(v.x * v.x + v.y * v.y));
    }

    float repulsive_force(float dist) const
    {
        return m_K * m_K / dist;
    }

    float attractive_force(float dist) const
    {
        return dist * dist / m_K;
    }

    float cooling()
    {
        m_temperature *= params.cooling_rate;
        return m_temperature;
    }

private:

    Net* m_net = nullptr;
    Vertices m_vertices;
    std::unordered_map<Node*, size_t> m_node_to_index;
    float m_width = 0.0f;
    float m_height = 0.0f;
    float m_temperature = 0.0f;
    float m_K = 0.0f;          //!< Optimal distance between nodes
    std::mt19937 m_rng{42};    //!< Random generator for initial positions
};

} // namespace tpne

#endif