#ifndef PETRIEDITOR_HISTORY_HPP
#  define PETRIEDITOR_HISTORY_HPP

#  include <list>
#  include <memory>

namespace tpne {

// ****************************************************************************
//! \brief Memorize user actions on the Net in the aim to allow undo/redo.
//! Code based on
//! https://www.codeproject.com/Articles/2500/A-Basic-Undo-Redo-Framework-For-C
// ****************************************************************************
class History
{
public:

	// ************************************************************************
	//! \brief
	// ************************************************************************
	class Action
	{
	public:
    using Ptr = std::unique_ptr<Action>;

		virtual ~Action() = default;
		//! \brief Do the action
		virtual bool undo() = 0;
		//! \brief Undo the action
		virtual bool redo() = 0;
	};

public:

    History(size_t const nUndoLevel = 10u)
		  : m_nUndoLevel(nUndoLevel)
    {}

    void add(History::Action::Ptr action) { addUndo(std::move(action)); }
    bool canUndo() const;
    bool undo();
    bool canRedo() const;
    bool redo();

    bool isDirty() const;
    void clear();

private:

    void addRedo(History::Action::Ptr action);
    void addUndo(History::Action::Ptr action);

private:

    std::list<History::Action::Ptr> m_undoList;
    std::list<History::Action::Ptr> m_redoList;
    size_t m_nUndoLevel;
    size_t m_nCleanCount = 0u;
};

} // namespace tpne

#endif