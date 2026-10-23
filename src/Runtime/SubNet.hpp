// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef TPNE_RUNTIME_SUBNET_HPP
#  define TPNE_RUNTIME_SUBNET_HPP

#  include "Runtime/Runtime.hpp"

#  include <algorithm>
#  include <memory>
#  include <mutex>
#  include <string>
#  include <vector>

namespace tpne {

// *****************************************************************************
//! \brief A net run as the function of a transition of another net (a macro
//! step): each firing of the transition runs a new instance of the runtime
//! until it ends, and its end (Success, Deadlocked, Canceled, Failed) is the
//! result of the transition. Canceling, pausing or resuming the transition
//! does the same to the instances running.
//!
//! \code
//! auto sub = std::make_shared<Runtime>(pick_net);
//! sub->on("grasp", ...);
//! parent.on("pick", SubNet(sub, { { "Done", Success } }));
//! \endcode
// *****************************************************************************
class SubNet
{
public:

    //--------------------------------------------------------------------------
    //! \brief The runtime of the sub net, the goal and the initial tokens of
    //! its instances (see Runtime::instantiate()).
    //--------------------------------------------------------------------------
    explicit SubNet(std::shared_ptr<Runtime> runtime, std::vector<PlaceTokens> goal = {},
                    std::vector<PlaceTokens> initial = {})
        : m_shared(std::make_shared<Shared>())
    {
        m_shared->runtime = std::move(runtime);
        m_shared->goal = std::move(goal);
        m_shared->initial = std::move(initial);
    }

    //--------------------------------------------------------------------------
    //! \brief Instances run so far (their journals).
    //--------------------------------------------------------------------------
    std::vector<std::shared_ptr<Instance>> instances() const
    {
        std::lock_guard<std::mutex> lock(m_shared->mutex);
        return m_shared->done;
    }

private:

    friend Color fire(SubNet const& sub);
    friend void cancel(SubNet const& sub);
    friend void pause(SubNet const& sub);
    friend void resume(SubNet const& sub);

    //! \brief Shared by the copies of the callback.
    struct Shared
    {
        std::shared_ptr<Runtime> runtime;
        std::vector<PlaceTokens> goal;
        std::vector<PlaceTokens> initial;
        std::mutex mutex;
        //! \brief Instances running, and ended.
        std::vector<std::shared_ptr<Instance>> running;
        std::vector<std::shared_ptr<Instance>> done;
        size_t count = 0u;
    };

    std::shared_ptr<Shared> m_shared;
};

//------------------------------------------------------------------------------
//! \brief Run a new instance of the sub net until it ends.
//------------------------------------------------------------------------------
inline Color fire(SubNet const& sub)
{
    SubNet::Shared& shared = *sub.m_shared;
    std::shared_ptr<Instance> instance;
    {
        std::lock_guard<std::mutex> lock(shared.mutex);
        instance = shared.runtime->instantiate("sub" + std::to_string(shared.count++),
                                               shared.initial, shared.goal);
        shared.running.push_back(instance);
    }
    Color const result = shared.runtime->run(*instance);
    {
        std::lock_guard<std::mutex> lock(shared.mutex);
        shared.running.erase(std::find(shared.running.begin(), shared.running.end(), instance));
        shared.done.push_back(instance);
    }
    return result;
}

//------------------------------------------------------------------------------
//! \brief Cancel, pause or resume the instances running.
//------------------------------------------------------------------------------
inline void cancel(SubNet const& sub)
{
    std::lock_guard<std::mutex> lock(sub.m_shared->mutex);
    for (auto const& instance : sub.m_shared->running)
        sub.m_shared->runtime->cancel(*instance);
}

inline void pause(SubNet const& sub)
{
    std::lock_guard<std::mutex> lock(sub.m_shared->mutex);
    for (auto const& instance : sub.m_shared->running)
        sub.m_shared->runtime->pause(*instance);
}

inline void resume(SubNet const& sub)
{
    std::lock_guard<std::mutex> lock(sub.m_shared->mutex);
    for (auto const& instance : sub.m_shared->running)
        sub.m_shared->runtime->resume(*instance);
}

} // namespace tpne

#endif
