//=====================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2022 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of TimedPetriNetEditor.
//
// TimedPetriNetEditor is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GNU Emacs.  If not, see <http://www.gnu.org/licenses/>.
//=====================================================================

#ifndef UTILS_HPP
#  define UTILS_HPP

#  include <SFML/System.hpp>
#  include <math.h>
#  include <sstream>
#  include <vector>
#  include <unistd.h> // tmpPetriFile()
#  include <iomanip>  // tmpPetriFile()
#  include <pwd.h>    // tmpPetriFile()
#  include <memory>
#  include <sys/stat.h>
#  include <sys/types.h>

#  ifdef __APPLE__
#    include <CoreFoundation/CFBundle.h>
#  endif

//------------------------------------------------------------------------------
// Return the data folder.
//------------------------------------------------------------------------------
inline std::string data_path(std::string const& file)
{
    struct stat exists; // folder exists ?
    std::string path;

#  ifdef __APPLE__

    // Return the resources folder inside MacOS bundle application
    CFURLRef resourceURL = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
    char resourcePath[PATH_MAX];
    if (CFURLGetFileSystemRepresentation(resourceURL, true,
                                         reinterpret_cast<UInt8 *>(resourcePath),
                                         PATH_MAX))
    {
        if (resourceURL != NULL)
        {
            CFRelease(resourceURL);
        }

        path = std::string(resourcePath) + "/" + file;
        if (stat(path.c_str(), &exists) == 0)
        {
            return path;
        }
    }

#endif

#ifdef DATADIR
    path = std::string(DATADIR) + "/" + file;
    if (stat(path.c_str(), &exists) == 0)
    {
        return path;
    }
#endif

    path = "data/" + file;
    if (stat(path.c_str(), &exists) == 0)
    {
        return path;
    }

    return file;
}

//------------------------------------------------------------------------------
// Make the C++14 std::make_unique available for C++11 and Visual Studio.
//------------------------------------------------------------------------------
#  if __cplusplus == 201103L

// These compilers do not support make_unique so redefine it
namespace std
{
    template<class T> struct _Unique_if
    {
        typedef unique_ptr<T> _Single_object;
    };

    template<class T> struct _Unique_if<T[]>
    {
        typedef unique_ptr<T[]> _Unknown_bound;
    };

    template<class T, size_t N> struct _Unique_if<T[N]>
    {
        typedef void _Known_bound;
    };

    template<class T, class... Args>
    typename _Unique_if<T>::_Single_object
    make_unique(Args&&... args)
    {
        return unique_ptr<T>(new T(std::forward<Args>(args)...));
    }

    template<class T>
    typename _Unique_if<T>::_Unknown_bound
    make_unique(size_t n)
    {
        typedef typename remove_extent<T>::type U;
        return unique_ptr<T>(new U[n]());
    }

    //! \brief Implement the C++14 std::make_unique for C++11
    template<class T, class... Args>
    typename _Unique_if<T>::_Known_bound
    make_unique(Args&&...) = delete;
}

#  endif // __cplusplus == 201103L

//------------------------------------------------------------------------------
inline float sqrtdist(sf::Vector2f const& a, sf::Vector2f const& b)
{
   return (b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y);
}

