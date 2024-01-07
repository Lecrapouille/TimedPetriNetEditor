#include "Utils/History.hpp"

namespace tpne {

bool History::canUndo() const
{
    return m_undoList.size() > 0u;
}

bool History::canRedo() const
{
    return m_redoList.size() > 0u;
}

bool History::isDirty() const
{
    return m_nCleanCount != 0u;
}

bool History::undo()
{
    if (!canUndo())
        return false;

    m_nCleanCount--;
    History::Action::Ptr action = std::move(m_undoList.back());
    m_undoList.pop_back();
    if (action->undo())
    {
        addRedo(std::move(action));
        return true;
    }
    return false;
}

bool History::redo()
{
    if (!canRedo())
        return false;

    m_nCleanCount++;
    History::Action::Ptr action = std::move(m_redoList.back());
    m_redoList.pop_back();
    if (action->redo())
    {
        addUndo(std::move(action));
        return true;
    }
    return false;
}

void History::clear()
{
    m_undoList.clear();
    m_redoList.clear();
    m_nCleanCount = 0u;
}

void History::addUndo(History::Action::Ptr action)
{
    if (m_undoList.size() >= m_nUndoLevel)
    {
        m_undoList.pop_front();
    }
    m_undoList.push_back(std::move(action));
    if ((m_nCleanCount < 0u) && (m_redoList.size() > 0u))
    {
        m_nCleanCount = m_undoList.size() + m_redoList.size() + 1u;
    }
    else
    {
        m_nCleanCount++;
    }
}

void History::addRedo(History::Action::Ptr action)
{
    m_redoList.push_back(std::move(action));
}

} // namespace tpne