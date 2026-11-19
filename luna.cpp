#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <cctype>
#include <map>
#include <fstream>
#include <sstream>
#include <windows.h>
#include <wininet.h> 

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "user32.lib") 
#pragma comment(lib, "gdi32.lib")  


HWND g_mainWindow = NULL;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED) {
            MessageBoxA(hwnd, "Event Triggered!", "Luna Event", MB_OK | MB_ICONINFORMATION);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

std::string CP1251_to_UTF8(const std::string& str) {
    if (str.empty()) return "";
    int wLen = MultiByteToWideChar(1251, 0, str.c_str(), -1, 0, 0);
    std::wstring wstr(wLen, 0);
    MultiByteToWideChar(1251, 0, str.c_str(), -1, &wstr[0], wLen);
    int uLen = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, 0, 0, 0, 0);
    std::string ustr(uLen, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &ustr[0], uLen, 0, 0);
    return ustr.c_str();
}

std::string UTF8_to_CP1251(const std::string& str) {
    if (str.empty()) return "";
    int wLen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, 0, 0);
    std::wstring wstr(wLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], wLen);
    int aLen = WideCharToMultiByte(1251, 0, wstr.c_str(), -1, 0, 0, 0, 0);
    std::string astr(aLen, 0);
    WideCharToMultiByte(1251, 0, wstr.c_str(), -1, &astr[0], aLen, 0, 0);
    return astr.c_str();
}

// ==========================================
// 1. ЛЕКСЕР
// ==========================================
enum class LexTokenType {
    KEYWORD_IF, KEYWORD_ELSE, KEYWORD_WHILE, KEYWORD_END, KEYWORD_PRINT, KEYWORD_INPUT,
    KEYWORD_FUNC, KEYWORD_RETURN, KEYWORD_REQUEST, KEYWORD_POST, KEYWORD_EXTRACT_AI,
    KEYWORD_WINDOW, KEYWORD_BUTTON, KEYWORD_LABEL, KEYWORD_RUN_GUI,
    IDENTIFIER, NUMBER, STRING_LITERAL,
    ASSIGN, OP_EQUALS, OP_GREATER, OP_LESS,
    OP_PLUS, OP_MINUS, OP_MUL, OP_DIV,
    LPAREN, RPAREN, COMMA, LBRACKET, RBRACKET,
    NEWLINE, END_OF_FILE, TOKEN_UNKNOWN
};

struct LexToken {
    LexTokenType type = LexTokenType::TOKEN_UNKNOWN;
    std::string value = "";
};

class Lexer {
private:
    std::string source;
    size_t pos;

    char currentChar() { return (pos >= source.length()) ? '\0' : source[pos]; }
    char peekChar() { return (pos + 1 >= source.length()) ? '\0' : source[pos + 1]; }
    void advance() { pos++; }

public:
    Lexer(const std::string& src) : source(src), pos(0) {}

