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
#include <algorithm>
#include <cmath>
#include <SDL.h>
#include <SDL_image.h>
#undef main

constexpr uint32_t white = 0xFFFFFFFF;
constexpr uint32_t black = 0x00000000;
constexpr uint32_t green = 0xFF007F00;
constexpr uint32_t red = 0xFFBB0000;
constexpr uint32_t yellow = 0xFFFFFF00;
constexpr uint32_t b_red = 0xFFFF0000;
constexpr uint32_t blue = 0xFF0000FF;

constexpr double PI = 3.14159274;
constexpr int screen_w = 640;
constexpr int screen_h = 640;
constexpr int channels = 3;
constexpr int grid_spacing = 32;
constexpr const char* win_title = "Graphing Calc";
int range_upper = 5;
int range_lower = -5;
constexpr float pt_step_count = 720;

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

    double clip(double n, double lower, double upper)
    {
        return std::max(lower, std::min(n, upper));
    }

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

    bool is_func(const std::string& str)
    {
        if (str == "sin" || str == "tan" || str == "acos" || str == "asin" || str == "abs")
        {
            return true;
        }
        else if (str == "atan" || str == "cosh" || str == "sinh" || str == "cos")
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

    //handls decimal numbers
    bool is_num(const std::string& s)
    {
        int num_found_periods = 0;
        for (int i = 0; i < s.size(); i++)
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

    std::string to_lower(const std::string& str)
    {
        std::string res;
        for (const char &s : str)
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
            return 0;
        }
    }

    std::string to_string(double d)
    {
        std::ostringstream strs;
        strs << d;
        return strs.str();
    }

    //tokenize func from https://stackoverflow.com/a/53921
    //spits using whitespace
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
                output_queue.push_back(to_string(PI));
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
            return std::pow(d1, d2); //TODO: FIX DOING x ^ 2 RESULTS IN CONSTANT Y VALUE
        }
        else
        {
            std::cout << R"(invalid operator: ")" << op << R"("  passed to func "compute_binary_ops")" << '\n';
            exit(-1);
        }
    }

    double compute_unary_ops(double d, const std::string op)
    {
        if (op == "sin") return sin(d);
        else if (op == "cos") return cos(d);
        else if (op == "sqrt") return sqrt(d);
        else if (op == "abs") return abs(d);
        else if (op == "tan") return tan(d);
        else if (op == "acos") return acos(d);
        else if (op == "asin") return asin(d);
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
            std::cout << R"(invalid operator/func : ")" << op << R"("  passed to func "compute_unary_ops")" << '\n';
            exit(-1);
        }
    }

    //TODO: negative broken
    double eval_rpn(const std::vector<std::string>& tokens, std::string var_name, double var_value)
    {     
        std::stack<std::string> stack;
        for (const std::string& tok : tokens)
        {
            if (to_lower(tok) == "pi")
            {
                stack.push(to_string(PI));
            }
            else if (tok == var_name)
            {
                stack.push(to_string(var_value));
            }
            else if (is_num(tok))
            {
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
                    const double d1 = std::stod(stack.top());
                    stack.pop();
                    double res = compute_unary_ops(d1, tok);
                    stack.push(to_string(res));
                }
            }
            else
            {
                continue;
            }
        }
        return std::stod(stack.top());
    }
}

namespace disp
{
    constexpr int32_t g_kRenderDeviceFlags = -1;
    constexpr  int32_t g_kErrorOccurred = -1;

    int32_t e(int32_t result, std::string errorMessage)
    {
        if (result)
            std::cout << errorMessage;

        return result;
    }

