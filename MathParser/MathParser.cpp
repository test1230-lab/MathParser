#include <iostream>
#include <unordered_map>
#include <functional>
#include <string_view>
#include <algorithm>
#include <limits>
#include <string>
#include <vector>
#include <utility>
#include <stack>
#include <ranges>
#include <numbers>
#include <regex>
#include <variant>
#include <chrono>
#include <cmath>
#define SDL_MAIN_HANDLED
#include <SDL.h>

constexpr uint32_t white = 0xFFFFFFFF;
constexpr uint32_t black = 0x00000000;
constexpr uint32_t green = 0xFF007F00;
constexpr uint32_t red = 0xFFBB0000;
constexpr uint32_t yellow = 0xFFFFFF00;
constexpr uint32_t bright_red = 0xFFFF0000;
constexpr uint32_t blue = 0xFF0000FF;

constexpr int screen_w = 640;
constexpr int screen_h = 640;
constexpr int grid_spacing = 32;
constexpr const char* win_title = "Graphing Calc";

struct pt_2d
{
    int x, y;
};

namespace parser
{
    enum class assoc
    {
        LEFT,
        RIGHT
    };

    //pair is <prec, assoc_id>
    static std::unordered_map<std::string_view, std::pair<int, assoc>> assoc_prec{
        {"^", {4, assoc::RIGHT}},
        {"*", {3, assoc::LEFT}},
        {"/", {3, assoc::LEFT}},
        {"+", {2, assoc::LEFT}},
        {"-", {2, assoc::LEFT}} };

    static std::unordered_map<std::string_view, double(*)(double)> unary_func_tbl{ 
        {"sin", std::sin},
        {"cos", std::cos},
        {"sqrt", std::sqrt},
        {"abs", std::fabs},
        {"tan", std::tan},
        {"acos", std::acos},
        {"asin", std::asin},
        {"atan", std::atan},
        {"log", std::log},
        {"log10", std::log10},
        {"cosh", std::cosh},
        {"sinh", std::sinh},
        {"tanh", std::tanh},
        {"exp", std::exp},
        {"cbrt", std::cbrt},
        {"tgamma", std::tgamma},
        {"lgamma", std::lgamma},
        {"ceil", std::ceil},
        {"floor", std::floor},
        {"acosh", std::acosh},
        {"asinh", std::asinh},
        {"trunc", std::trunc},
        {"atanh", std::atanh} };

    bool is_left_assoc(std::string_view str)
    {
        return assoc_prec[str].second == assoc::LEFT;
    }

    bool is_binary_op(std::string_view str)
    {
        return (str == "/" || str == "*" || str == "+" || str == "-" || str == "^");
    }

    bool is_func(std::string_view str)
    {
        return unary_func_tbl.contains(str);
    }

    //checks if the str is a number, handles decimal numbers
    bool is_num(std::string_view str)
    {
        int num_found_periods = 0;
        for (const auto c : str)
        {
            if (c == '.')
            {
                ++num_found_periods;
                if (num_found_periods > 1)
                {
                    return false;
                }
            }
            if (!std::isdigit(c) && c != '.')
            {
                return false;
            }
        }
        return true;
    }

    //checks if the number of ")" and "(" are the same
    //this is better than the std algo because it
    //only walks the str once
    bool check_num_parentheses(std::string_view str)
    {
        int left_count = 0;
        int right_count = 0;
        for (const auto c : str)
        {
            if (c == '(')
            {
                ++left_count;
            }
            else if (c == ')')
            {
                ++right_count;
            }
        }
        return left_count == right_count;
    }

    class parse_error : public std::runtime_error 
    {
        public:
            explicit parse_error(const std::string& what) : std::runtime_error(what) {}
            explicit parse_error(const char* what) : std::runtime_error(what) {}
    };

    int get_prec(std::string_view str)
    {
        if (is_func(str)) { return 1; } //TODO: check it this is the correct value
        else if (is_binary_op(str)) { return assoc_prec[str].first; }
        else { return 0; }
    }

