#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <cctype>
#include <cmath>
#include <filesystem>

namespace fs = std::filesystem;

std::string findSystemFont() {
    const std::string fontDirs[] = {
        "/usr/share/fonts",
        "/usr/local/share/fonts",
        std::string(std::getenv("HOME") ? std::getenv("HOME") : "") + "/.fonts",
        std::string(std::getenv("HOME") ? std::getenv("HOME") : "") + "/.local/share/fonts"
    };
    for (const auto& dir : fontDirs) {
        if (!fs::exists(dir)) continue;
        for (auto& p : fs::recursive_directory_iterator(dir)) {
            if (!p.is_regular_file()) continue;
            if (p.path().extension() == ".ttf") return p.path().string();
        }
    }
    return "";
}

enum TokenType { NUMBER, OPERATOR, LPAREN, RPAREN };
struct Token {
    TokenType type;
    double value;
    char op;
};

std::vector<Token> tokenize(const std::string& expr) {
    std::vector<Token> tokens;
    size_t i = 0;
    while (i < expr.size()) {
        if (isspace(expr[i])) { i++; continue; }
        if (isdigit(expr[i]) || expr[i] == '.') {
            size_t len = 0;
            double val = std::stod(expr.substr(i), &len);
            tokens.push_back({NUMBER, val, 0});
            i += len;
        } else {
            char c = expr[i];
            if (c == '+' || c == '-' || c == '*' || c == '/' || c == '^') tokens.push_back({OPERATOR, 0, c});
            else if (c == '(') tokens.push_back({LPAREN, 0, 0});
            else if (c == ')') tokens.push_back({RPAREN, 0, 0});
            i++;
        }
    }
    return tokens;
}

int precedence(char op) {
    switch (op) {
        case '^': return 4;
        case '*': case '/': return 3;
        case '+': case '-': return 2;
        default: return 0;
    }
}

double applyOp(double a, double b, char op) {
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return b != 0 ? a / b : NAN;
        case '^': return pow(a, b);
        default: return NAN;
    }
}

std::vector<Token> infixToPostfix(const std::vector<Token>& tokens) {
    std::vector<Token> output;
    std::stack<Token> ops;
    for (auto& t : tokens) {
        if (t.type == NUMBER) output.push_back(t);
        else if (t.type == OPERATOR) {
            while (!ops.empty() && ops.top().type == OPERATOR) {
                if ((precedence(ops.top().op) > precedence(t.op)) ||
                    (precedence(ops.top().op) == precedence(t.op) && t.op != '^')) {
                    output.push_back(ops.top());
                    ops.pop();
                } else break;
            }
            ops.push(t);
        } else if (t.type == LPAREN) ops.push(t);
        else if (t.type == RPAREN) {
            while (!ops.empty() && ops.top().type != LPAREN) {
                output.push_back(ops.top());
                ops.pop();
            }
            if (!ops.empty() && ops.top().type == LPAREN) ops.pop();
        }
    }
    while (!ops.empty()) {
        output.push_back(ops.top());
        ops.pop();
    }
    return output;
}

double evalPostfix(const std::vector<Token>& postfix) {
    std::stack<double> st;
    for (auto& t : postfix) {
        if (t.type == NUMBER) st.push(t.value);
        else if (t.type == OPERATOR) {
            if (st.size() < 2) return NAN;
            double b = st.top(); st.pop();
            double a = st.top(); st.pop();
            st.push(applyOp(a, b, t.op));
        }
    }
    if (st.size() != 1) return NAN;
    return st.top();
}

double evaluate(const std::string& expr) {
    auto tokens = tokenize(expr);
    auto postfix = infixToPostfix(tokens);
    return evalPostfix(postfix);
}