    LexToken getNextToken() {
        while (currentChar() != '\0') {
            char c = currentChar();
            if (c == '\n') { advance(); return { LexTokenType::NEWLINE, "\\n" }; }
            if (c == ' ' || c == '\t' || c == '\r') { advance(); continue; }

            if (c == '(') { advance(); return { LexTokenType::LPAREN, "(" }; }
            if (c == ')') { advance(); return { LexTokenType::RPAREN, ")" }; }
            if (c == '[') { advance(); return { LexTokenType::LBRACKET, "[" }; }
            if (c == ']') { advance(); return { LexTokenType::RBRACKET, "]" }; }
            if (c == ',') { advance(); return { LexTokenType::COMMA, "," }; }

            if (c == '"') {
                advance();
                std::string str = "";
                while (currentChar() != '\0') {
                    if (currentChar() == '\\' && peekChar() == '"') {
                        str += '"'; advance(); advance();
                    }
                    else if (currentChar() == '"') { break; }
                    else { str += currentChar(); advance(); }
                }
                if (currentChar() == '"') advance();
                return { LexTokenType::STRING_LITERAL, str };
            }

            if (c == '+') { advance(); return { LexTokenType::OP_PLUS, "+" }; }
            if (c == '-') { advance(); return { LexTokenType::OP_MINUS, "-" }; }
            if (c == '*') { advance(); return { LexTokenType::OP_MUL, "*" }; }

            if (c == '/') {
                if (peekChar() == '/') {
                    while (currentChar() != '\n' && currentChar() != '\0') {
                        advance();
                    }
                    continue;
                }
                advance(); return { LexTokenType::OP_DIV, "/" };
            }

            if (c == '=') {
                if (peekChar() == '=') { advance(); advance(); return { LexTokenType::OP_EQUALS, "==" }; }
                advance(); return { LexTokenType::ASSIGN, "=" };
            }
            if (c == '>') { advance(); return { LexTokenType::OP_GREATER, ">" }; }
            if (c == '<') { advance(); return { LexTokenType::OP_LESS, "<" }; }

            if (isdigit(c)) {
                std::string numStr = "";
                while (isdigit(currentChar())) { numStr += currentChar(); advance(); }
                return { LexTokenType::NUMBER, numStr };
            }

            if (isalpha(c) || c == '_' || c < 0) {
                std::string word = "";
                while (isalpha(currentChar()) || currentChar() == '_' || currentChar() < 0 || isdigit(currentChar())) {
                    word += currentChar(); advance();
                }

                // ОБНОВЛЕНИЕ: Поддержка английских и русских синонимов
                if (word == "если" || word == "if") return { LexTokenType::KEYWORD_IF, word };
                if (word == "иначе" || word == "else") return { LexTokenType::KEYWORD_ELSE, word };
                if (word == "пока" || word == "while") return { LexTokenType::KEYWORD_WHILE, word };
                if (word == "конец" || word == "end") return { LexTokenType::KEYWORD_END, word };
                if (word == "вывод" || word == "print") return { LexTokenType::KEYWORD_PRINT, word };
                if (word == "ввод" || word == "input") return { LexTokenType::KEYWORD_INPUT, word };
                if (word == "функция" || word == "func") return { LexTokenType::KEYWORD_FUNC, word };
                if (word == "возврат" || word == "return") return { LexTokenType::KEYWORD_RETURN, word };
                if (word == "запрос" || word == "get") return { LexTokenType::KEYWORD_REQUEST, word };
                if (word == "пост" || word == "post") return { LexTokenType::KEYWORD_POST, word };
                if (word == "ответ_ии" || word == "ai_text") return { LexTokenType::KEYWORD_EXTRACT_AI, word };
                if (word == "окно" || word == "window") return { LexTokenType::KEYWORD_WINDOW, word };
                if (word == "кнопка" || word == "button") return { LexTokenType::KEYWORD_BUTTON, word };
                if (word == "надпись" || word == "label") return { LexTokenType::KEYWORD_LABEL, word };
                if (word == "старт_окна" || word == "run_gui") return { LexTokenType::KEYWORD_RUN_GUI, word };

                return { LexTokenType::IDENTIFIER, word };
            }
            advance();
            return { LexTokenType::TOKEN_UNKNOWN, std::string(1, c) };
        }
        return { LexTokenType::END_OF_FILE, "" };
    }
};

// ==========================================
// 2. ПАМЯТЬ
// ==========================================
struct Value {
    bool isString = false;
    bool isArray = false;
    int num = 0;
    std::string str = "";
    std::vector<Value> arr;

    Value() : isString(false), isArray(false), num(0) {}
    Value(int n) : isString(false), isArray(false), num(n) {}
    Value(std::string s) : isString(true), isArray(false), str(s) {}
    Value(std::vector<Value> a) : isString(false), isArray(true), arr(a) {}

    void print() const {
        if (isArray) {
            std::cout << "[";
            for (size_t i = 0; i < arr.size(); ++i) {
                arr[i].print();
                if (i < arr.size() - 1) std::cout << ", ";
            }
            std::cout << "]";
        }
        else if (isString) std::cout << str;
        else std::cout << num;
    }

    int asInt() const {
        if (isArray) return static_cast<int>(arr.size());
        if (isString) return str.length() > 0 ? 1 : 0;
        return num;
    }
};

class FunctionNode;

class Environment {
public:
    std::map<std::string, Value> vars;
    std::map<std::string, const FunctionNode*> functions;
    Environment* parent;

    Environment(Environment* p = nullptr) : parent(p) {}

    void setVar(const std::string& name, Value value) { vars[name] = value; }

    Value getVar(const std::string& name) {
        if (vars.find(name) != vars.end()) return vars[name];
        if (parent != nullptr) return parent->getVar(name);
        std::cerr << "Error: variable '" << name << "' is not initialized!\n"; exit(1);
    }

    void setArrayElement(const std::string& name, int index, Value value) {
        if (vars.find(name) != vars.end()) {
            if (!vars[name].isArray) { std::cerr << "Error: variable '" << name << "' is not an array!\n"; exit(1); }
            if (index < 0 || index >= static_cast<int>(vars[name].arr.size())) { std::cerr << "Error: index out of bounds!\n"; exit(1); }
            vars[name].arr[index] = value;
            return;
        }
        if (parent != nullptr) { parent->setArrayElement(name, index, value); return; }
        std::cerr << "Error: variable '" << name << "' not found!\n"; exit(1);
    }

    void setFunc(const std::string& name, const FunctionNode* func) {
        if (parent != nullptr) { parent->setFunc(name, func); }
        else { functions[name] = func; }
    }

    const FunctionNode* getFunc(const std::string& name) {
        if (functions.find(name) != functions.end()) return functions[name];
        if (parent != nullptr) return parent->getFunc(name);
        std::cerr << "Error: function '" << name << "' not found!\n"; exit(1);
    }
};

struct ReturnValue { Value val; };

// ==========================================
// 3. АСТ И ИНТЕРПРЕТАТОР
// ==========================================
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual Value evaluate(Environment& env) const { return Value(); }
    virtual void execute(Environment& env) const {}
};