//------------------------------------------------------------------------------
inline float distance(sf::Vector2f const& a, sf::Vector2f const& b)
{
   return sqrtf((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
}

//------------------------------------------------------------------------------
inline float norm(const float xa, const float ya, const float xb, const float yb)
{
    return sqrtf((xb - xa) * (xb - xa) + (yb - ya) * (yb - ya));
}

//------------------------------------------------------------------------------
inline float random(int lower, int upper)
{
    auto const t = static_cast<unsigned int>(time(NULL));
    srand(t);
    return float(rand() % (upper - lower + 1)) + float(lower);
}

//------------------------------------------------------------------------------
inline const char* current_time()
{
    static char buffer[32];

    time_t current_time = ::time(nullptr);
    strftime(buffer, sizeof (buffer), "[%H:%M:%S] ", localtime(&current_time));
    return buffer;
}

//------------------------------------------------------------------------------
template<typename T> T convert_to(const std::string &str)
{
    std::istringstream ss(str); T num; ss >> num; return num;
}

template<typename T> T convert_to(const char* str)
{
    std::istringstream ss(str); T num; ss >> num; return num;
}

//------------------------------------------------------------------------------
inline size_t token2vector(std::string const& s, std::vector<std::string>& words)
{
    std::stringstream ss(s);
    std::string tmp;

    words.clear();
    while (getline(ss, tmp, ','))
    {
        words.push_back(tmp);
    }

    return words.size();
}

//------------------------------------------------------------------------------
//! \brief Return current date as string.
inline std::string currentDate()
{
    std::ostringstream oss;
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    oss << std::put_time(&tm, "%Y-%m-%d__%H-%M-%S");
    return oss.str();
}

//------------------------------------------------------------------------------
//! \brief Create the name of a temporary Petri file.
inline std::string tmpPetriFile()
{
    std::ostringstream oss;
    oss << getpwuid(getuid())->pw_dir; // Home folder
    oss << ".TimedPetriNetEditor/petri__" << currentDate() << ".json";
    return oss.str();
}

//------------------------------------------------------------------------------
inline uint8_t fading(sf::Clock& timer, bool restart, float blink_period)
{
    if (restart)
        timer.restart();

    float period = timer.getElapsedTime().asSeconds();
    if (period >= blink_period)
        period = blink_period;

    return uint8_t(255.0f - (255.0f * period / blink_period));
}

//------------------------------------------------------------------------------
//! \brief Helper structure for building sparse matrix for the exportation of
//! the Petri net to Julia language (Julia is a vectorial language mixing Matlab
//! and Python syntax but with a faster runtime) as Max-Plus dynamical linear
//! systems (State space representation).
//!
//! This class is only used for storing elements not for doing matrix
//! operations. In Julia, a sparse matrix of dimensions m x n is built with the
//! function sparse(I, J, D, n, m) where I, J are two column vectors indicating
//! coordinates of the non-zero elements, D is column vector holding values to
//! be stored. Note that in Julia indexes starts at 1, contrary to C/C++
//! starting at 0.
//------------------------------------------------------------------------------
struct SparseMatrix
{
    SparseMatrix(size_t const N_ = 0u, size_t const M_ = 0u)
        : N(N_), M(M_)
    {}

    void dim(size_t const N_, size_t const M_)
    {
        N = N_;
        M = M_;
    }

    void clear()
    {
        i.clear();
        j.clear();
        d.clear();
    }

    //! \brief Beware double inclusions are not checked
    void add(size_t i_, size_t j_, float d_)
    {
        i.push_back(i_ + 1u);
        j.push_back(j_ + 1u);
        d.push_back(d_);
    }

    //! \brief (I,J) Coordinates
    std::vector<size_t> i, j;
    //! \brief Non zero element (double to be usable by Julia)
    std::vector<double> d;
    //! \brief Matrix dimension
    size_t N, M;
    //! \brief Option to display for C++ or for Julia
    static bool display_for_julia;
};

//------------------------------------------------------------------------------
//! \brief Output sparse matrix as I, J, D where I, J and D are 3 vectors.
//------------------------------------------------------------------------------
inline std::ostream & operator<<(std::ostream &os, SparseMatrix const& matrix)
{
    std::string separator;

    if (!matrix.display_for_julia)
    {
        os << matrix.M << "x" << matrix.N << " sparse (max,+) matrix with "
        << matrix.d.size() << " stored entry:" << std::endl;
    }
    os << "[";
    for (auto const& it: matrix.i)
    {
        os << separator << (matrix.display_for_julia ? it : (it - 1));
        separator = ", ";
    }

    os << "], [";
    separator.clear();
    for (auto const& it: matrix.j)
    {
        os << separator << (matrix.display_for_julia ? it : (it - 1));
        separator = ", ";
    }

    os << "], MP([";
    separator.clear();
    for (auto const& it: matrix.d)
    {
        os << separator << it;
        separator = ", ";
    }
    os << "])";
    if (matrix.display_for_julia)
    {
        os << ", " << matrix.M << ", " << matrix.N;
    }

    return os;
}

#endif
