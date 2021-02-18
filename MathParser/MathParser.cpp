#include <string>
#include <vector>
#include <utility>
#include <iostream>
#include <istream>
#include <ostream>
#include <iterator>
#include <map>
#include <stack>
#include <sstream>
#include <algorithm>
#include <cmath>

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
        if (id == 1)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool is_left_assoc(const std::string& str)
    {
        int id = assoc_prec.at(str).second;
        if (id == 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool is_op(const std::string& str)
    {
        if (str == "%" || str == "/" || str == "*" || str == "+" || str == "-")
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool if_in(std::vector<std::string> vec, std::string key)
    {
        if (std::find(vec.begin(), vec.end(), key) != vec.end())
            return true;
        else
            return false;
    }

    bool is_func(const std::string& str)
    {
        if (str == "sin" || str == "tan" || str == "acos" || str == "asin" || str == "abs")
        {
            return true;
        }
        else if (str == "atan" || str == "atan2" || str == "cosh" || str == "sinh")
        {
            return true;
        }
        else if (str == "tanh" || str == "acosh" || str == "asinh" || str == "atanh")
        {
            return true;
        }
        else if (str == "exp" || str == "frexp" || str == "ldexp" || str == "log")
        {
            return true;
        }
        else if (str == "log10" || str == "pow" || str == "sqrt" || str == "cbrt")
        {
            return true;
        }
        else if (str == "hypot" || str == "tgamma" || str == "lgamma" || str == "ceil" || str == "floor")
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    //is_num func from https://stackoverflow.com/a/16465826
    bool is_num(const std::string& s)
    {
        return(strspn(s.c_str(), "-.0123456789") == s.size());
    }

    std::string collapse_str_vec(std::vector<std::string> v)
    {
        std::string res;
        for (std::string& s : v)
        {
            res.append(s);
        }
        return res;
    }

    //TODO: implement other funcs (see is_func for list)
    double eval(double d1, double d2, std::string type)
    {
        if (type == "*")
        {
            return d1 * d2;
        }
        else if (type == "+")
        {
            return d1 + d2;
        }
        else if (type == "-")
        {
            return d1 - d2;
        }
        else if (type == "/")
        {
            return d1 / d2;
        }
        else if (type == "^")
        {
            return std::pow(d1, d2);
        }
        else
        {
            throw std::runtime_error("invalid operation passed to func: eval");
        }
    }

    int get_prec(std::string str)
    {
        if (is_func(str))
        {
            return 5;
        }
        else if (is_op(str))
        {
            return assoc_prec.at(str).first;
        }
        else
        {
            throw std::runtime_error("error thrown by func: get_prec, invalid operator/func");
        }
    }

    std::string to_string(double d)
    {
        std::ostringstream strs;
        strs << d;
        return strs.str();
    }

    //tokenize func from https://stackoverflow.com/a/53921
    //spits by whitespace
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

    //convert string in infix notation to string in RPN
    //using dijkstra's shunting yard algorithm 
    std::vector<std::string> s_yard(std::string str, double x_val)
    {
        std::vector<std::string> tokens = tokenize(str);
        std::vector<std::string> output_queue;

        std::stack<std::string> op_stack;

        for (std::string& tok : tokens)
        {
            if (is_num(tok))
            {
                output_queue.push_back(tok);
            }
            else if (is_func(tok))
            {
                op_stack.push(tok);
            }
            else if (is_op(tok))
            {
                while ((is_op(op_stack.top()) && get_prec(op_stack.top()) > get_prec(tok)) || (get_prec(op_stack.top()) == get_prec(tok) && is_left_assoc(tok)))
                {

                }
            }
        }
    }
    
    //tokens is a ref to a vector of tokens, in RPN
    //double x is what x in the expression will be substitied with
    double eval_rpn(const std::vector<std::string>& tokens)
    {
        double result = 0.0;

        std::stack<std::string> stack;
        for (const std::string& tok : tokens)
        {
            if (!is_op(tok))
            {
                stack.push(tok);
            }
            else
            {
                double d2 = std::strtod(stack.top().c_str(), NULL);
                stack.pop();
                //get top 2 elements
                if (!stack.empty())
                {
                    double d1 = std::strtod(stack.top().c_str(), NULL);
                    stack.pop();
                    result = eval(d2, d1, tok);
                    stack.push(to_string(result));
                }
                else
                {
                    if (tok == "-")
                    {
                        result = -(d2);
                    }
                    else
                    {
                        result = d2;
                    }
                }
                stack.push(to_string(result));
            }
        }
        return (double)std::strtod(stack.top().c_str(), NULL);
    }

    double run_parser_and_eval(std::string& input, double x_val)
    {
        std::vector<std::string>rpn = s_yard(input, x_val);
        //std::cout << collapse_str_vec(rpn) << '\n';
        return eval_rpn(rpn);
    }
}


int main()
{
    std::string a = "10 + 7";
    std::vector<std::string>rpn = parser::s_yard(a, 10);
    std::cout << parser::collapse_str_vec(rpn) << '\n';
    return 0;
}