    //create regex string partially from func table
    static const std::string get_regex(std::string var_name) 
    {
        auto func_names{ std::views::keys(unary_func_tbl) };
        std::ostringstream os;
        os << "(";
        for (const auto& name : std::views::keys(unary_func_tbl)) 
        {
            os << name << "|";
        }
        os << var_name << "|"
           << R"(e|pi)|)"
           << R"(([0-9]+[.])?[0-9]+|[-+\()/*^])";
        return os.str();
    }

    //passing by value so we can modify the copy
    //tokenize using regex
    std::vector<std::string> tokenize(std::string str, std::string var_name)
    {
        if (!check_num_parentheses(str))
        {
            throw parse_error("parenthesis issue");
        }

        std::vector<std::string> res;
        std::string test;
        const std::regex words_regex(get_regex(var_name), std::regex_constants::egrep);
         
        auto words_begin = std::sregex_iterator(str.begin(), str.end(), words_regex);
        auto words_end = std::sregex_iterator();
        for (std::sregex_iterator i = words_begin; i != words_end; ++i) 
        {
            res.push_back((*i).str());
            test += (*i).str();
        }
        //this will find all unknown tokens, therefor the rest of 
        //the parser will assume all tokens are valid
        std::erase(str, ' ');
        if (test != str)
        {
            throw parse_error("unknown token in input"); 
        }
        return res;
    }

