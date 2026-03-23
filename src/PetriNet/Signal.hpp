//=============================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2026 Quentin Quadrat <lecrapouille@gmail.com>
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
//=============================================================================

#pragma once

#include <cstddef>
#include <functional>
#include <map>

namespace tpne {

// ****************************************************************************
//! \brief Signal/Slot system for event handling.
//!
//! This class provides a signal/slot mechanism for connecting callbacks
//! to events. Slots can be connected and disconnected, and signals can
//! be emitted to call all connected slots.
//!
//! \tparam Args Parameter types for the signal.
// ****************************************************************************
template <typename... Args>
class Signal
{
public:

    using SlotType = std::function<void(Args...)>;
    using ConnectionId = size_t;

    // ------------------------------------------------------------------------
    //! \brief Connect a slot (function, lambda, method).
    //! \param p_slot The slot to connect.
    //! \return Connection ID for later disconnection.
    // ------------------------------------------------------------------------
    ConnectionId connect(SlotType p_slot)
    {
        ConnectionId id = m_next_id++;
        m_slots[id] = std::move(p_slot);
        return id;
    }

    // ------------------------------------------------------------------------
    //! \brief Disconnect a slot by its connection ID.
    //! \param p_id Connection ID to disconnect.
    // ------------------------------------------------------------------------
    void disconnect(ConnectionId p_id)
    {
        m_slots.erase(p_id);
    }

    // ------------------------------------------------------------------------
    //! \brief Disconnect all slots.
    // ------------------------------------------------------------------------
    void disconnectAll()
    {
        m_slots.clear();
    }

    // ------------------------------------------------------------------------
    //! \brief Emit the signal, calling all connected slots.
    //! \param p_args Arguments to pass to the slots.
    // ------------------------------------------------------------------------
    void emit(Args... p_args)
    {
        // Copy to avoid issues if a slot disconnects during emission
        auto m_slotscopy = m_slots;
        for (const auto& [id, slot] : m_slotscopy)
        {
            if (m_slots.count(id)) // Check if still connected
            {
                slot(p_args...);
            }
        }
    }

    // ------------------------------------------------------------------------
    //! \brief Operator() for more natural syntax.
    //! \param p_args Arguments to pass to the slots.
    // ------------------------------------------------------------------------
    void operator()(Args... p_args)
    {
        emit(p_args...);
    }

    // ------------------------------------------------------------------------
    //! \brief Get the number of connected slots.
    //! \return Number of connections.
    // ------------------------------------------------------------------------
    size_t connectionCount() const
    {
        return m_slots.size();
    }

private:

    //! \brief Map of connection IDs to slots.
    std::map<ConnectionId, SlotType> m_slots;
    //! \brief Next connection ID to assign.
    ConnectionId m_next_id = 0;
};

// ****************************************************************************
//! \brief RAII wrapper for automatic connection management.
//!
//! This class automatically disconnects a signal connection when it goes
//! out of scope.
//!
//! \tparam Args Parameter types for the signal.
// ****************************************************************************
template <typename... Args>
class ScopedConnection
{
public:

    // ------------------------------------------------------------------------
    //! \brief Constructor.
    //! \param p_signal Pointer to the signal.
    //! \param p_id Connection ID.
    // ------------------------------------------------------------------------
    ScopedConnection(Signal<Args...>* p_signal,
                     typename Signal<Args...>::ConnectionId p_id)
        : m_signal(p_signal), m_id(p_id)
    {
    }

    // ------------------------------------------------------------------------
    //! \brief Destructor - automatically disconnects.
    // ------------------------------------------------------------------------
    ~ScopedConnection()
    {
        if (m_signal)
        {
            m_signal->disconnect(m_id);
        }
    }

    // Delete copy operations
    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;

    // ------------------------------------------------------------------------
    //! \brief Move constructor.
    // ------------------------------------------------------------------------
    ScopedConnection(ScopedConnection&& p_other) noexcept
        : m_signal(p_other.m_signal), m_id(p_other.m_id)
    {
        p_other.m_signal = nullptr;
    }

    // ------------------------------------------------------------------------
    //! \brief Move assignment operator.
    // ------------------------------------------------------------------------
    ScopedConnection& operator=(ScopedConnection&& p_other) noexcept
    {
        if (this != &p_other)
        {
            // Disconnect current connection
            if (m_signal)
            {
                m_signal->disconnect(m_id);
            }
            // Move from other
            m_signal = p_other.m_signal;
            m_id = p_other.m_id;
            p_other.m_signal = nullptr;
        }
        return *this;
    }

private:

    //! \brief Pointer to the signal.
    Signal<Args...>* m_signal;
    //! \brief Connection ID.
    typename Signal<Args...>::ConnectionId m_id;
};

} // namespace tpne
