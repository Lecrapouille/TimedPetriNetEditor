#ifndef PETRIEDITOR_HISTORY_HPP
#  define PETRIEDITOR_HISTORY_HPP

#  include "TimedPetriNetEditor/PetriNet.hpp"
#  include <list>
#  include <memory>
#  include <functional>

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

// *****************************************************************************
//! \brief Quick and dirty net memorization for performing undo/redo.
//! Stores complete copies of the net before and after a modification.
//! \note This has high memory usage. A better approach would be to store
//! commands, but node ID changes on removal make this problematic.
// *****************************************************************************
class NetModificationAction : public History::Action
{
public:

    //--------------------------------------------------------------------------
    //! \brief Constructor.
    //! \param[in] net_getter Function returning a reference to the current net.
    //--------------------------------------------------------------------------
    explicit NetModificationAction(std::function<Net&()> net_getter)
        : m_net_getter(std::move(net_getter))
    {}

    //--------------------------------------------------------------------------
    //! \brief Store the net state before modification.
    //! \param[in] net The net to snapshot.
    //--------------------------------------------------------------------------
    void before(Net const& net) { m_before = net; }

    //--------------------------------------------------------------------------
    //! \brief Store the net state after modification.
    //! \param[in] net The net to snapshot.
    //--------------------------------------------------------------------------
    void after(Net const& net) { m_after = net; }

    //--------------------------------------------------------------------------
    //! \brief Restore the net to its state before modification.
    //! \return true on success.
    //--------------------------------------------------------------------------
    virtual bool undo() override
    {
        m_net_getter() = m_before;
        return true;
    }

    //--------------------------------------------------------------------------
    //! \brief Restore the net to its state after modification.
    //! \return true on success.
    //--------------------------------------------------------------------------
    virtual bool redo() override
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

#endif