class NumberNode : public ASTNode {
    std::string value;
public:
    NumberNode(std::string val) : value(val) {}
    Value evaluate(Environment& env) const override { return Value(std::stoi(value)); }
};

class StringNode : public ASTNode {
    std::string value;
public:
    StringNode(std::string val) : value(val) {}
    Value evaluate(Environment& env) const override { return Value(value); }
};

class VarNode : public ASTNode {
    std::string name;
public:
    VarNode(std::string n) : name(n) {}
    Value evaluate(Environment& env) const override { return env.getVar(name); }
};

class ArrayNode : public ASTNode {
    std::vector<std::unique_ptr<ASTNode>> elements;
public:
    ArrayNode(std::vector<std::unique_ptr<ASTNode>> el) : elements(std::move(el)) {}
    Value evaluate(Environment& env) const override {
        std::vector<Value> arrValues;
        for (const auto& el : elements) arrValues.push_back(el->evaluate(env));
        return Value(arrValues);
    }
};

class IndexAccessNode : public ASTNode {
    std::string varName; std::unique_ptr<ASTNode> indexExpr;
public:
    IndexAccessNode(std::string name, std::unique_ptr<ASTNode> idx) : varName(name), indexExpr(std::move(idx)) {}
    Value evaluate(Environment& env) const override {
        Value arrVal = env.getVar(varName);
        if (!arrVal.isArray) { std::cerr << "Error: '" << varName << "' is not an array!\n"; exit(1); }
        int idx = indexExpr->evaluate(env).num;
        if (idx < 0 || idx >= static_cast<int>(arrVal.arr.size())) { std::cerr << "Error: index out of bounds!\n"; exit(1); }
        return arrVal.arr[idx];
    }
};

class IndexAssignNode : public ASTNode {
    std::string varName; std::unique_ptr<ASTNode> indexExpr; std::unique_ptr<ASTNode> valueExpr;
public:
    IndexAssignNode(std::string name, std::unique_ptr<ASTNode> idx, std::unique_ptr<ASTNode> val)
        : varName(name), indexExpr(std::move(idx)), valueExpr(std::move(val)) {
    }
    void execute(Environment& env) const override {
        int idx = indexExpr->evaluate(env).num; Value val = valueExpr->evaluate(env);
        env.setArrayElement(varName, idx, val);
    }
};

class WindowNode : public ASTNode {
    std::unique_ptr<ASTNode> titleExpr, wExpr, hExpr;
public:
    WindowNode(std::unique_ptr<ASTNode> t, std::unique_ptr<ASTNode> w, std::unique_ptr<ASTNode> h)
        : titleExpr(std::move(t)), wExpr(std::move(w)), hExpr(std::move(h)) {
    }
    void execute(Environment& env) const override { evaluate(env); }
    Value evaluate(Environment& env) const override {
        std::string title = titleExpr->evaluate(env).str;
        int width = wExpr->evaluate(env).num;
        int height = hExpr->evaluate(env).num;

        WNDCLASSA wc = { 0 };
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = "LunaWindow";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
        RegisterClassA(&wc);

        g_mainWindow = CreateWindowA("LunaWindow", title.c_str(), WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, wc.hInstance, NULL);

        return Value(1);
    }
};

class ButtonNode : public ASTNode {
    std::unique_ptr<ASTNode> textExpr, xExpr, yExpr, wExpr, hExpr;
public:
    ButtonNode(std::unique_ptr<ASTNode> t, std::unique_ptr<ASTNode> x, std::unique_ptr<ASTNode> y, std::unique_ptr<ASTNode> w, std::unique_ptr<ASTNode> h)
        : textExpr(std::move(t)), xExpr(std::move(x)), yExpr(std::move(y)), wExpr(std::move(w)), hExpr(std::move(h)) {
    }
    void execute(Environment& env) const override { evaluate(env); }
    Value evaluate(Environment& env) const override {
        if (!g_mainWindow) { std::cerr << "Error: Create a window first!\n"; exit(1); }
        CreateWindowA("BUTTON", textExpr->evaluate(env).str.c_str(),
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            xExpr->evaluate(env).num, yExpr->evaluate(env).num,
            wExpr->evaluate(env).num, hExpr->evaluate(env).num,
            g_mainWindow, (HMENU)1, GetModuleHandle(NULL), NULL);
        return Value(1);
    }
};

class LabelNode : public ASTNode {
    std::unique_ptr<ASTNode> textExpr, xExpr, yExpr, wExpr, hExpr;
public:
    LabelNode(std::unique_ptr<ASTNode> t, std::unique_ptr<ASTNode> x, std::unique_ptr<ASTNode> y, std::unique_ptr<ASTNode> w, std::unique_ptr<ASTNode> h)
        : textExpr(std::move(t)), xExpr(std::move(x)), yExpr(std::move(y)), wExpr(std::move(w)), hExpr(std::move(h)) {
    }
    void execute(Environment& env) const override { evaluate(env); }
    Value evaluate(Environment& env) const override {
        if (!g_mainWindow) { std::cerr << "Error: Create a window first!\n"; exit(1); }
        CreateWindowA("STATIC", textExpr->evaluate(env).str.c_str(),
            WS_VISIBLE | WS_CHILD,
            xExpr->evaluate(env).num, yExpr->evaluate(env).num,
            wExpr->evaluate(env).num, hExpr->evaluate(env).num,
            g_mainWindow, NULL, GetModuleHandle(NULL), NULL);
        return Value(1);
    }
};

