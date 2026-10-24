// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef TPNE_RUNTIME_CALLBACK_HPP
#  define TPNE_RUNTIME_CALLBACK_HPP

#  include "PetriNet/Color.hpp"

#  include <functional>
#  include <memory>
#  include <type_traits>
#  include <utility>

namespace tpne {

// *****************************************************************************
//! \file The function called when a transition fires. Callback accepts any
//! type T (Sean Parent's type erasure):
//!   - a function or a lambda: called; it returns nothing (Success), a bool
//!     (Success or Failed) or a Color (the color of the tokens it produces);
//!   - any other type for which a free function `Color fire(T const&)` is
//!     found by ADL (in the namespace of T).
//! Optional free functions, found the same way, extend it (else defaults):
//!   - `void cancel(T const&)`, `void pause(T const&)`, `void resume(T const&)`
//!     (default: nothing);
//!   - `bool isSynchronous(T const&)` (default: false, the executor runs it;
//!     true: called at once by the loop of the instance).
// *****************************************************************************

namespace detail {

template<class T, class = void> struct HasFire : std::false_type {};
template<class T> struct HasFire<T, std::void_t<decltype(fire(std::declval<T const&>()))>> : std::true_type {};
template<class T, class = void> struct HasCancel : std::false_type {};
template<class T> struct HasCancel<T, std::void_t<decltype(cancel(std::declval<T const&>()))>> : std::true_type {};
template<class T, class = void> struct HasPause : std::false_type {};
template<class T> struct HasPause<T, std::void_t<decltype(pause(std::declval<T const&>()))>> : std::true_type {};
template<class T, class = void> struct HasResume : std::false_type {};
template<class T> struct HasResume<T, std::void_t<decltype(resume(std::declval<T const&>()))>> : std::true_type {};
template<class T, class = void> struct HasIsSynchronous : std::false_type {};
template<class T> struct HasIsSynchronous<T, std::void_t<decltype(isSynchronous(std::declval<T const&>()))>> : std::true_type {};

//! \brief Color of the result of a call.
template<class R, class F>
Color colorOf(F const& f)
{
    if constexpr (std::is_void_v<R>)
    {
        f();
        return Success;
    }
    else if constexpr (std::is_same_v<std::decay_t<R>, bool>)
    {
        return f() ? Success : Failed;
    }
    else
    {
        return static_cast<Color>(f());
    }
}

//! \brief Call the free functions of T (found by ADL). Out of the class
//! Callback, whose members would hide them.
template<class T> void callCancel(T const& t) { cancel(t); }
template<class T> void callPause(T const& t) { pause(t); }
template<class T> void callResume(T const& t) { resume(t); }
template<class T> bool callIsSynchronous(T const& t) { return isSynchronous(t); }

//! \brief Call the callback: invoke it, or its free function fire().
template<class T>
Color call(T const& callback)
{
    if constexpr (std::is_invocable_v<T const&>)
    {
        return colorOf<std::invoke_result_t<T const&>>([&callback]() { return std::invoke(callback); });
    }
    else
    {
        static_assert(HasFire<T>::value, "Callback: T shall be callable or have a free function fire(T const&)");
        return colorOf<decltype(fire(callback))>([&callback]() { return fire(callback); });
    }
}

} // namespace detail

// *****************************************************************************
//! \brief Function of a transition, of any type (see the file description).
//! Copies share the same function.
// *****************************************************************************
class Callback
{
public:

    template<class T, class = std::enable_if_t<!std::is_same_v<std::decay_t<T>, Callback>>>
    Callback(T callback)
        : m_self(std::make_shared<Model<T>>(std::move(callback)))
    {}

    //! \brief Run the function: the color of the tokens it produces.
    Color fire() const { return m_self->fire(); }
    void cancel() const { m_self->cancel(); }
    void pause() const { m_self->pause(); }
    void resume() const { m_self->resume(); }
    bool isSynchronous() const { return m_self->isSynchronous(); }

private:

    struct Concept
    {
        virtual ~Concept() = default;
        virtual Color fire() const = 0;
        virtual void cancel() const = 0;
        virtual void pause() const = 0;
        virtual void resume() const = 0;
        virtual bool isSynchronous() const = 0;
    };

    template<class T>
    struct Model final : Concept
    {
        explicit Model(T callback_) noexcept(std::is_nothrow_move_constructible_v<T>)
            : callback(std::move(callback_))
        {}

        Color fire() const override { return detail::call(callback); }

        void cancel() const override
        {
            if constexpr (detail::HasCancel<T>::value)
                detail::callCancel(callback);
        }

        void pause() const override
        {
            if constexpr (detail::HasPause<T>::value)
                detail::callPause(callback);
        }

        void resume() const override
        {
            if constexpr (detail::HasResume<T>::value)
                detail::callResume(callback);
        }

        bool isSynchronous() const override
        {
            if constexpr (detail::HasIsSynchronous<T>::value)
                return detail::callIsSynchronous(callback);
            else
                return false;
        }

        T callback;
    };

    std::shared_ptr<Concept const> m_self;
};

} // namespace tpne

#endif
