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
//! \brief Quick and dirty container of sensor boolean values.
//! \fixme should store analogical value.
// ****************************************************************************
class Sensors
{
public:

    static Sensors& instance() { static Sensors sensor; return sensor; }

    //! \brief Get the value of the given sensor. Throw if the sensor is unkown.
    bool get(std::string const& sensor) { return m_values.at(sensor); modified = true; }
    void set(std::string const& sensor, int const value) { m_values[sensor] = value; }
    std::map<const std::string, int>& database() { return m_values; }
    void clear() { m_values.clear(); modified = false; }

private:

    Sensors() = default;

private:

    std::map<const std::string, int> m_values;

public:
    bool modified = false;
};

// *****************************************************************************
//! \brief Receptivity is the boolean expression stored in GRAFCET transitions
//! that make a tramsition fireable or not. This class allows to parse simple
//! boolean expressions and create an abstrqct syntaxt tree (AST). For simplicity
//! reasons the syntax use reversed polish notation (RPN). Therefore expression
//! such as "(a or b) and X0" will be written as "a b or X0 and".
// *****************************************************************************
class Receptivity
{
public:

    std::string compile(std::string const& code, Net& net);
    inline bool isValid() const { return m_valid; }
    inline std::string const& error() const { return m_error; }
    bool evaluate();

    // *************************************************************************
    //! \brief Base class of boolean expression used as AST node.
    // *************************************************************************
    class BooleanExp
    {
    public:

        virtual ~BooleanExp() = default;
        virtual bool evaluate() const = 0;
    };

    // *************************************************************************
    //! \brief Expression for GRAFCET place state. In GRAFCET places are named
    //! steps and are named X0, X1 ... (while inside the editor places ID are
    //! named P0, P1, ...) we are following the GRAFCET standard.
    //! \note the GRAFCET net shall no remove the place !
    // *************************************************************************
    class StepExp : public BooleanExp
    {
    public:

        //! \param[in] name Step (aka place0 name, i.e. "X0".
        StepExp(Net& net, std::string const& name);
        virtual bool evaluate() const override;

    private:

        Net& m_net;
        //! \brief Place ID. The GRAFCET net shall no remove the place t let this
        //! ID valid.
        size_t m_id;
    };

    // *************************************************************************
    //! \brief Expression for sensor variable name (i.e. "foo")
    // *************************************************************************
    class VariableExp : public BooleanExp
    {
    public:

        VariableExp(std::string name)
            : m_name(name)
        {}

        virtual bool evaluate() const override;

    private:

        std::string m_name;
    };

    // *************************************************************************
    //! \brief Expression for constant (i.e. "true" and "false")
    // *************************************************************************
    class ConstExp : public BooleanExp
    {
    public:

        ConstExp(std::string const& operand);
        virtual ~ConstExp() = default;
        virtual bool evaluate() const override
        {
            return m_operand;
        }

    private:

        bool m_operand;
    };

    // *************************************************************************
    //! \brief Expression for negation (i.e. "foo !")
    // *************************************************************************
    class NotExp : public BooleanExp
    {
    public:

        NotExp(std::shared_ptr<BooleanExp> operand)
            : m_operand(operand)
        {}

        virtual ~NotExp() = default;
        virtual bool evaluate() const override
        {
            return !m_operand->evaluate();
        }

    private:

        std::shared_ptr<BooleanExp> m_operand;
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
        virtual bool evaluate() const override
        {
            return m_operand1->evaluate() && m_operand2->evaluate();
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
        virtual bool evaluate() const override
        {
            return m_operand1->evaluate() || m_operand2->evaluate();
        }

    private:

        std::shared_ptr<BooleanExp> m_operand1;
        std::shared_ptr<BooleanExp> m_operand2;
    };

    // *****************************************************************************
    //! \brief Quick Reverse Polish Notation parser of boolean expression set inside
    //! GRAFCET transitions (i.e. "a b . X0 + !" for "not((a and b) or X0)").
    //! \fixme Currently management of bad syntax is not made (assert and throw).
    // *****************************************************************************
    class Parser
    {
    public:

        //--------------------------------------------------------------------------
        //! \brief Translate the given RPN code of the receptivity to a desired
        //! language. Currently supported languages are:
        //!  - C language "C"
        //!  - Structure Text language "ST".
        //--------------------------------------------------------------------------
        static std::string translate(std::string const& code, std::string const& lang);

        //--------------------------------------------------------------------------
        //! \brief Parse a postfix notation into an AST.
        //--------------------------------------------------------------------------
        static std::shared_ptr<BooleanExp> compile(std::string const& code, Net& net, std::string& error);

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
        //! brief Convert a token from the reverse polish notation into the desired
        //! language (currently: C language or Structure Text language). If not found
        //! the token is returned as it.
        //--------------------------------------------------------------------------
        static std::string convert(std::string const& token, std::string const& lang);
    };

private:

    //! \brief Valid or invalid syntax (invalid may also mean not parsed).
    bool m_valid = false;
    //! \brief Abstract syntax tree of the boolean expression (GRAFCET transitivity).
    std::shared_ptr<BooleanExp> m_ast;
    //! \brief Parsing error
    std::string m_error;
};

} // namespace tpne

#endif