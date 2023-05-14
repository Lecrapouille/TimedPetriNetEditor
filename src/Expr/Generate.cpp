#include <memory>
#include <string>
#include <stack>
#include <map>
#include <vector>
#include <iostream>

static bool isOperator(std::string const& token)
{
    static const std::vector<std::string> operators = {
        ".", "+"
    };

    for (auto const& it: operators)
    {
        if (token == it)
            return true;
    }
    return false;
}

static std::string translate(std::string const& token, std::string const& lang)
{
    // <Forth symbole> -> map(<destination language>, <destination symbole>)
    static std::map<std::string, std::map<std::string, std::string>> translations = {
        { ".", { { "C", "&" }, { "ST", "AND" } } },
        { "+", { { "C", "|" }, { "ST", "OR" } } },
    };

    // Not token found => return directly
    auto const& it = translations.find(token);
    if (it == translations.end())
        return token;

    // Not language found => return directly
    auto const& itt = (*it).second.find(lang);
    if (itt == (*it).second.end())
        return token;

    return (*itt).second;
}

static std::vector<std::string> tokenizer(std::string const& s, std::string const& delimiter)
{
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
    {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}

static std::string postfixToInfix(std::string const& receptivite, std::string const& lang)
{
    std::stack<std::string> exprs;
    std::vector<std::string> tokens = tokenizer(receptivite, " ");

    for (auto const& it: tokens)
    {
        if (isOperator(it))
        {
            if (exprs.size() < 2u)
            {
                throw std::string("Bad expression");
            }

            std::string operand1 = exprs.top();
            exprs.pop();

            std::string operand2 = exprs.top();
            exprs.pop();

            exprs.push("(" + operand2 + " " + translate(it, lang) + " " + operand1 + ")");
        }
        else
        {
            exprs.push(it);
        }
    }
    return exprs.top();
}

class Context;

class BooleanExp
{
public:

    virtual ~BooleanExp() = default;
    virtual bool evaluate(Context const&) const = 0;
};

class Context
{
public:

    bool lookup(std::string const& key) const { return m.at(key); }
    void assign(std::string const& key, bool const value) { m[key] = value; }

private:

    std::map<const std::string, bool> m;
};

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

static std::shared_ptr<BooleanExp> ast(std::string const& receptivite)
{
    std::stack<std::shared_ptr<BooleanExp>> exprs;
    std::vector<std::string> tokens = tokenizer(receptivite, " ");

    for (auto const& it: tokens)
    {
        if (isOperator(it))
        {
            if (exprs.size() < 2u)
            {
                throw std::string("Bad expression");
            }

            std::shared_ptr<BooleanExp> operand1 = exprs.top();
            exprs.pop();

            std::shared_ptr<BooleanExp> operand2 = exprs.top();
            exprs.pop();

            if (it == ".")
                exprs.push(std::make_shared<AndExp>(operand1, operand2));
            if (it == "+")
                exprs.push(std::make_shared<OrExp>(operand1, operand2));
        }
        else
        {
            exprs.push(std::make_shared<VariableExp>(it));
        }
    }
    return exprs.top();
}

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