    SDL_Window* CreateCenteredWindow(uint32_t width, uint32_t height, std::string title)
    {
        // Get current device's Display Mode to calculate window position
        SDL_DisplayMode DM;
        SDL_GetCurrentDisplayMode(0, &DM);

        // Calculate where the upper-left corner of a centered window will be
        const int32_t x = DM.w / 2 - width / 2;
        const int32_t y = DM.h / 2 - height / 2;

        // Create the SDL window
        SDL_Window* pWindow = SDL_CreateWindow(win_title, x, y, screen_w, screen_h,
            SDL_WINDOW_ALLOW_HIGHDPI);

        if (e(!pWindow, "Failed to create Window\n"));

        return pWindow;
    }

    // Create SDL renderer and configure whether or not to use Hardware Acceleration
    SDL_Renderer* CreateRenderer(SDL_Window* pWindow, bool hardwareAccelerated)
    {
        if (hardwareAccelerated)
            return SDL_CreateRenderer(pWindow, g_kRenderDeviceFlags, SDL_RENDERER_ACCELERATED);
        else
            return SDL_CreateRenderer(pWindow, g_kRenderDeviceFlags, SDL_RENDERER_SOFTWARE);
    }

    // Create an SDL Texture to use as a "back buffer"
    SDL_Texture* CreateBackBufferTexture(SDL_Renderer* pRenderer)
    {
        SDL_Texture* pTexture = SDL_CreateTexture(pRenderer, SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING, screen_w, screen_h);

        if (e(!pTexture, "Failed to create Back Buffer Texture\n"));

        return pTexture;
    }

    // Free resources 
    void Shutdown(SDL_Window** ppWindow, SDL_Renderer** ppRenderer, SDL_Texture** ppTexture)
    {
        // Free the Back Buffer
        if (ppTexture)
        {
            SDL_DestroyTexture(*ppTexture);
            *ppTexture = nullptr;
        }

        // Free the SDL renderer
        if (ppRenderer)
        {
            SDL_DestroyRenderer(*ppRenderer);
            *ppRenderer = nullptr;
        }

        // Free the SDL window
        if (ppWindow)
        {
            SDL_DestroyWindow(*ppWindow);
            *ppWindow = nullptr;
        }
        exit(-1);
    }

    // Initialize SDL Components 
    int32_t Startup(SDL_Window** ppWindow, SDL_Renderer** ppRenderer, SDL_Texture** ppTexture)
    {
        SDL_Init(SDL_INIT_VIDEO);

        if (e(!ppWindow, "Potiner to Window* was null\n")) return -1;

        *ppWindow = CreateCenteredWindow(screen_w, screen_h, win_title);

        if (e(!*ppWindow, "No Window. Aborting..."))
        {
            Shutdown(ppWindow, ppRenderer, ppTexture);

            return -1;
        }

        if (e(!ppRenderer, "Pointer to Renderer* was null\n")) return -1;

        *ppRenderer = CreateRenderer(*ppWindow, true);

        if (e(!ppRenderer, "No Renderer. Aborting..."))
        {
            Shutdown(ppWindow, ppRenderer, ppTexture);

            return -1;
        }

        if (e(!ppTexture, "Pointer to Texture* was null\n")) return -1;

        *ppTexture = CreateBackBufferTexture(*ppRenderer);

        if (e(!*ppTexture, "No back buffer Texture. Aborting..."))
        {
            Shutdown(ppWindow, ppRenderer, ppTexture);

            return -1;
        }

        return 0;
    }

    // Call this once during each render loop in order to determine when the user wishes to terminate the program
    bool ProcessInput()
    {
        // Return this value to tell the caller whether or not it should continue rendering
        // We will terminate the application if any key is pressed
        bool keepRenderLoopRunning = true;

        // Events are generated by SDL whenever something occurs system-wide
        // We are only interested in keyboard events and when the user closes the window
        // We will terminate the application if a key is pressed or if the window is manually closed
        SDL_Event event;

        // Process all events and return whether or not to quit
        while (SDL_PollEvent(&event))
        {
            // Handle relevant SDL events
            switch (event.type)
            {
                // Terminate application if a key is pressed or if the user closes the window
            case SDL_KEYDOWN:
            case SDL_QUIT:
                keepRenderLoopRunning = false;
            }
        }

        // Let the caller know if it should continue rendering, otherwise terminate
        return keepRenderLoopRunning;
    }


