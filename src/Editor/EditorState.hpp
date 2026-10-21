// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef EDITOR_STATE_HPP
#  define EDITOR_STATE_HPP

#  include "PetriNet/PetriNet.hpp"
#  include "Editor/Messages.hpp"
#  include <list>
#  include <memory>
#  include <functional>

namespace tpne {

// *****************************************************************************
//! \brief Undo/redo system for the editor.
//! Stores actions that can be undone and redone.
// *****************************************************************************
class History
{
public:

    //! \brief Abstract base class for undoable actions
    class Action
    {
    public:
        using Ptr = std::unique_ptr<Action>;
        virtual ~Action() = default;
        virtual bool undo() = 0;
        virtual bool redo() = 0;
    };

    //--------------------------------------------------------------------------
    //! \brief Constructor.
    //! \param[in] max_levels Maximum number of undo levels to keep.
    //--------------------------------------------------------------------------
    explicit History(size_t max_levels = 100u)
        : m_max_levels(max_levels)
    {}

    //--------------------------------------------------------------------------
    //! \brief Add a new action to the history.
    //--------------------------------------------------------------------------
    void add(Action::Ptr action)
    {
        if (m_undo_stack.size() >= m_max_levels)
            m_undo_stack.pop_front();
        m_undo_stack.push_back(std::move(action));
        if (!m_redo_stack.empty())
            m_dirty_count = m_undo_stack.size() + m_redo_stack.size() + 1u;
        else
            m_dirty_count++;
    }

    //--------------------------------------------------------------------------
    //! \brief Check if undo is available.
    //--------------------------------------------------------------------------
    bool canUndo() const { return !m_undo_stack.empty(); }

    //--------------------------------------------------------------------------
    //! \brief Check if redo is available.
    //--------------------------------------------------------------------------
    bool canRedo() const { return !m_redo_stack.empty(); }

    //--------------------------------------------------------------------------
    //! \brief Undo the last action.
    //! \return true if successful.
    //--------------------------------------------------------------------------
    bool undo()
    {
        if (!canUndo())
            return false;
        m_dirty_count--;
        Action::Ptr action = std::move(m_undo_stack.back());
        m_undo_stack.pop_back();
        if (action->undo())
        {
            m_redo_stack.push_back(std::move(action));
            return true;
        }
        return false;
    }

    //--------------------------------------------------------------------------
    //! \brief Redo the last undone action.
    //! \return true if successful.
    //--------------------------------------------------------------------------
    bool redo()
    {
        if (!canRedo())
            return false;
        m_dirty_count++;
        Action::Ptr action = std::move(m_redo_stack.back());
        m_redo_stack.pop_back();
        if (action->redo())
        {
            m_undo_stack.push_back(std::move(action));
            return true;
        }
        return false;
    }

    //--------------------------------------------------------------------------
    //! \brief Check if there are unsaved changes.
    //--------------------------------------------------------------------------
    bool isDirty() const { return m_dirty_count != 0u; }

    //--------------------------------------------------------------------------
    //! \brief Clear all history.
    //--------------------------------------------------------------------------
    void clear()
    {
        m_undo_stack.clear();
        m_redo_stack.clear();
        m_dirty_count = 0u;
    }

private:

    std::list<Action::Ptr> m_undo_stack;
    std::list<Action::Ptr> m_redo_stack;
    size_t m_max_levels;
    size_t m_dirty_count = 0u;
};

// *****************************************************************************
//! \brief Action that stores complete net state for undo/redo.
//! \note Memory-intensive but simple. Stores full copies of the net.
// *****************************************************************************
class NetModificationAction : public History::Action
{
public:

    //--------------------------------------------------------------------------
    //! \brief Constructor.
    //! \param[in] net_getter Function returning reference to the current net.
    //--------------------------------------------------------------------------
    explicit NetModificationAction(std::function<Net&()> net_getter)
        : m_net_getter(std::move(net_getter))
    {}

    //--------------------------------------------------------------------------
    //! \brief Snapshot the net state before modification.
    //--------------------------------------------------------------------------
    void before(Net const& net) { m_before = net; }

    //--------------------------------------------------------------------------
    //! \brief Snapshot the net state after modification.
    //--------------------------------------------------------------------------
    void after(Net const& net) { m_after = net; }

    bool undo() override
    {
        m_net_getter() = m_before;
        return true;
    }

    bool redo() override
    {
        m_net_getter() = m_after;
        return true;
    }

private:

    std::function<Net&()> m_net_getter;
    Net m_before;
    Net m_after;
};

} // namespace tpne

#endif // EDITOR_STATE_HPP
