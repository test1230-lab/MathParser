#include <istream>
#include <string>
#include <vector>
#include <utility>
#include <iostream>
#include <ostream>
#include <iterator>
#include <map>
#include <stack>
#include <sstream>
#include <numbers>
#include <algorithm>
#include <cmath>

#define M_PI 3.14159274101257324219

//should i do using namespace std?

namespace parser
{
    const int LEFT_ASSOC = 0;
    const int RIGHT_ASSOC = 1;

    //pair is <prec, assoc_id>
    const std::map<std::string, std::pair<int, int>> assoc_prec{ {"^", std::make_pair(4, RIGHT_ASSOC)},
                                                                 {"*", std::make_pair(3, LEFT_ASSOC)},
                                                                 {"/", std::make_pair(3, LEFT_ASSOC)},
                                                                 {"+", std::make_pair(2, LEFT_ASSOC)},
                                                                 {"-", std::make_pair(2, LEFT_ASSOC)} };

    bool is_right_assoc(const std::string& str)
    {
        int id = assoc_prec.at(str).second;
        if (id == 1) return true;
        else return false;
    }

    bool is_left_assoc(const std::string& str)
    {
        int id = assoc_prec.at(str).second;
        if (id == 0) return true;
        else return false;
    }

    bool is_binary_op(const std::string& str)
    {
        if (str == "%" || str == "/" || str == "*" || str == "+" || str == "-")
        {
            return true;
        }
        else return false;
    }

    bool if_in(std::vector<std::string> vec, std::string key)
    {
        if (std::find(vec.begin(), vec.end(), key) != vec.end()) return true;
        else return false;
    }

    bool is_func(const std::string& str)
    {
        if (str == "sin" || str == "tan" || str == "acos" || str == "asin" || str == "abs")
        {
            return true;
        }
        else if (str == "atan" || str == "cosh" || str == "sinh")
        {
            return true;
        }
        else if (str == "tanh" || str == "acosh" || str == "asinh" || str == "atanh")
        {
            return true;
        }
        else if (str == "exp" || str == "ldexp" || str == "log")
        {
            return true;
        }
        else if (str == "log10" ||  str == "sqrt" || str == "cbrt")
        {
            return true;
        }
        else if (str == "tgamma" || str == "lgamma" || str == "ceil" || str == "floor")
        {
            return true;
        }
        else return false;
    }

    //checks is str is number, checks if it is negitive
    //handls decimal numbers
    bool is_num(const std::string& s)
    {
        int num_found_periods = 0;
        int i = 0;
        if (s[0] == '-')
        {
            i = 1; //ignore the input number's sign
        }
        if (s[0] == '-' && s.size() < 2)
        {
            return 0;
        }
        for (; i < s.size(); i++)
        {
            if (s[i] == '.')
            {
                num_found_periods++;
                if (num_found_periods > 1)
                {
                    return false;
                }
            }
            if (!isdigit(s[i]) && s[i] != '.')
            {
                return false;
            }
        }
        return true;
    }

    std::string to_lower(std::string& str)
    {
        std::string res;
        for (char s : str)
        {
            res.push_back(std::tolower(s));
        }
        return res;
    }

    int get_prec(const std::string str)
    {
        if (is_func(str))
        {
            return 1; //TODO: check it this is the correct value
        }
        else if (is_binary_op(str))
        {
            return assoc_prec.at(str).first;
        }
        else
        {
            std::cout << "invalid operator/func\n";
            exit(-1);
        }
    }

    std::string to_string(double d)
    {
        std::ostringstream strs;
        strs << d;
        return strs.str();
    }

    //tokenize func from https://stackoverflow.com/a/53921
    //spits using regex
    std::vector<std::string> tokenize(const std::string str)
    {
        // construct a stream from the string
        std::stringstream strstr(str);

        // use stream iterators to copy the stream to the vector as whitespace separated strings
        std::istream_iterator<std::string> it(strstr);
        std::istream_iterator<std::string> end;
        std::vector<std::string> results(it, end);

        return results;
    }

    //params: 
    //str - string to be converted
    //var_name - any occurances of this as a seperate token will be treated as a varable
    //convert string in infix notation to a string in Reverse Polish Notation
    //using dijkstra's shunting yard algorithm 
    std::vector<std::string> s_yard(std::string str, std::string var_name)
    {
        std::vector<std::string> tokens = tokenize(str);
        std::vector<std::string> output_queue;
        std::stack<std::string> op_stack;

        for (std::string& tok : tokens)
        {
            if (to_lower(tok) == "pi")
            {
                output_queue.push_back(to_string(M_PI));
            }
            else if (tok == var_name)
            {
                output_queue.push_back(tok);
            }
            else if (is_num(tok))
            {
                output_queue.push_back(tok);
            }
            else if (is_func(tok))
            {
                op_stack.push(tok);
            }
            else if (is_binary_op(tok))
            {   
                /*
                    While loop:
                    while there are elements in the stack
                    while ((there is an operator at the top of the operator stack)
                    and ((the operator at the top of the operator stack has greater precedence)
                    or (the operator at the top of the operator stack has equal precedence and the token is left associative))
                    and (the operator at the top of the operator stack is not a left parenthesis))
                */
                while (!op_stack.empty() && \
                      (is_binary_op(op_stack.top()) && get_prec(op_stack.top()) > (get_prec(tok)) || \
                      (get_prec(op_stack.top()) == get_prec(tok) && is_left_assoc(tok))) && \
                      (op_stack.top() != "("))
                {
                    //pop operators from stack to queue
                    while (!op_stack.empty())
                    {
                        output_queue.push_back(op_stack.top());
                        op_stack.pop();
                    }
                }
                op_stack.push(tok);
            }
            else if (tok == "(")
            {
                op_stack.push(tok);
            }
            else if (tok == ")")
            {
                while (op_stack.top() != "(")
                {
                    output_queue.push_back(op_stack.top());
                    op_stack.pop();
                }
                if (op_stack.top() == "(")
                {
                    op_stack.pop();
                }
                if (is_func(op_stack.top()))
                {
                    output_queue.push_back(op_stack.top());
                    op_stack.pop();
                }
            }
        }
        //all tokens read
        while (!op_stack.empty())
        {
            //there are mismatched parentheses
            if (op_stack.top() == "(" || op_stack.top() == ")")
            {
                std::cout << "mismatched parentheses\n";
                exit(-1);
            }
            output_queue.push_back(op_stack.top());
            op_stack.pop();
        }
        return output_queue;
    }
    