    // Call this within every render loop
    // Fills screen with randomly generated colored pixels
    int32_t Render(SDL_Window* pWindow, SDL_Renderer* pRenderer, SDL_Texture* pTexture, uint32_t* data)
    {
        // The Back Buffer texture may be stored with an extra bit of width (pitch) on the video card in order to properly
        // align it in VRAM should the width not lie on the correct memory boundary (usually four bytes).
        int32_t pitch = 0;

        // This will hold a pointer to the memory position in VRAM where our Back Buffer texture lies
        uint32_t* pPixelBuffer = nullptr;

        // Lock the memory in order to write our Back Buffer image to it
        if (!SDL_LockTexture(pTexture, NULL, (void**)&pPixelBuffer, &pitch))
        {
            // The pitch of the Back Buffer texture in VRAM must be divided by four bytes
            // as it will always be a multiple of four
            pitch /= sizeof(uint32_t);

            memcpy(pPixelBuffer, data, screen_h * static_cast<size_t>(pitch)* sizeof(uint32_t));

            // Unlock the texture in VRAM to let the GPU know we are done writing to it
            SDL_UnlockTexture(pTexture);

            // Copy our texture in VRAM to the display framebuffer in VRAM
            SDL_RenderCopy(pRenderer, pTexture, NULL, NULL);

            // Copy the VRAM framebuffer to the display
            SDL_RenderPresent(pRenderer);

            return 0;
        }
        else
            return g_kErrorOccurred;
    }
}