class RunGuiNode : public ASTNode {
public:
    void execute(Environment& env) const override { evaluate(env); }
    Value evaluate(Environment& env) const override {
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return Value(1);
    }
};

class ExtractAINode : public ASTNode {
    std::unique_ptr<ASTNode> jsonExpr;
public:
    ExtractAINode(std::unique_ptr<ASTNode> j) : jsonExpr(std::move(j)) {}
    void execute(Environment& env) const override { evaluate(env); }
    Value evaluate(Environment& env) const override {
        std::string json = jsonExpr->evaluate(env).str;
        std::string key = "\"text\": \"";
        size_t pos = json.find(key);
        if (pos == std::string::npos) return Value(json);

        pos += key.length();
        std::string result = "";
        while (pos < json.length()) {
            if (json[pos] == '"' && json[pos - 1] != '\\') break;
            if (json[pos] == '\\' && pos + 1 < json.length()) {
                if (json[pos + 1] == 'n') { result += '\n'; pos += 2; continue; }
                if (json[pos + 1] == '"') { result += '"'; pos += 2; continue; }
            }
            result += json[pos];
            pos++;
        }
        return Value(result);
    }
};

class RequestNode : public ASTNode {
    std::unique_ptr<ASTNode> urlExpr;
public:
    RequestNode(std::unique_ptr<ASTNode> url) : urlExpr(std::move(url)) {}
    void execute(Environment& env) const override { evaluate(env); }
    Value evaluate(Environment& env) const override {
        Value urlVal = urlExpr->evaluate(env);
        if (!urlVal.isString) { std::cerr << "Error: URL must be a string!\n"; exit(1); }

        HINTERNET hInternet = InternetOpenA("LunaLanguage/1.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet) return Value("Network Error");
        HINTERNET hConnect = InternetOpenUrlA(hInternet, urlVal.str.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (!hConnect) { InternetCloseHandle(hInternet); return Value("Network Error"); }

        std::string response = ""; char buffer[4096]; DWORD bytesRead = 0;
        while (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
            buffer[bytesRead] = '\0'; response += buffer;
        }
        InternetCloseHandle(hConnect); InternetCloseHandle(hInternet);
        return Value(UTF8_to_CP1251(response));
    }
};

class PostRequestNode : public ASTNode {
    std::unique_ptr<ASTNode> serverExpr; std::unique_ptr<ASTNode> pathExpr; std::unique_ptr<ASTNode> bodyExpr;
public:
    PostRequestNode(std::unique_ptr<ASTNode> s, std::unique_ptr<ASTNode> p, std::unique_ptr<ASTNode> b)
        : serverExpr(std::move(s)), pathExpr(std::move(p)), bodyExpr(std::move(b)) {
    }
    void execute(Environment& env) const override { evaluate(env); }
    Value evaluate(Environment& env) const override {
        Value sVal = serverExpr->evaluate(env); Value pVal = pathExpr->evaluate(env); Value bVal = bodyExpr->evaluate(env);

        if (!sVal.isString || !pVal.isString || !bVal.isString) {
            std::cerr << "Error: POST arguments must be strings!\n"; exit(1);
        }

        HINTERNET hInternet = InternetOpenA("Luna/1.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet) return Value("Network Error");

        HINTERNET hConnect = InternetConnectA(hInternet, sVal.str.c_str(), INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
        if (!hConnect) { InternetCloseHandle(hInternet); return Value("Connection Error"); }

        HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", pVal.str.c_str(), NULL, NULL, NULL, INTERNET_FLAG_SECURE, 0);
        if (!hRequest) { InternetCloseHandle(hConnect); InternetCloseHandle(hInternet); return Value("Request Error"); }

        std::string headers = "Content-Type: application/json\r\n";
        std::string utf8_body = CP1251_to_UTF8(bVal.str);
        HttpSendRequestA(hRequest, headers.c_str(), static_cast<DWORD>(headers.length()), (LPVOID)utf8_body.c_str(), static_cast<DWORD>(utf8_body.length()));

        std::string response = ""; char buffer[4096]; DWORD bytesRead = 0;
        while (InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
            buffer[bytesRead] = '\0'; response += buffer;
        }

        InternetCloseHandle(hRequest); InternetCloseHandle(hConnect); InternetCloseHandle(hInternet);
        return Value(UTF8_to_CP1251(response));
    }
};

class BinOpNode : public ASTNode {
    std::unique_ptr<ASTNode> left; LexToken op; std::unique_ptr<ASTNode> right;
public:
    BinOpNode(std::unique_ptr<ASTNode> l, LexToken o, std::unique_ptr<ASTNode> r)
        : left(std::move(l)), op(o), right(std::move(r)) {
    }

    Value evaluate(Environment& env) const override {
        Value leftVal = left->evaluate(env);
        Value rightVal = right->evaluate(env);

        if (op.type == LexTokenType::OP_PLUS) {
            if (leftVal.isString || rightVal.isString) {
                std::string s1 = leftVal.isString ? leftVal.str : std::to_string(leftVal.num);
                std::string s2 = rightVal.isString ? rightVal.str : std::to_string(rightVal.num);
                return Value(s1 + s2);
            }
            return Value(leftVal.num + rightVal.num);
        }

        if (op.type == LexTokenType::OP_EQUALS) {
            if (leftVal.isString && rightVal.isString) return Value((leftVal.str == rightVal.str) ? 1 : 0);
            if (!leftVal.isString && !rightVal.isString) return Value((leftVal.num == rightVal.num) ? 1 : 0);
            return Value(0);
        }

        if (leftVal.isArray || rightVal.isArray || leftVal.isString || rightVal.isString) {
            std::cerr << "Error: Math operations require numbers!\n"; exit(1);
        }

        if (op.type == LexTokenType::OP_MINUS)   return Value(leftVal.num - rightVal.num);
        if (op.type == LexTokenType::OP_MUL)     return Value(leftVal.num * rightVal.num);
        if (op.type == LexTokenType::OP_DIV) {
            if (rightVal.num == 0) { std::cerr << "Division by zero!\n"; exit(1); }
            return Value(leftVal.num / rightVal.num);
        }
        if (op.type == LexTokenType::OP_GREATER) return Value((leftVal.num > rightVal.num) ? 1 : 0);
        if (op.type == LexTokenType::OP_LESS)    return Value((leftVal.num < rightVal.num) ? 1 : 0);

        return Value(0);
    }
};

class AssignNode : public ASTNode {
    std::string varName; std::unique_ptr<ASTNode> expr;
public:
    AssignNode(std::string name, std::unique_ptr<ASTNode> e) : varName(name), expr(std::move(e)) {}
    void execute(Environment& env) const override { env.setVar(varName, expr->evaluate(env)); }
};

class PrintNode : public ASTNode {
    std::unique_ptr<ASTNode> expr;
public:
    PrintNode(std::unique_ptr<ASTNode> e) : expr(std::move(e)) {}
    void execute(Environment& env) const override { expr->evaluate(env).print(); std::cout << "\n"; }
};

class InputNode : public ASTNode {
    std::string varName;
public:
    InputNode(std::string name) : varName(name) {}
    void execute(Environment& env) const override {
        std::string inputStr; std::getline(std::cin, inputStr);
        try {
            size_t pos; int num = std::stoi(inputStr, &pos);
            if (pos == inputStr.length()) { env.setVar(varName, Value(num)); }
            else { env.setVar(varName, Value(inputStr)); }
        }
        catch (...) { env.setVar(varName, Value(inputStr)); }
    }
};

class BlockNode : public ASTNode {
public:
    std::vector<std::unique_ptr<ASTNode>> statements;
    void execute(Environment& env) const override { for (const auto& stmt : statements) stmt->execute(env); }
};

class IfNode : public ASTNode {
    std::unique_ptr<ASTNode> condition; std::unique_ptr<BlockNode> thenBlock; std::unique_ptr<BlockNode> elseBlock;
public:
    IfNode(std::unique_ptr<ASTNode> cond, std::unique_ptr<BlockNode> tBlock, std::unique_ptr<BlockNode> eBlock)
        : condition(std::move(cond)), thenBlock(std::move(tBlock)), elseBlock(std::move(eBlock)) {
    }
    void execute(Environment& env) const override {
        if (condition->evaluate(env).asInt() > 0) thenBlock->execute(env);
        else if (elseBlock != nullptr) elseBlock->execute(env);
    }
};

class WhileNode : public ASTNode {
    std::unique_ptr<ASTNode> condition; std::unique_ptr<BlockNode> body;
public:
    WhileNode(std::unique_ptr<ASTNode> cond, std::unique_ptr<BlockNode> b)
        : condition(std::move(cond)), body(std::move(b)) {
    }
    void execute(Environment& env) const override {
        while (condition->evaluate(env).asInt() > 0) body->execute(env);
    }
};

class ReturnNode : public ASTNode {
    std::unique_ptr<ASTNode> expr;
public:
    ReturnNode(std::unique_ptr<ASTNode> e) : expr(std::move(e)) {}
    void execute(Environment& env) const override { throw ReturnValue{ expr->evaluate(env) }; }
};

class FunctionNode : public ASTNode {
public:
    std::string name; std::vector<std::string> args; std::unique_ptr<BlockNode> body;
    FunctionNode(std::string n, std::vector<std::string> a, std::unique_ptr<BlockNode> b)
        : name(n), args(std::move(a)), body(std::move(b)) {
    }
    void execute(Environment& env) const override { env.setFunc(name, this); }
};

class CallNode : public ASTNode {
    std::string funcName; std::vector<std::unique_ptr<ASTNode>> args;
public:
    CallNode(std::string name, std::vector<std::unique_ptr<ASTNode>> a) : funcName(name), args(std::move(a)) {}
    Value evaluate(Environment& env) const override {
        const FunctionNode* func = env.getFunc(funcName);
        if (func->args.size() != args.size()) { std::cerr << "Error: Invalid number of arguments\n"; exit(1); }

        std::vector<Value> argValues;
        for (const auto& arg : args) argValues.push_back(arg->evaluate(env));

        Environment localEnv(&env);
        for (size_t i = 0; i < func->args.size(); ++i) localEnv.setVar(func->args[i], argValues[i]);

        try { func->body->execute(localEnv); }
        catch (ReturnValue ret) { return ret.val; }
        return Value(0);
    }
    void execute(Environment& env) const override { evaluate(env); }
};

// ==========================================
// 4. ПАРСЕР
// ==========================================
class Parser {
    Lexer lexer;
    LexToken currentToken;

    void eat(LexTokenType type) {
        if (currentToken.type == type) {
            currentToken = lexer.getNextToken();
        }
        else {
            std::cerr << "Syntax Error: unexpected token '" << currentToken.value << "'\n";
            exit(1);
        }
    }
    void skipNewlines() { while (currentToken.type == LexTokenType::NEWLINE) eat(LexTokenType::NEWLINE); }

    std::unique_ptr<ASTNode> parseFactor() {
        LexToken token = currentToken;
        if (token.type == LexTokenType::NUMBER) { eat(LexTokenType::NUMBER); return std::make_unique<NumberNode>(token.value); }
        else if (token.type == LexTokenType::STRING_LITERAL) { eat(LexTokenType::STRING_LITERAL); return std::make_unique<StringNode>(token.value); }
        else if (token.type == LexTokenType::KEYWORD_REQUEST) {
            eat(LexTokenType::KEYWORD_REQUEST); eat(LexTokenType::LPAREN);
            auto urlNode = parseMathExpression(); eat(LexTokenType::RPAREN);
            return std::make_unique<RequestNode>(std::move(urlNode));
        }
        else if (token.type == LexTokenType::KEYWORD_POST) {
            eat(LexTokenType::KEYWORD_POST); eat(LexTokenType::LPAREN);
            auto serverNode = parseMathExpression(); eat(LexTokenType::COMMA);
            auto pathNode = parseMathExpression(); eat(LexTokenType::COMMA);
            auto bodyNode = parseMathExpression(); eat(LexTokenType::RPAREN);
            return std::make_unique<PostRequestNode>(std::move(serverNode), std::move(pathNode), std::move(bodyNode));
        }
        else if (token.type == LexTokenType::KEYWORD_EXTRACT_AI) {
            eat(LexTokenType::KEYWORD_EXTRACT_AI); eat(LexTokenType::LPAREN);
            auto jsonNode = parseMathExpression(); eat(LexTokenType::RPAREN);
            return std::make_unique<ExtractAINode>(std::move(jsonNode));
        }
        else if (token.type == LexTokenType::KEYWORD_WINDOW) {
            eat(LexTokenType::KEYWORD_WINDOW); eat(LexTokenType::LPAREN);
            auto title = parseMathExpression(); eat(LexTokenType::COMMA);
            auto w = parseMathExpression(); eat(LexTokenType::COMMA);
            auto h = parseMathExpression(); eat(LexTokenType::RPAREN);
            return std::make_unique<WindowNode>(std::move(title), std::move(w), std::move(h));
        }
        else if (token.type == LexTokenType::KEYWORD_BUTTON) {
            eat(LexTokenType::KEYWORD_BUTTON); eat(LexTokenType::LPAREN);
            auto title = parseMathExpression(); eat(LexTokenType::COMMA);
            auto x = parseMathExpression(); eat(LexTokenType::COMMA);
            auto y = parseMathExpression(); eat(LexTokenType::COMMA);
            auto w = parseMathExpression(); eat(LexTokenType::COMMA);
            auto h = parseMathExpression(); eat(LexTokenType::RPAREN);
            return std::make_unique<ButtonNode>(std::move(title), std::move(x), std::move(y), std::move(w), std::move(h));
        }
        else if (token.type == LexTokenType::KEYWORD_LABEL) {
            eat(LexTokenType::KEYWORD_LABEL); eat(LexTokenType::LPAREN);
            auto title = parseMathExpression(); eat(LexTokenType::COMMA);
            auto x = parseMathExpression(); eat(LexTokenType::COMMA);
            auto y = parseMathExpression(); eat(LexTokenType::COMMA);
            auto w = parseMathExpression(); eat(LexTokenType::COMMA);
            auto h = parseMathExpression(); eat(LexTokenType::RPAREN);
            return std::make_unique<LabelNode>(std::move(title), std::move(x), std::move(y), std::move(w), std::move(h));
        }
        else if (token.type == LexTokenType::KEYWORD_RUN_GUI) {
            eat(LexTokenType::KEYWORD_RUN_GUI); eat(LexTokenType::LPAREN); eat(LexTokenType::RPAREN);
            return std::make_unique<RunGuiNode>();
        }
        else if (token.type == LexTokenType::LBRACKET) {
            eat(LexTokenType::LBRACKET);
            std::vector<std::unique_ptr<ASTNode>> elements;
            if (currentToken.type != LexTokenType::RBRACKET) {
                elements.push_back(parseMathExpression());
                while (currentToken.type == LexTokenType::COMMA) {
                    eat(LexTokenType::COMMA);
                    elements.push_back(parseMathExpression());
                }
            }
            eat(LexTokenType::RBRACKET);
            return std::make_unique<ArrayNode>(std::move(elements));
        }
        else if (token.type == LexTokenType::IDENTIFIER) {
            eat(LexTokenType::IDENTIFIER);
            if (currentToken.type == LexTokenType::LPAREN) {
                eat(LexTokenType::LPAREN);
                std::vector<std::unique_ptr<ASTNode>> args;
                if (currentToken.type != LexTokenType::RPAREN) {
                    args.push_back(parseMathExpression());
                    while (currentToken.type == LexTokenType::COMMA) {
                        eat(LexTokenType::COMMA); args.push_back(parseMathExpression());
                    }
                }
                eat(LexTokenType::RPAREN);
                return std::make_unique<CallNode>(token.value, std::move(args));
            }
            else if (currentToken.type == LexTokenType::LBRACKET) {
                eat(LexTokenType::LBRACKET);
                auto indexExpr = parseMathExpression();
                eat(LexTokenType::RBRACKET);
                return std::make_unique<IndexAccessNode>(token.value, std::move(indexExpr));
            }
            return std::make_unique<VarNode>(token.value);
        }
        else if (token.type == LexTokenType::LPAREN) {
            eat(LexTokenType::LPAREN);
            auto node = parseMathExpression();
            eat(LexTokenType::RPAREN);
            return node;
        }
        std::cerr << "Syntax Error: expected number, string or function\n"; exit(1);
    }

    std::unique_ptr<ASTNode> parseTerm() {
        auto left = parseFactor();
        while (currentToken.type == LexTokenType::OP_MUL || currentToken.type == LexTokenType::OP_DIV) {
            LexToken op = currentToken; eat(op.type);
            left = std::make_unique<BinOpNode>(std::move(left), op, parseFactor());
        }
        return left;
    }

    std::unique_ptr<ASTNode> parseMathExpression() {
        auto left = parseTerm();
        while (currentToken.type == LexTokenType::OP_PLUS || currentToken.type == LexTokenType::OP_MINUS) {
            LexToken op = currentToken; eat(op.type);
            left = std::make_unique<BinOpNode>(std::move(left), op, parseTerm());
        }
        return left;
    }

    std::unique_ptr<ASTNode> parseCondition() {
        auto left = parseMathExpression();
        LexToken op = currentToken;
        if (op.type == LexTokenType::OP_GREATER || op.type == LexTokenType::OP_LESS || op.type == LexTokenType::OP_EQUALS) {
            eat(op.type);
            return std::make_unique<BinOpNode>(std::move(left), op, parseMathExpression());
        }
        return left;
    }

    std::unique_ptr<ASTNode> parseStatement() {
        if (currentToken.type == LexTokenType::IDENTIFIER) {
            std::string name = currentToken.value;
            eat(LexTokenType::IDENTIFIER);

            if (currentToken.type == LexTokenType::LPAREN) {
                eat(LexTokenType::LPAREN);
                std::vector<std::unique_ptr<ASTNode>> args;
                if (currentToken.type != LexTokenType::RPAREN) {
                    args.push_back(parseMathExpression());
                    while (currentToken.type == LexTokenType::COMMA) {
                        eat(LexTokenType::COMMA); args.push_back(parseMathExpression());
                    }
                }
                eat(LexTokenType::RPAREN);
                return std::make_unique<CallNode>(name, std::move(args));
            }
            else if (currentToken.type == LexTokenType::LBRACKET) {
                eat(LexTokenType::LBRACKET);
                auto indexExpr = parseMathExpression();
                eat(LexTokenType::RBRACKET);
                eat(LexTokenType::ASSIGN);
                return std::make_unique<IndexAssignNode>(name, std::move(indexExpr), parseMathExpression());
            }
            else {
                eat(LexTokenType::ASSIGN);
                return std::make_unique<AssignNode>(name, parseMathExpression());
            }
        }
        else if (currentToken.type == LexTokenType::KEYWORD_PRINT) {
            eat(LexTokenType::KEYWORD_PRINT);
            return std::make_unique<PrintNode>(parseMathExpression());
        }
        else if (currentToken.type == LexTokenType::KEYWORD_INPUT) {
            eat(LexTokenType::KEYWORD_INPUT);
            std::string varName = currentToken.value; eat(LexTokenType::IDENTIFIER);
            return std::make_unique<InputNode>(varName);
        }
        else if (currentToken.type == LexTokenType::KEYWORD_FUNC) {
            eat(LexTokenType::KEYWORD_FUNC); std::string funcName = currentToken.value;
            eat(LexTokenType::IDENTIFIER); eat(LexTokenType::LPAREN);

            std::vector<std::string> args;
            if (currentToken.type != LexTokenType::RPAREN) {
                args.push_back(currentToken.value); eat(LexTokenType::IDENTIFIER);
                while (currentToken.type == LexTokenType::COMMA) {
                    eat(LexTokenType::COMMA); args.push_back(currentToken.value); eat(LexTokenType::IDENTIFIER);
                }
            }
            eat(LexTokenType::RPAREN); skipNewlines();

            auto body = std::make_unique<BlockNode>();
            while (currentToken.type != LexTokenType::KEYWORD_END) {
                body->statements.push_back(parseStatement()); skipNewlines();
            }
            eat(LexTokenType::KEYWORD_END);
            return std::make_unique<FunctionNode>(funcName, std::move(args), std::move(body));
        }
        else if (currentToken.type == LexTokenType::KEYWORD_RETURN) {
            eat(LexTokenType::KEYWORD_RETURN);
            return std::make_unique<ReturnNode>(parseMathExpression());
        }
        else if (currentToken.type == LexTokenType::KEYWORD_IF) {
            eat(LexTokenType::KEYWORD_IF); auto condition = parseCondition(); skipNewlines();
            auto thenBlock = std::make_unique<BlockNode>();
            while (currentToken.type != LexTokenType::KEYWORD_ELSE && currentToken.type != LexTokenType::KEYWORD_END) {
                thenBlock->statements.push_back(parseStatement()); skipNewlines();
            }
            std::unique_ptr<BlockNode> elseBlock = nullptr;
            if (currentToken.type == LexTokenType::KEYWORD_ELSE) {
                eat(LexTokenType::KEYWORD_ELSE); skipNewlines();
                elseBlock = std::make_unique<BlockNode>();
                while (currentToken.type != LexTokenType::KEYWORD_END) {
                    elseBlock->statements.push_back(parseStatement()); skipNewlines();
                }
            }
            eat(LexTokenType::KEYWORD_END);
            return std::make_unique<IfNode>(std::move(condition), std::move(thenBlock), std::move(elseBlock));
        }
        else if (currentToken.type == LexTokenType::KEYWORD_WHILE) {
            eat(LexTokenType::KEYWORD_WHILE); auto condition = parseCondition(); skipNewlines();
            auto bodyBlock = std::make_unique<BlockNode>();
            while (currentToken.type != LexTokenType::KEYWORD_END) {
                bodyBlock->statements.push_back(parseStatement()); skipNewlines();
            }
            eat(LexTokenType::KEYWORD_END);
            return std::make_unique<WhileNode>(std::move(condition), std::move(bodyBlock));
        }

        if (currentToken.type == LexTokenType::KEYWORD_WINDOW ||
            currentToken.type == LexTokenType::KEYWORD_BUTTON ||
            currentToken.type == LexTokenType::KEYWORD_LABEL ||
            currentToken.type == LexTokenType::KEYWORD_RUN_GUI) {
            return parseMathExpression();
        }

        std::cerr << "Syntax Error: unknown command\n"; exit(1);
    }

public:
    Parser(const std::string& sourceCode) : lexer(sourceCode) { currentToken = lexer.getNextToken(); }

    std::unique_ptr<BlockNode> parseProgram() {
        auto program = std::make_unique<BlockNode>(); skipNewlines();
        while (currentToken.type != LexTokenType::END_OF_FILE) {
            program->statements.push_back(parseStatement()); skipNewlines();
        }
        return program;
    }
};

// ==========================================
// МЕЙН
// ==========================================
int main(int argc, char* argv[]) {
    // Оставляем кодировку консоли для правильного отображения кириллицы
    SetConsoleCP(1251); SetConsoleOutputCP(1251); setlocale(LC_ALL, "Russian");

    std::string fileName = "main.lun";
    if (argc > 1) fileName = argv[1];

    std::ifstream file(fileName);
    if (!file.is_open()) { std::cerr << "Critical Error: Could not open file '" << fileName << "'!\n"; return 1; }

    std::stringstream buffer; buffer << file.rdbuf();
    std::string sourceCode = buffer.str();

    if (sourceCode.size() >= 3 && (unsigned char)sourceCode[0] == 0xEF &&
        (unsigned char)sourceCode[1] == 0xBB && (unsigned char)sourceCode[2] == 0xBF) {
        sourceCode = sourceCode.substr(3);
    }

    Parser parser(sourceCode);
    std::unique_ptr<BlockNode> ast = parser.parseProgram();

    Environment env; ast->execute(env);
    return 0;
}
