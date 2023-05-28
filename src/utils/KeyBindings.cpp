//=============================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2023 Quentin Quadrat <lecrapouille@gmail.com>
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

#include "KeyBindings.hpp"

//------------------------------------------------------------------------------
std::string const& to_str(sf::Keyboard::Key const key)
{
#if !defined(_WIN32)
#  pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#endif
#define ITEM(x) [sf::Keyboard::x + 1u] = #x // +1u because sf::Keyboard::Unknown starts from -1

    static_assert(sf::Keyboard::KeyCount == 101, "Number of SFML keys has changed");
    static const std::string s_keys[sf::Keyboard::KeyCount + 1u] =
    {
        ITEM(Unknown),
        ITEM(A),
        ITEM(B),
        ITEM(C),
        ITEM(D),
        ITEM(E),
        ITEM(F),
        ITEM(G),
        ITEM(H),
        ITEM(I),
        ITEM(J),
        ITEM(K),
        ITEM(L),
        ITEM(M),
        ITEM(N),
        ITEM(O),
        ITEM(P),
        ITEM(Q),
        ITEM(R),
        ITEM(S),
        ITEM(T),
        ITEM(U),
        ITEM(V),
        ITEM(W),
        ITEM(X),
        ITEM(Y),
        ITEM(Z),
        ITEM(Num0),
        ITEM(Num1),
        ITEM(Num2),
        ITEM(Num3),
        ITEM(Num4),
        ITEM(Num5),
        ITEM(Num6),
        ITEM(Num7),
        ITEM(Num8),
        ITEM(Num9),
        ITEM(Escape),
        ITEM(LControl),
        ITEM(LShift),
        ITEM(LAlt),
        ITEM(LSystem),
        ITEM(RControl),
        ITEM(RShift),
        ITEM(RAlt),
        ITEM(RSystem),
        ITEM(Menu),
        ITEM(LBracket),
        ITEM(RBracket),
        ITEM(SemiColon),
        ITEM(Comma),
        ITEM(Period),
        ITEM(Quote),
        ITEM(Slash),
        ITEM(BackSlash),
        ITEM(Tilde),
        ITEM(Equal),
        ITEM(Dash),
        ITEM(Space),
        ITEM(Return),
        ITEM(BackSpace),
        ITEM(Tab),
        ITEM(PageUp),
        ITEM(PageDown),
        ITEM(End),
        ITEM(Home),
        ITEM(Insert),
        ITEM(Delete),
        ITEM(Add),
        ITEM(Subtract),
        ITEM(Multiply),
        ITEM(Divide),
        ITEM(Left),
        ITEM(Right),
        ITEM(Up),
        ITEM(Down),
        ITEM(Numpad0),
        ITEM(Numpad1),
        ITEM(Numpad2),
        ITEM(Numpad3),
        ITEM(Numpad4),
        ITEM(Numpad5),
        ITEM(Numpad6),
        ITEM(Numpad7),
        ITEM(Numpad8),
        ITEM(Numpad9),
        ITEM(F1),
        ITEM(F2),
        ITEM(F3),
        ITEM(F4),
        ITEM(F5),
        ITEM(F6),
        ITEM(F7),
        ITEM(F8),
        ITEM(F9),
        ITEM(F10),
        ITEM(F11),
        ITEM(F12),
        ITEM(F13),
        ITEM(F14),
        ITEM(F15),
        ITEM(Pause),
    };

#undef ITEM
#if !defined(_WIN32)
#  pragma GCC diagnostic pop
#endif

    if (key < sf::Keyboard::KeyCount)
        return s_keys[key + 1u];
    return s_keys[sf::Keyboard::Unknown + 1u];
}
