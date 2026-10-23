// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef TPNE_RUNTIME_EXECUTOR_HPP
#  define TPNE_RUNTIME_EXECUTOR_HPP

#  include <condition_variable>
#  include <cstddef>
#  include <deque>
#  include <functional>
#  include <mutex>
#  include <thread>
#  include <vector>

namespace tpne {

// *****************************************************************************
//! \brief Runs the functions of the transitions that are not synchronous. The
//! runtime only posts tasks: the executor chooses where they run.
// *****************************************************************************
class Executor
{
public:

    virtual ~Executor() = default;

    //--------------------------------------------------------------------------
    //! \brief Run the task, now or later, in this thread or another one.
    //--------------------------------------------------------------------------
    virtual void post(std::function<void()> task) = 0;
};

// *****************************************************************************
//! \brief Runs each task at once in the thread posting it: no thread, the
//! results are deterministic (tests, Arduino).
// *****************************************************************************
class InlineExecutor final : public Executor
{
public:

    void post(std::function<void()> task) override { task(); }
};

// *****************************************************************************
//! \brief Runs the tasks on a pool of threads (desktop, micro-services).
// *****************************************************************************
class ThreadPoolExecutor final : public Executor
{
public:

    //--------------------------------------------------------------------------
    //! \brief Start the threads (at least one).
    //--------------------------------------------------------------------------
    explicit ThreadPoolExecutor(size_t const threads = std::thread::hardware_concurrency())
    {
        size_t const n = (threads == 0u) ? 1u : threads;
        m_threads.reserve(n);
        for (size_t i = 0u; i < n; ++i)
            m_threads.emplace_back([this]() { work(); });
    }

    //--------------------------------------------------------------------------
    //! \brief Run the tasks already posted, then stop the threads.
    //--------------------------------------------------------------------------
    ~ThreadPoolExecutor() override
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stopping = true;
        }
        m_condition.notify_all();
        for (auto& thread : m_threads)
            thread.join();
    }

    ThreadPoolExecutor(ThreadPoolExecutor const&) = delete;
    ThreadPoolExecutor& operator=(ThreadPoolExecutor const&) = delete;

    void post(std::function<void()> task) override
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_tasks.push_back(std::move(task));
        }
        m_condition.notify_one();
    }

private:

    void work()
    {
        for (;;)
        {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_condition.wait(lock, [this]() { return m_stopping || !m_tasks.empty(); });
                if (m_tasks.empty())
                    return;
                task = std::move(m_tasks.front());
                m_tasks.pop_front();
            }
            task();
        }
    }

    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::deque<std::function<void()>> m_tasks;
    std::vector<std::thread> m_threads;
    bool m_stopping = false;
};

} // namespace tpne

// *****************************************************************************
//! \brief Taskflow (https://github.com/taskflow/taskflow) as executor, when its
//! headers are found and TPNE_WITH_TASKFLOW is defined: its work-stealing
//! threads run the tasks.
// *****************************************************************************
#  if defined(TPNE_WITH_TASKFLOW)
#    include <taskflow/taskflow.hpp>

namespace tpne {

class TaskflowExecutor final : public Executor
{
public:

    explicit TaskflowExecutor(size_t const threads = std::thread::hardware_concurrency())
        : m_executor(threads)
    {}

    ~TaskflowExecutor() override { m_executor.wait_for_all(); }

    void post(std::function<void()> task) override
    {
        m_executor.silent_async(std::move(task));
    }

private:

    tf::Executor m_executor;
};

} // namespace tpne

#  endif

#endif