struct Button {
    SDL_Rect rect;
    std::string label;
    char inputChar;
};

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return 1;
    if (TTF_Init() != 0) { SDL_Quit(); return 1; }

    const int winW = 400, winH = 500;
    SDL_Window* window = SDL_CreateWindow("Wumbo Calculator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, winW, winH, 0);
    if (!window) { TTF_Quit(); SDL_Quit(); return 1; }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) { SDL_DestroyWindow(window); TTF_Quit(); SDL_Quit(); return 1; }

    std::string fontPath = findSystemFont();
    if (fontPath.empty()) { SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); TTF_Quit(); SDL_Quit(); return 1; }
    TTF_Font* font = TTF_OpenFont(fontPath.c_str(), 28);
    if (!font) { SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); TTF_Quit(); SDL_Quit(); return 1; }

    const int btnCols = 4, btnRows = 5, btnW = 80, btnH = 60, btnMargin = 10, startX = 20, startY = 150;

    std::vector<Button> buttons = {
        {{0,0,btnW,btnH}, "1", '1'}, {{0,0,btnW,btnH}, "2", '2'}, {{0,0,btnW,btnH}, "3", '3'}, {{0,0,btnW,btnH}, "/", '/'},
        {{0,0,btnW,btnH}, "4", '4'}, {{0,0,btnW,btnH}, "5", '5'}, {{0,0,btnW,btnH}, "6", '6'}, {{0,0,btnW,btnH}, "x", '*'},
        {{0,0,btnW,btnH}, "7", '7'}, {{0,0,btnW,btnH}, "8", '8'}, {{0,0,btnW,btnH}, "9", '9'}, {{0,0,btnW,btnH}, "-", '-'},
        {{0,0,btnW,btnH}, "0", '0'}, {{0,0,btnW,btnH}, "(", '('}, {{0,0,btnW,btnH}, ")", ')'}, {{0,0,btnW,btnH}, ".", '.'},
        {{0,0,btnW,btnH}, "+", '+'}, {{0,0,btnW,btnH}, "C", 0}, {{0,0,btnW,btnH}, "=", 0}, {{0,0,btnW,btnH}, "^", '^'}
    };

    for (int i = 0; i < (int)buttons.size(); ++i) {
        int row = i / btnCols, col = i % btnCols;
        buttons[i].rect.x = startX + col * (btnW + btnMargin);
        buttons[i].rect.y = startY + row * (btnH + btnMargin);
    }

    std::string input;
    bool quit = false;
    int inputScrollX = 0;
    SDL_StartTextInput();

    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) quit = true;
            else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int mx = e.button.x, my = e.button.y;
                for (auto& btn : buttons) {
                    if (mx >= btn.rect.x && mx <= btn.rect.x + btn.rect.w && my >= btn.rect.y && my <= btn.rect.y + btn.rect.h) {
                        if (btn.label == "C") {
                            input.clear();
                        } else if (btn.label == "=") {
                            double res = evaluate(input);
                            if (std::isnan(res)) input = "";
                            else {
                                char buf[64];
                                snprintf(buf, sizeof(buf), "%.6g", res);
                                input = buf;
                            }
                        } else input += btn.inputChar;
                        inputScrollX = 1000000;
                    }
                }
            } else if (e.type == SDL_KEYDOWN) {
                SDL_Keycode k = e.key.keysym.sym;
                if (k == SDLK_BACKSPACE && !input.empty()) input.pop_back();
                else if (k == SDLK_RETURN || k == SDLK_KP_ENTER) {
                    double res = evaluate(input);
                    if (std::isnan(res)) input = "";
                    else {
                        char buf[64];
                        snprintf(buf, sizeof(buf), "%.6g", res);
                        input = buf;
                    }
                    inputScrollX = 1000000;
                } else if (k == SDLK_ESCAPE) quit = true;
                else if (k == SDLK_LEFT) inputScrollX -= 15;
                else if (k == SDLK_RIGHT) inputScrollX += 15;
            } else if (e.type == SDL_TEXTINPUT) {
                char c = e.text.text[0];
                if ((isdigit(c) || c == '+' || c == '-' || c == '*' || c == '/' || c == '.' || c == '(' || c == ')' || c == '^')) {
                    input += c;
                    inputScrollX = 1000000;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);

        SDL_Rect inputRect = {20, 50, 360, 60};
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderFillRect(renderer, &inputRect);

        if (!input.empty()) {
            SDL_Color white = {255, 255, 255, 255};
            SDL_Surface* surf = TTF_RenderUTF8_Blended(font, input.c_str(), white);
            if (surf) {
                SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
                int textW = surf->w, textH = surf->h;

                if (inputScrollX > textW - inputRect.w + 10) inputScrollX = std::max(0, textW - inputRect.w + 10);
                if (inputScrollX < 0) inputScrollX = 0;

                SDL_Rect srcRect, dstRect;
                if (textW <= inputRect.w) {
                    srcRect = {0, 0, textW, textH};
                    dstRect = {inputRect.x + 5, inputRect.y + (inputRect.h - textH) / 2, textW, textH};
                } else {
                    srcRect = {inputScrollX, 0, inputRect.w - 10, textH};
                    dstRect = {inputRect.x + 5, inputRect.y + (inputRect.h - textH) / 2, inputRect.w - 10, textH};
                }
                SDL_RenderCopy(renderer, tex, &srcRect, &dstRect);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(surf);
            }
        }

        for (auto& btn : buttons) {
            SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
            SDL_RenderFillRect(renderer, &btn.rect);
            SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
            SDL_RenderDrawRect(renderer, &btn.rect);
            SDL_Color lightgray = {200, 200, 200, 255};
            SDL_Surface* surf = TTF_RenderUTF8_Blended(font, btn.label.c_str(), lightgray);
            if (surf) {
                SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
                int textW = surf->w, textH = surf->h;
                SDL_Rect dstRect = {btn.rect.x + (btn.rect.w - textW) / 2, btn.rect.y + (btn.rect.h - textH) / 2, textW, textH};
                SDL_RenderCopy(renderer, tex, NULL, &dstRect);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(surf);
            }
        }

        SDL_RenderPresent(renderer);
    }

    SDL_StopTextInput();
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
