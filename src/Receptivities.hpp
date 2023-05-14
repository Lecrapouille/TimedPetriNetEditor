//=============================================================================
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
//=============================================================================

#ifndef RECEPTIVITIES_HPP
#  define RECEPTIVITIES_HPP

#  include <memory>
#  include <string>
#  include <stack>
#  include <map>
#  include <vector>
#  include <iostream>

// *****************************************************************************
//! \brief
// *****************************************************************************
class Receptivity
{
    // *************************************************************************
    //! \brief
    // *************************************************************************
    class Context
    {
    public:

        bool lookup(std::string const& key) const { return m.at(key); }
        void assign(std::string const& key, bool const value) { m[key] = value; }

    private:

        std::map<const std::string, bool> m;
    };

    // *************************************************************************
    //! \brief
    // *************************************************************************
    class BooleanExp
    {
    public:

        virtual ~BooleanExp() = default;
        virtual bool evaluate(Context const&) const = 0;
    };

    // *************************************************************************
    //! \brief
    // *************************************************************************
    class VariableExp : public BooleanExp
    {
    public:

        VariableExp(std::string name)
            : m_name(name)
        {}

        virtual bool evaluate(Context const& context) const
        {
            return context.lookup(m_name);
        }

    private:

        std::string m_name;
    };

    // *************************************************************************
    //! \brief
    // *************************************************************************
    class AndExp : public BooleanExp
    {
    public:

        AndExp(std::shared_ptr<BooleanExp> op1, std::shared_ptr<BooleanExp> op2)
            : m_operand1(op1), m_operand2(op2)
        {}

        virtual ~AndExp() = default;
        virtual bool evaluate(Context const& context) const
        {
            return m_operand1->evaluate(context) && m_operand2->evaluate(context);
        }

    private:

        std::shared_ptr<BooleanExp> m_operand1;
        std::shared_ptr<BooleanExp> m_operand2;
    };

    // *************************************************************************
    //! \brief
    // *************************************************************************
    class OrExp : public BooleanExp
    {
    public:

        OrExp(std::shared_ptr<BooleanExp> op1, std::shared_ptr<BooleanExp> op2)
            : m_operand1(op1), m_operand2(op2)
        {}

        virtual ~OrExp() = default;
        virtual bool evaluate(Context const& context) const
        {
            return m_operand1->evaluate(context) || m_operand2->evaluate(context);
        }

    private:

        std::shared_ptr<BooleanExp> m_operand1;
        std::shared_ptr<BooleanExp> m_operand2;
    };

public:

    //--------------------------------------------------------------------------
    //! brief Postfix to Infix
    //--------------------------------------------------------------------------
    void code(std::string const& code_);
    std::string const& code() const { return m_code; }

    //--------------------------------------------------------------------------
    //! \brief Rranslate the code of the receptivity to a given language.
    //--------------------------------------------------------------------------
    std::string translate(std::string const& lang);

private:

    //--------------------------------------------------------------------------
    //! brief
    //--------------------------------------------------------------------------
    bool isOperator(std::string const& token);

    //--------------------------------------------------------------------------
    //! brief
    //--------------------------------------------------------------------------
    std::vector<std::string> tokenizer(std::string const& s, std::string const& delimiter);

    //--------------------------------------------------------------------------
    //! brief
    //--------------------------------------------------------------------------
    std::string convert(std::string const& token, std::string const& lang);

    //--------------------------------------------------------------------------
    //! \brief Postfix to Infix
    //--------------------------------------------------------------------------
    std::shared_ptr<BooleanExp> parse();

public:

    //static
    std::string m_code;
    std::shared_ptr<BooleanExp> m_expression;
};

#if 0
int main()
{
    // Receptivite de la transition
    std::string receptivite("Dcy X14 . foo +");

    // Traduction vers C++ ou structure text
    std::string infix = postfixToInfix(receptivite, "C");

    std::cout << "Postfix expression : " << receptivite << std::endl;
    std::cout << "Infix expression : " << infix << std::endl;

    // Vers AST pour l'editeur
    std::shared_ptr<BooleanExp> expression = ast("Dcy X14 . foo +");
    Context context; // sensors
    context.assign("Dcy", false);
    context.assign("X14", true);
    context.assign("foo", true);

    // TODO Dear Im Gui
    // for (auto const& it: context) { ImGui::InputFloat(it.first, it.second); }

    bool result = expression->evaluate(context);
    std::cout << result << '\n';

    bool Dcy = false; bool X14 = true; bool foo = true;
    std::cout << "Attendu: " << ((Dcy & X14) | foo) << '\n';

    return 0;
}
#endif

#endif