void create_canvas(uint32_t *data)
{
    //create grid  
    for (int x = grid_spacing; x < screen_w; x += grid_spacing)
    {
        for (int y = 0; y < screen_h; y++)
        {
            //printf("x:%d y:%d\n", x, y);
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

    //y axis
    for (int y = 0; y < screen_h; y++)
    {
        data[screen_w/2 + (y * screen_w)] = red;
    }
    //x axis
    for (int x = 0; x < screen_h; x++)
    {
        data[x + (screen_h/2 * screen_w)] = red;
    }   
}

void plot(uint32_t* data, int range_lower, int range_upper, std::vector<std::string>& rpn, std::string var_name)
{
    const int ratio = screen_w / range_upper;
    for (int x = range_lower * pt_step_count; x < range_upper * pt_step_count; x++)
    {
        double tx = x / pt_step_count;
        double ty = parser::eval_rpn(rpn, var_name, tx);

        tx *= (screen_w / static_cast<double>(range_upper));
        ty *= (screen_w / static_cast<double>(range_upper));

        int ix = round(tx / 2.0);
        int iy = round(ty / 2.0);

        ix += screen_w / 2;
        iy += screen_h / 2;

        if (ix < screen_w - 1 && iy < screen_h - 1 && ix > 0 && iy > 0)
        {
            data[ix + (iy * screen_w)] = yellow;
        }
    }
}

int main()
{
    bool first_iter = true;

    uint32_t* data = new uint32_t[screen_w * screen_h];
    SDL_Window* pWindow = nullptr;
    SDL_Renderer* pRenderer = nullptr;
    SDL_Texture* pTexture = nullptr;

    std::string in_txt;
    std::string last_txt;
    std::string var_name = "x";

    create_canvas(data);

    if (disp::e(disp::Startup(&pWindow, &pRenderer, &pTexture), "Startup Failed. Aborting...\n"))
    {
        disp::Shutdown(&pWindow, &pRenderer, &pTexture);
        return -1;
    }

    SDL_StartTextInput();
    disp::Render(pWindow, pRenderer, pTexture, data);

    while (true)
    {    
        a:
            in_txt.clear();
            SDL_Event e;
            while (SDL_WaitEvent(&e)) 
            {
                if (e.type == SDL_QUIT) 
                {
                    disp::Shutdown(&pWindow, &pRenderer, &pTexture);
                }
                else if (e.type == SDL_TEXTINPUT) 
                {
                    in_txt.append(e.text.text);
                    if (in_txt != "-" && in_txt != "+")
                    {
                        std::cout << in_txt << '\r';
                    }               
                }
                else if (e.type == SDL_KEYDOWN) 
                {
                    if (e.key.keysym.sym == SDLK_ESCAPE) 
                    {
                        disp::Shutdown(&pWindow, &pRenderer, &pTexture);
                    }

                    else if (e.key.keysym.sym == SDLK_BACKSPACE) 
                    {
                        if (in_txt.size() > 0) 
                        {
                            std::string clearstr(in_txt.size(), ' ');
                            // Removing multi-byte characters from the UTF-8 string.
                            while (in_txt[in_txt.size() - 1] < -64)
                            {
                                in_txt.erase(in_txt.size() - 1);
                                std::cout << in_txt << '\r';
                            }
                            in_txt.erase(in_txt.size() - 1);
                            
                        }
                    }
                }
                else if (e.type == SDL_KEYUP) 
                {
                    if (e.key.keysym.sym == SDLK_RETURN)
                     {
                        if (in_txt != "-" && in_txt != "+")
                        {
                            std::cout << '\n';
                        }
                        break;
                    }
                }
            }

        SDL_Delay(1);

        if (in_txt == "c" || in_txt == "C")
        {
            for (int jj = 0; jj < (screen_w * screen_h); jj++)
            {
                data[jj] = black;
            }
            create_canvas(data);
            disp::Render(pWindow, pRenderer, pTexture, data);
            SDL_Delay(50);
            goto a;
        }
        else if (in_txt == "+")
        {
            if ((range_upper - 1) > 0 && (range_lower + 1) < 0)
            {
                range_lower++;
                range_upper--;
            }
            
            if (!last_txt.empty())
            {
                for (int jj = 0; jj < (screen_w * screen_h); jj++)
                {
                    data[jj] = black;
                }               
            }
            create_canvas(data);
            std::vector<std::string>rpn = parser::s_yard(last_txt, var_name);
            plot(data, range_lower, range_upper, rpn, var_name);
            disp::Render(pWindow, pRenderer, pTexture, data);
            std::cout << "range: " << range_lower << " to: " << range_upper << '\n';
            SDL_Delay(50);
            goto a;
        }
        else if (in_txt == "-")
        {
            range_lower--;
            range_upper++;
            if (!last_txt.empty())
            {
                for (int jj = 0; jj < (screen_w * screen_h); jj++)
                {
                    data[jj] = black;
                }            
            }          
            std::vector<std::string>rpn = parser::s_yard(last_txt, var_name);
            plot(data, range_lower, range_upper, rpn, var_name);
            create_canvas(data);
            disp::Render(pWindow, pRenderer, pTexture, data);
            std::cout << "range: " << range_lower << " to: " << range_upper << '\n';
            SDL_Delay(50);
            goto a;
        }

        last_txt = in_txt;
        if (!first_iter && in_txt != "-" && in_txt != "+" && !in_txt.empty() && in_txt != "=")
        {
            std::vector<std::string>rpn = parser::s_yard(in_txt, var_name);
            //loop
            plot(data, range_lower, range_upper, rpn, var_name);
        }
        else if (in_txt != "-" && in_txt != "+" && !in_txt.empty() && in_txt != "=")
        {
            std::vector<std::string> rpn = parser::s_yard(in_txt, "x");
            //plot
            plot(data, range_lower, range_upper, rpn, var_name);
            first_iter = false;
        }
        disp::Render(pWindow, pRenderer, pTexture, data);     
    }   
    return 0;
}