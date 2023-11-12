//=============================================================================
// TimedNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2023 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of TimedNetEditor.
//
// TimedNetEditor is free software: you can redistribute it and/or modify it
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

namespace tpne {

class Net;

// ****************************************************************************
//! \brief Container of sensors.
// ****************************************************************************
class Sensors
{
public:

    bool lookup(std::string const& key) const { return values.at(key); }
    void assign(std::string const& key, int const value) { values[key] = value; }
    std::map<const std::string, int>& database() { return values; }
    std::map<const std::string, int> const& database() const { return values; }
    void clear() { values.clear(); }

private:

    std::map<const std::string, int> values;
};

// *****************************************************************************
//! \brief
// *****************************************************************************
class Receptivity
{
public:

    // *************************************************************************
    //! \brief
    // *************************************************************************
    class BooleanExp
    {
    public:

        virtual ~BooleanExp() = default;
        virtual bool evaluate(Sensors const&) const = 0;
    };

    // *************************************************************************
    //! \brief
    // *************************************************************************
    class StepExp : public BooleanExp
    {
    public:

        StepExp(Net& net, std::string const& token);
        virtual bool evaluate(Sensors const& sensors) const override;

    private:

        Net& m_net;
        size_t m_id; // Place id
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

        virtual bool evaluate(Sensors const& sensors) const override
        {
            return sensors.lookup(m_name);
        }

    private:

        std::string m_name;
    };

    // *************************************************************************
    //! \brief
    // *************************************************************************
    class ConstExp : public BooleanExp
    {
    public:

        ConstExp(std::string const& op1)
        {
            if (op1 == "true")
                m_operand1 = true;
            else
                m_operand1 = false;
        }

        virtual ~ConstExp() = default;
        virtual bool evaluate(Sensors const& sensors) const override
        {
            return m_operand1;
        }

    private:

        bool m_operand1;
    };

    // *************************************************************************
    //! \brief
    // *************************************************************************
    class NotExp : public BooleanExp
    {
    public:

        NotExp(std::shared_ptr<BooleanExp> op1)
            : m_operand1(op1)
        {}

        virtual ~NotExp() = default;
        virtual bool evaluate(Sensors const& sensors) const override
        {
            return !m_operand1->evaluate(sensors);
        }

    private:

        std::shared_ptr<BooleanExp> m_operand1;
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
        virtual bool evaluate(Sensors const& sensors) const override
        {
            return m_operand1->evaluate(sensors) && m_operand2->evaluate(sensors);
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
        virtual bool evaluate(Sensors const& sensors) const override
        {
            return m_operand1->evaluate(sensors) || m_operand2->evaluate(sensors);
        }

    private:

        std::shared_ptr<BooleanExp> m_operand1;
        std::shared_ptr<BooleanExp> m_operand2;
    };

    // *****************************************************************************
    //! \brief
    // *****************************************************************************
    class Parser
    {
    public:

        //--------------------------------------------------------------------------
        //! \brief Rranslate the code of the receptivity to a given language.
        //--------------------------------------------------------------------------
        static std::string translate(std::string const& code, std::string const& lang);

        //--------------------------------------------------------------------------
        //! \brief Parse a postfix notation into an AST.
        //--------------------------------------------------------------------------
        static std::shared_ptr<BooleanExp> parse(Net& net, std::string const& code, std::string& error);

        //--------------------------------------------------------------------------
        //! \brief Parse a postfix notation into an AST.
        //--------------------------------------------------------------------------
        static bool evaluate(Receptivity const recept, Sensors const& sensors);

    private:

        //--------------------------------------------------------------------------
        //! brief
        //--------------------------------------------------------------------------
        static bool isBinaryOperator(std::string const& token);
        static bool isUnitaryOperator(std::string const& token);
        static bool isConst(std::string const& token);
        static bool isState(std::string const& token);
        static bool isVariable(std::string const& token);

        //--------------------------------------------------------------------------
        //! brief
        //--------------------------------------------------------------------------
        static std::vector<std::string> tokenizer(std::string const& s, std::string const& delimiter);

        //--------------------------------------------------------------------------
        //! brief
        //--------------------------------------------------------------------------
        static std::string convert(std::string const& token, std::string const& lang);
    };

public:

    std::shared_ptr<BooleanExp> expression;
    bool valid = false;
};

} // namespace tpne

#endif