    //params: 
    //str - string to be converted
    //var_name - any occurances of this as a seperate token will be treated as a varable
    //convert string in infix notation to a string in Reverse Polish Notation
    //using dijkstra's shunting yard algorithm 
    std::vector<std::variant<double, std::string>> s_yard(const std::string& str, std::string var_name)
    {
        std::vector<std::variant<double, std::string>> output_queue;
        std::stack<std::string> op_stack;

        //tokenize function should handle all bad tokens
        for (const auto& tok : tokenize(str, var_name)) 
        {
            if (tok == "pi")
            {
                output_queue.push_back(std::numbers::pi);
            }
            else if (tok == "e")
            {
                output_queue.push_back(std::numbers::e_v<double>);
            }
            else if (tok == var_name)
            {
                output_queue.push_back(tok);
            }
            else if (is_num(tok))
            {
                output_queue.push_back(strtod(tok.c_str(), nullptr));
            }
            else if (is_func(tok))
            {
                op_stack.push(tok);
            }
            else if (is_binary_op(tok))
            {   
                while (!op_stack.empty() &&
                      (is_binary_op(op_stack.top()) && get_prec(op_stack.top()) > get_prec(tok) ||
                      (get_prec(op_stack.top()) == get_prec(tok) && is_left_assoc(tok))) &&
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
            else
            {
                throw parse_error("unknown token: " + tok);
            }
        }
        //all tokens read
        while (!op_stack.empty())
        {
            //there are mismatched parentheses
            if (op_stack.top() == "(" || op_stack.top() == ")")
            {
                throw parse_error("mismatched parentheses");
            }
            output_queue.push_back(op_stack.top());
            op_stack.pop();
        }
        return output_queue;
    }
    
    double compute_binary_ops(double d1, double d2, std::string_view op)
    {
        if (op == "*") return d1 * d2;
        else if (op == "+") return d1 + d2;
        else if (op == "-") return d1 - d2;
        else if (op == "/") return d1 / d2;
        else if (op == "^") return std::pow(d1, d2);
    }

    //creates function that fully represents the rpn vec that is passed
    //this can be optimized by reducing uneeded functions
    //eg 1 + 1 + 2 could just be 4
    std::function<double(double)> build_func(const std::vector<std::variant<double, std::string>>& tokens, std::string var_name)
    {
        std::stack<std::function<double(double)>> stack;
        for (const auto& tok : tokens)
        {
            if (const double* num_ptr = std::get_if<double>(&tok))
            {
                stack.push([number = *num_ptr](double) {return number;});
            }
            else if (std::get<std::string>(tok) == var_name)
            {
                stack.push([](double var_value) {return var_value;});
            }
            else if (is_binary_op(std::get<std::string>(tok)))
            {
                auto right = stack.top();
                stack.pop();
                if (!stack.empty())
                {
                    auto left = stack.top();
                    stack.pop();
                    auto op = std::get<std::string>(tok);
                    stack.push([left, right, op](double var_value) {return compute_binary_ops(left(var_value), right(var_value), op);});
                }
                //if unary minus
                else if(std::get<std::string>(tok) == "-")
                {
                    stack.push([right](double var_value) {return -(right(var_value));});
                }         
            }
            else if (is_func(std::get<std::string>(tok)))
            {
                auto operand = stack.top();
                stack.pop();
                auto func = unary_func_tbl[std::get<std::string>(tok)];
                stack.push([operand, func](double var_value) {return func(operand(var_value));});
            }
        }
        return stack.top();
    }
}


SDL_Window* create_centered_window(uint32_t width, uint32_t height, const char* title)
{
    // Get current device's Display Mode to calculate window position
    SDL_DisplayMode DM;
    SDL_GetCurrentDisplayMode(0, &DM);

    // Calculate where the upper-left corner of a centered window will be
    const int32_t x = DM.w / 2 - width / 2;
    const int32_t y = DM.h / 2 - height / 2;

    // Create the SDL window
    SDL_Window* pWindow = SDL_CreateWindow(title, x, y, screen_w, screen_h,
        SDL_WINDOW_ALLOW_HIGHDPI);

    return pWindow;
}

//free and terminate
[[noreturn]] void shutdown(SDL_Window** ppWindow, SDL_Renderer** ppRenderer, SDL_Texture** ppTexture)
{
    if (ppTexture) { SDL_DestroyTexture(*ppTexture); }
    if (ppRenderer) { SDL_DestroyRenderer(*ppRenderer); }
    if (ppWindow) { SDL_DestroyWindow(*ppWindow); }
    exit(0);
}

class SDL_error : public std::runtime_error
{
public:
    explicit SDL_error(const std::string& what) : std::runtime_error(what) {}
    explicit SDL_error(const char* what) : std::runtime_error(what) {}
};

//from https://stackoverflow.com/a/27030598
template<typename T>
std::vector<double> linspace(T start_in, T end_in, int num_in)
{
    std::vector<double> linspaced;
    linspaced.reserve(num_in);

    double start = static_cast<double>(start_in);
    double end = static_cast<double>(end_in);
    double num = static_cast<double>(num_in);

    if (num == 0) { return linspaced; }
    if (num == 1)
    {
        linspaced.push_back(start);
        return linspaced;
    }

    double delta = (end - start) / (num - 1);

    for (int i = 0; i < num - 1; ++i)
    {
        linspaced.push_back(start + delta * i);
    }
    linspaced.push_back(end); // I want to ensure that start and end
                              // are exactly the same as the input
    return linspaced;
}

double dist_2d(pt_2d a, pt_2d b)
{
    return std::sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
}

//creates the grid and axis
//uses global constants to deduce array size
void create_canvas(uint32_t *data)
{
    //create grid  
    for (int x = grid_spacing; x < screen_w; x += grid_spacing)
    {
        for (int y = 0; y < screen_h; y++)
        {
            data[x + (y * screen_w)] = green;
        }
    }
   
    for (int x = 0; x < screen_w; x++)
    {
        for (int y = grid_spacing; y < screen_h; y += grid_spacing)
        {
            data[x + (y * screen_w)] = green;
        }
    }

    //draw y axis
    for (int y = 0; y < screen_h; y++)
    {
        data[screen_w/2 + (y * screen_w)] = red;
    }
    //draw x axis
    for (int x = 0; x < screen_h; x++)
    {
        data[x + (screen_h/2 * screen_w)] = red;
    }   
}

//fills the gap between two points with a line
void fill_gaps(uint32_t* data, pt_2d a, pt_2d b, int max)
{
    const double dist = dist_2d(a, b);
    if (dist > 2 && dist < max)
    {
        std::vector<double> xvec = linspace(a.x, b.x, std::round(dist) + 1);
        std::vector<double> yvec = linspace(a.y, b.y, std::round(dist) + 1);
        for (int i = 0; i < xvec.size(); i++)
        {
            int ix = std::round(xvec[i]);
            int iy = std::round(yvec[i]);
            if (ix < screen_w - 1 && iy < screen_h - 1 && ix > 0 && iy > 0)
            {
                data[ix + (iy * screen_w)] = yellow;
            }
        }
    }
}

//plot the function across the range lower to upper
void plot(uint32_t* data, int range_lower, int range_upper, std::function<double(double)> func, std::string_view var_name, float pt_step_count, int max)
{
    const int ratio = screen_w / range_upper;
    int lst_ix = 0;
    int lst_iy = 0;
    
    for (int x = (range_lower * pt_step_count); x < (range_upper * pt_step_count); x++)
    {
        double tx = x / pt_step_count;       
        double ty = func(tx);

        tx *= (screen_w / static_cast<double>(range_upper));
        ty *= (screen_w / static_cast<double>(range_upper));

        int ix = -std::round(tx / 2.0);
        int iy = std::round(ty / 2.0);

        ix += screen_w / 2;
        iy += screen_h / 2;

        if (ix < screen_w - 1 && iy < screen_h - 1 && ix > 0 && iy > 0)
        {
            data[ix + (iy * screen_w)] = yellow;
        }
        
        fill_gaps(data, { lst_ix, lst_iy }, { ix, iy }, max);
        lst_ix = ix;
        lst_iy = iy;
    }
}

/* 
  Renders Pixel Buffer
  params:
  sdl window
  sdl renderer
  sdl texture
  uint32 1d pixel buffer
*/
void render(SDL_Window* pWindow, SDL_Renderer* pRenderer, SDL_Texture* pTexture, uint32_t* data)
{
    int32_t pitch = 0;
    uint32_t* pPixelBuffer = nullptr;
    if (!SDL_LockTexture(pTexture, nullptr, (void**)(&pPixelBuffer), &pitch))
    {
        pitch /= sizeof(uint32_t);
        memcpy(pPixelBuffer, data, screen_h * static_cast<size_t>(pitch) * sizeof(uint32_t));
        SDL_UnlockTexture(pTexture);
        SDL_RenderCopy(pRenderer, pTexture, nullptr, nullptr);
        SDL_RenderPresent(pRenderer);
    }
}

bool equality(double a, double b)
{
    return (std::abs(a - b) < std::numeric_limits<double>::epsilon());
}

void tests()
{
    const std::string a = "x+2+5 + 6 + 10";
    const std::string b = "cos( sin(tan(x)) )";
    const std::string c = "x+ 10 *(5 +2)";
    const std::string d = "-x + 1 - (3 + 2)";

    double real = 1.0 + 2.0 + 5.0 + 6.0 + 10.0;
    auto rpn = parser::s_yard(a, "x");
    auto func = parser::build_func(rpn, "x");
    if (!equality(real, func(1.0))) 
        std::cout << "test on: " << a << " failed... " << real << " " << func(1.0) << '\n'; 
    else
        std::cout << "test on: " << a << " passed... " << real << " " << func(1.0) << '\n';

    real = std::cos(std::sin(std::tan(1.0)));
    rpn = parser::s_yard(b, "x");
    func = parser::build_func(rpn, "x");
    if (!equality(real, func(1.0))) 
        std::cout << "test on: " << b << " failed... " << real << " " << func(1.0) << '\n'; 
    else 
        std::cout << "test on: " << b << " passed... " << real << " " << func(1.0) << '\n';

    real = 1.0 + 10.0 * (5.0 + 2.0);
    rpn = parser::s_yard(c, "x");
    func = parser::build_func(rpn, "x");
    if (!equality(real, func(1.0))) 
        std::cout << "test on: " << c << " failed... " << real << " " << func(1.0) << '\n'; 
    else  
        std::cout << "test on: " << c << " passed... " << real << " " << func(1.0) << '\n'; 

    real = -1.0 + 1.0 - (3.0 + 2.0);
    rpn = parser::s_yard(d, "x");
    func = parser::build_func(rpn, "x"); 
    if (!equality(real, func(1.0)))
        std::cout << "test on: " << d << " failed... " << real << " " << func(1.0) << '\n'; 
    else  
        std::cout << "test on: " << d << " passed... " << real << " " << func(1.0) << '\n'; 

    //testing all funcs from the table, mostly uneeded
    for (const auto& [k, v] : parser::unary_func_tbl)
    {
        double test_val = 1.0;
        //make sure the function is well defined at the input value
        if (k == "atanh")
        {
            test_val = -0.5;
        }
      
        const std::string s = static_cast<std::string>(k) + "(x)";
        real = v(test_val);
        rpn = parser::s_yard(s, "x");
        func = parser::build_func(rpn, "x");
        if (!equality(real, func(test_val)))
            std::cout << "test on: " << s << " failed... " << real << " " << func(test_val) << '\n';
        else 
            std::cout << "test on " << s << " passed... " << real << " " << func(test_val) << '\n'; 
    }

    for (const auto& [k, v] : parser::unary_func_tbl)
    {
        double test_val = 1.0;
        
        if (k == "atanh")
        {
            test_val = -0.5;
        }
        else if (k == "acosh")
        {
            test_val = 2.0;
        }
        const std::string s = static_cast<std::string>(k) + "(x-0.001*(2 - 1))";
        real = v(test_val - 0.001 * (2 - 1));
        rpn = parser::s_yard(s, "x");
        func = parser::build_func(rpn, "x");
        if (!equality(real, func(test_val)))
            std::cout << "test on: " << s << " failed... " << real << " " << func(test_val) << '\n';
        else 
            std::cout << "test on " << s << " passed... " << real << " " << func(test_val) << '\n'; 
    }
}

int main()
{
    //tests();
    bool clr_ln = false;
    const std::string var_name = "x";
    int range_upper = 5;
    int range_lower = -5;
    int max = 125;
    float pt_step_count = 500;

    SDL_Event e;
    SDL_Window* pWindow = nullptr;
    SDL_Renderer* pRenderer = nullptr;
    SDL_Texture* pTexture = nullptr;

    std::string in_txt;
    std::vector<std::function<double(double)>> eqs_on_graph;
    std::vector<std::string> eqs; //store input strings, so we can check for dupes

    //the exceptions throw if a sdl func fails will terminate the program
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { throw SDL_error("SDL Failed to Init"); }

    pWindow = create_centered_window(screen_w, screen_h, win_title);
    if (!pWindow) { throw SDL_error("SDL window creation failed"); }

    pRenderer = SDL_CreateRenderer(pWindow, -1, SDL_RENDERER_ACCELERATED);
    if (!pRenderer) { throw SDL_error("SDL renderer creation failed"); }

    pTexture = SDL_CreateTexture(pRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, screen_w, screen_h);
    if (!pTexture) { throw SDL_error("SDL texture creation failed"); }

    uint32_t* data = new uint32_t[screen_w * screen_h];

    SDL_StartTextInput();
    create_canvas(data);
    render(pWindow, pRenderer, pTexture, data);

    std::cout << "zoom out with arrow down, zoom in with arrow up, reset with del, exit with esc\n";

    for (;;)
    {
        in_txt.clear();
        while (SDL_WaitEvent(&e))
        {       
            if (e.type == SDL_QUIT)
            {
                delete[] data;
                shutdown(&pWindow, &pRenderer, &pTexture);          
            }
            else if (e.type == SDL_TEXTINPUT)
            {
                if (clr_ln)
                {
                    std::cout << std::string(22, ' ') << '\r';
                }
                in_txt.append(e.text.text);
                std::cout << in_txt << '\r';
                clr_ln = false;
            }
            else if (e.type == SDL_KEYDOWN)
            {
                if (e.key.keysym.sym == SDLK_ESCAPE)
                {
                    delete[] data;
                    shutdown(&pWindow, &pRenderer, &pTexture);
                }
                //reset graph(default zoom, other params, clear)
                else if (e.key.keysym.sym == SDLK_DELETE)
                {
                    eqs.clear();
                    eqs_on_graph.clear();
                    std::cout << "\033[2J" << "\033[1;1H";
                    memset(data, black, screen_w * screen_h * sizeof(uint32_t));
                    create_canvas(data);
                    render(pWindow, pRenderer, pTexture, data);
                    range_upper = 5;
                    range_lower = -5;
                    max = 125;
                    pt_step_count = 1000;
                    std::cout << "range: " << range_lower << " to: " << range_upper << '\r';
                    continue;
                }
                //zoom in
                else if (e.key.keysym.sym == SDLK_UP)
                {
                    if ((range_upper - 1) > 0 && (range_lower + 1) < 0)
                    {
                        ++range_lower;
                        --range_upper;
                    }
                    if (pt_step_count <= 200)
                    {
                        pt_step_count = 200;
                    }
                    --pt_step_count;
                    --max;
                    
                    if (max <= 200)
                    {
                        max = 200;
                    }
                    if (!eqs_on_graph.empty())
                    {
                        memset(data, black, screen_w * screen_h * sizeof(uint32_t));
                        create_canvas(data);
                        for (const auto& func : eqs_on_graph)
                        {
                            plot(data, range_lower, range_upper, func, var_name, pt_step_count, max);
                        }
                        render(pWindow, pRenderer, pTexture, data);
                    }
                    std::cout << "range: " << range_lower << " to: " << range_upper << "    " <<'\r';
                    clr_ln = true;
                    continue;
                }
                //zoom out
                else if (e.key.keysym.sym == SDLK_DOWN)
                {
                    --range_lower;
                    ++range_upper;
                    ++max;
                    ++pt_step_count;
                    
                    if (pt_step_count >= 750)
                    {
                        pt_step_count = 750;
                    }
                    if (max >= 500)
                    {
                        max = 500;
                    }
                    if (!eqs_on_graph.empty())
                    {
                        memset(data, black, screen_w * screen_h * sizeof(uint32_t));
                        create_canvas(data);
                        for (const auto& func : eqs_on_graph)
                        {
                            plot(data, range_lower, range_upper, func, var_name, pt_step_count, max);
                        }
                        render(pWindow, pRenderer, pTexture, data);
                    }
                    std::cout << "range: " << range_lower << " to: " << range_upper << "    " << '\r';
                    clr_ln = true;
                    continue;
                }
                //delete last char from in_txt
                else if (e.key.keysym.sym == SDLK_BACKSPACE)
                {
                    if (!in_txt.empty())
                    {
                        in_txt.pop_back();
                        std::cout << in_txt << ' ' << '\r';
                    }
                }
            }
            else if (e.type == SDL_KEYUP)
            {
                if (e.key.keysym.sym == SDLK_RETURN)
                {
                    std::cout << '\n';
                    break;
                }
            }             
        }

        if (!in_txt.empty())
        {
            try
            {
                auto rpn = parser::s_yard(in_txt, var_name); //will throw if there is bad input
                auto func = parser::build_func(rpn, var_name);
                
                //if eq is not already on graph
                if (std::find(eqs.begin(), eqs.end(), in_txt) == eqs.end()) 
                {                  
                    plot(data, range_lower, range_upper, func, var_name, pt_step_count, max);
                    render(pWindow, pRenderer, pTexture, data);
                    eqs_on_graph.push_back(func);
                    eqs.push_back(in_txt);
                }      
            }
            catch (const parser::parse_error& exc)
            {
                std::cerr << exc.what() << '\n';
            }            
        }
    }
}