    double compute_binary_ops(double d1, double d2, const std::string op)
    {
        if (op == "*")
        {
            return d1 * d2;
        }
        else if (op == "+")
        {
            return d1 + d2;
        }
        else if (op == "-")
        {
            return d1 - d2;
        }
        else if (op == "/")
        {
            return d1 / d2;
        }
        else if (op == "^")
        {
            return std::pow(d1, d2);
        }
        else
        {
            std::cout << "invalid operation passed to func: eval\n";
            exit(-1);
        }
    }

    double compute_unary_ops(double d, const std::string op)
    {
        if (op == "sin") return sin(d);
        else if (op == "sqrt") return sqrt(d);
        else if (op == "abs") return abs(d);
        else if (op == "tan") return tan(d);
        else if (op == "acos") return acos(d);
        else if (op == "asin") return asin(d);
        else if (op == "abs") return abs(d);
        else if (op == "atan") return atan(d);
        else if (op == "log") return log(d);
        else if (op == "log10") return log10(d);
        else if (op == "cosh") return cosh(d);
        else if (op == "sinh") return sinh(d);
        else if (op == "tanh") return tanh(d);
        else if (op == "exp") return exp(d);
        else if (op == "cbrt") return cbrt(d);
        else if (op == "tgamma") return tgamma(d);
        else if (op == "lgamma") return lgamma(d);
        else if (op == "ceil") return ceil(d);
        else if (op == "floor") return floor(d);
        else if (op == "acosh") return acosh(d);
        else if (op == "asinh") return asinh(d);
        else if (op == "atanh") return atanh(d);
        else
        {
            std::cout << "invalid unary op / function\n";
            exit(-1);
        }
    }

    /*
    double eval_rpn(const std::vector<std::string>& tokens, std::string var_name, double var_value)
    {     
        std::stack<std::string> stack;
        for (const std::string& tok : tokens)
        {
            std::cout << "stack size: " << stack.size() << '\n';
            //reset d0,d1 after each iteration
            double d0 = 0.0;
            double d1 = 0.0;
            if (tok == var_name)
            {
                stack.push(to_string(var_value));
            }
            else if (is_num(tok))
            {
                stack.push(tok);
            }
            //if binary operator apply that operator to the top-most two entries on the stack
            //pop those two entries and push the result.
            else if (is_binary_op)
            {
                d0 = std::stod(stack.top());
                stack.pop();
                if (!stack.empty())
                {
                    d1 = std::stod(stack.top());
                    stack.pop();
                    double res = compute_binary_ops(d0, d1, tok);
                    stack.push(to_string(res));
                }            
            }
            else if (is_func)
            {
                d0 = std::stod(stack.top());
                stack.pop();
                double res = compute_unary_ops(d0, tok);
                stack.push(to_string(res));
            }
        }
        
        return std::stod(stack.top());
    }
    */

    double eval_rpn(const std::vector<std::string>& tokens, std::string var_name, double var_value)
    {     
        std::stack<std::string> stack;
        for (const std::string& tok : tokens)
        {
            
            if (tok == var_name)
            {
                stack.push(to_string(var_value));
            }
            else if (is_num(tok))
            {
                std::cout << "token: " << tok << " " << "stack size: " << stack.size() << '\n';
                stack.push(tok);
            }
            //handle binary operaters
            else if(is_binary_op(tok))
            {
                double res = 0.0;
                const double d2 = std::stod(stack.top());
                stack.pop();
                if (!stack.empty())
                {
                    
                    const double d1 = std::stod(stack.top());
                    stack.pop();
                    res = compute_binary_ops(d1, d2, tok);
                    stack.push(to_string(res));
                }
            }
            //handle funcs(unary ops)
            else if (is_func(tok))
            {
                if (!stack.empty())
                {
                    double res = 0.0;
                    const double d1 = std::stod(stack.top());
                    stack.pop();
                    res = compute_unary_ops(d1, tok);
                    stack.push(to_string(res));
                }
            }
            else
            {
                double res = 0.0; //is this ok
                stack.push(to_string(res));
            }

        }
        return std::stod(stack.top());
    }
}


int main()
{
    std::string a = "sin ( 10 ) + 7 - 8 + 5 + 3";
    std::vector<std::string>rpn = parser::s_yard(a, "x");
    double res = parser::eval_rpn(rpn, "x", 10);
    std::cout << res << '\n';
    /*
    for (auto& a : rpn)
    {
        std::cout << a << '\n';
    }
    */
    return 0;
}