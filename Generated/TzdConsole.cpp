// ============================================================================
// TzdConsole.cpp - Interactive console with syntax highlighting,
//                  clipboard, history, and line editing
// ============================================================================

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include "TzdConsole.h"
#include <iostream>
#include <algorithm>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <winuser.h>
#endif

// ============================================================================
// ANSI color codes
// ============================================================================
static const char* ANSI_RESET   = "\033[0m";
static const char* ANSI_RED     = "\033[31m";
static const char* ANSI_GREEN   = "\033[32m";
static const char* ANSI_YELLOW  = "\033[33m";
static const char* ANSI_BLUE    = "\033[34m";
static const char* ANSI_MAGENTA = "\033[35m";
static const char* ANSI_CYAN    = "\033[36m";
static const char* ANSI_WHITE   = "\033[37m";
static const char* ANSI_BBLUE   = "\033[94m";
static const char* ANSI_BGREEN  = "\033[92m";
static const char* ANSI_BYELLOW = "\033[93m";
static const char* ANSI_BCYAN   = "\033[96m";
static const char* ANSI_GRAY    = "\033[90m";
static const char* ANSI_BRED    = "\033[91m";

// ============================================================================
// Keywords and Types
// ============================================================================
static const std::vector<std::string> KEYWORDS = {
    "fun", "class", "var", "let", "const", "if", "else", "elif",
    "for", "while", "do", "return", "break", "continue",
    "import", "export", "new", "extends", "super", "this",
    "try", "catch", "throw", "finally", "switch", "case",
    "default", "in", "is", "as", "null", "true", "false",
    "and", "or", "not", "async", "await", "yield"
};

static const std::vector<std::string> TYPES = {
    "int", "float", "double", "string", "bool", "long",
    "ptr", "void", "array", "map", "function", "hwnd",
    "char", "byte", "short", "uint", "ulong"
};

// ============================================================================
// Constructor / Destructor
// ============================================================================
TzdConsole::TzdConsole()
    : m_highlightEnabled(true)
    , m_vtEnabled(false)
    , m_historyIndex(-1)
    , m_cursorPos(0)
    , m_braceCount(0)
    , m_inString(false)
    , m_inBlockComment(false)
{
#ifdef _WIN32
    m_hInput = GetStdHandle(STD_INPUT_HANDLE);
    m_hOutput = GetStdHandle(STD_OUTPUT_HANDLE);

    // Save old modes
    GetConsoleMode(m_hInput, &m_oldInputMode);
    GetConsoleMode(m_hOutput, &m_oldOutputMode);

    // Enable Virtual Terminal Processing on output
    DWORD outMode = m_oldOutputMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(m_hOutput, outMode);
    m_vtEnabled = true;

    // Set input mode: disable line input and echo for per-keystroke reading
    DWORD inMode = ENABLE_PROCESSED_INPUT;
    SetConsoleMode(m_hInput, inMode);
#endif
}

TzdConsole::~TzdConsole() {
#ifdef _WIN32
    // Restore original console modes
    SetConsoleMode(m_hInput, m_oldInputMode);
    SetConsoleMode(m_hOutput, m_oldOutputMode);
#endif
}

// ============================================================================
// Static methods
// ============================================================================
std::string TzdConsole::ansiColor(const std::string& code) {
    return code;
}

std::string TzdConsole::ansiReset() {
    return ANSI_RESET;
}

void TzdConsole::printColored(const std::string& text, const std::string& color) {
    std::cout << color << text << ANSI_RESET;
}

void TzdConsole::printError(const std::string& text) {
    std::cout << ANSI_BRED << text << ANSI_RESET << std::endl;
}

void TzdConsole::printSuccess(const std::string& text) {
    std::cout << ANSI_BGREEN << text << ANSI_RESET << std::endl;
}

void TzdConsole::printPrompt(const std::string& text) {
    std::cout << ANSI_BCYAN << text << ANSI_RESET;
}

void TzdConsole::clearScreen() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(m_hOutput, &csbi);
    DWORD cells = csbi.dwSize.X * csbi.dwSize.Y;
    COORD home = {0, 0};
    DWORD written;
    FillConsoleOutputCharacter(m_hOutput, ' ', cells, home, &written);
    FillConsoleOutputAttribute(m_hOutput, csbi.wAttributes, cells, home, &written);
    SetConsoleCursorPosition(m_hOutput, home);
#else
    std::cout << "\033[2J\033[H";
#endif
}

// ============================================================================
// Keyword/Type checking
// ============================================================================
const std::vector<std::string>& TzdConsole::getKeywords() { return KEYWORDS; }
const std::vector<std::string>& TzdConsole::getTypes() { return TYPES; }

bool TzdConsole::isKeyword(const std::string& s) {
    return std::find(KEYWORDS.begin(), KEYWORDS.end(), s) != KEYWORDS.end();
}

bool TzdConsole::isType(const std::string& s) {
    return std::find(TYPES.begin(), TYPES.end(), s) != TYPES.end();
}

// ============================================================================
// Tokenizer
// ============================================================================
std::vector<TzdConsole::Token> TzdConsole::tokenize(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;
    size_t len = line.size();

    while (i < len) {
        // Whitespace
        if (isspace((unsigned char)line[i])) {
            size_t start = i;
            while (i < len && isspace((unsigned char)line[i])) i++;
            tokens.push_back({TokenType::Whitespace, line.substr(start, i - start), (int)start, (int)(i - start)});
            continue;
        }

        // Line comment
        if (i + 1 < len && line[i] == '/' && line[i+1] == '/') {
            tokens.push_back({TokenType::Comment, line.substr(i), (int)i, (int)(len - i)});
            i = len;
            continue;
        }

        // Block comment
        if (i + 1 < len && line[i] == '/' && line[i+1] == '*') {
            size_t start = i;
            i += 2;
            while (i + 1 < len && !(line[i] == '*' && line[i+1] == '/')) i++;
            if (i + 1 < len) i += 2;
            tokens.push_back({TokenType::Comment, line.substr(start, i - start), (int)start, (int)(i - start)});
            continue;
        }

        // String literal
        if (line[i] == '"') {
            size_t start = i;
            i++; // skip opening quote
            while (i < len) {
                if (line[i] == '\\' && i + 1 < len) { i += 2; continue; }
                if (line[i] == '"') { i++; break; }
                i++;
            }
            tokens.push_back({TokenType::String, line.substr(start, i - start), (int)start, (int)(i - start)});
            continue;
        }

        // Single-quoted string
        if (line[i] == '\'') {
            size_t start = i;
            i++;
            while (i < len) {
                if (line[i] == '\\' && i + 1 < len) { i += 2; continue; }
                if (line[i] == '\'') { i++; break; }
                i++;
            }
            tokens.push_back({TokenType::String, line.substr(start, i - start), (int)start, (int)(i - start)});
            continue;
        }

        // Number
        if (isdigit((unsigned char)line[i]) || (line[i] == '.' && i + 1 < len && isdigit((unsigned char)line[i+1]))) {
            size_t start = i;
            // Handle hex
            if (line[i] == '0' && i + 1 < len && (line[i+1] == 'x' || line[i+1] == 'X')) {
                i += 2;
                while (i < len && isxdigit((unsigned char)line[i])) i++;
            } else {
                while (i < len && (isdigit((unsigned char)line[i]) || line[i] == '.')) i++;
            }
            tokens.push_back({TokenType::Number, line.substr(start, i - start), (int)start, (int)(i - start)});
            continue;
        }

        // Identifier or keyword
        if (isalpha((unsigned char)line[i]) || line[i] == '_') {
            size_t start = i;
            while (i < len && (isalnum((unsigned char)line[i]) || line[i] == '_')) i++;
            std::string word = line.substr(start, i - start);

            TokenType type;
            if (isKeyword(word)) type = TokenType::Keyword;
            else if (isType(word)) type = TokenType::Type;
            else if (i < len && line[i] == '(') type = TokenType::Function;
            else type = TokenType::Identifier;

            tokens.push_back({type, word, (int)start, (int)(i - start)});
            continue;
        }

        // Operators and punctuation
        if (strchr("+-*/%=<>!&|^~?:.,;(){}[]@", line[i])) {
            size_t start = i;
            i++;
            // Multi-char operators
            if (i < len && strchr("=+-*/<>|&", line[i])) i++;
            tokens.push_back({TokenType::Operator, line.substr(start, i - start), (int)start, (int)(i - start)});
            continue;
        }

        // Unknown character
        tokens.push_back({TokenType::Unknown, std::string(1, line[i]), (int)i, 1});
        i++;
    }

    return tokens;
}

std::string TzdConsole::tokenColor(TokenType type) {
    switch (type) {
        case TokenType::Keyword:  return ANSI_BBLUE;
        case TokenType::Type:      return ANSI_CYAN;
        case TokenType::String:    return ANSI_GREEN;
        case TokenType::Number:    return ANSI_YELLOW;
        case TokenType::Operator:  return ANSI_WHITE;
        case TokenType::Comment:   return ANSI_GRAY;
        case TokenType::Function:  return ANSI_BYELLOW;
        case TokenType::Identifier:return ANSI_WHITE;
        case TokenType::Whitespace:return "";
        default:                   return "";
    }
}

// ============================================================================
// Syntax highlighting
// ============================================================================
std::string TzdConsole::highlightLine(const std::string& line) {
    if (!m_highlightEnabled || !m_vtEnabled) return line;

    auto tokens = tokenize(line);
    std::string result;
    for (const auto& tok : tokens) {
        std::string color = tokenColor(tok.type);
        if (color.empty()) {
            result += tok.text;
        } else {
            result += color + tok.text + ANSI_RESET;
        }
    }
    return result;
}

// ============================================================================
// Render line with highlighting and cursor
// ============================================================================
void TzdConsole::renderLine(const std::string& prompt, const std::string& line, size_t cursor) {
    if (!m_vtEnabled) {
        // Fallback: simple output without highlighting
        // Clear current line and redraw
        std::cout << "\r" << prompt << line;
        // Clear remaining chars on line
        std::cout << "\033[K"; // VT erase to end of line
        // Position cursor
        size_t targetCol = prompt.size() + cursor;
        std::cout << "\r";
        for (size_t i = 0; i < targetCol; i++) std::cout << "\033[C";
        return;
    }

#ifdef _WIN32
    // Get console info
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(m_hOutput, &csbi);

    // Clear current line
    COORD pos = {csbi.dwCursorPosition.X, csbi.dwCursorPosition.Y};
    // Move to beginning of line (after prompt)
    SHORT promptLen = (SHORT)prompt.size();
    COORD lineStart = {(SHORT)(pos.X - (SHORT)m_lineBuffer.size() - promptLen + (SHORT)m_cursorPos), pos.Y};

    // Simplified: just clear and redraw
    std::cout << "\r";
    // Print prompt
    std::cout << ANSI_BCYAN << prompt << ANSI_RESET;
    // Print highlighted line
    if (m_highlightEnabled) {
        std::cout << highlightLine(line);
    } else {
        std::cout << line;
    }
    // Clear to end of line
    std::cout << "\033[K";

    // Position cursor
    size_t targetCol = prompt.size() + cursor;
    std::cout << "\r";
    if (targetCol > 0) {
        std::cout << "\033[" << targetCol << "C";
    }
#else
    std::cout << "\r" << ANSI_BCYAN << prompt << ANSI_RESET << highlightLine(line) << "\033[K";
    size_t targetCol = prompt.size() + cursor;
    std::cout << "\r";
    for (size_t i = 0; i < targetCol; i++) std::cout << "\033[C";
#endif
}

// ============================================================================
// Clipboard
// ============================================================================
std::string TzdConsole::clipboardPaste() {
#ifdef _WIN32
    std::string result;
    if (!OpenClipboard(nullptr)) return result;
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData) {
        wchar_t* wide = (wchar_t*)GlobalLock(hData);
        if (wide) {
            int len = (int)GlobalSize(hData) / sizeof(wchar_t);
            // Convert wide to UTF-8
            int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wide, len, nullptr, 0, nullptr, nullptr);
            if (utf8Len > 0) {
                result.resize(utf8Len);
                WideCharToMultiByte(CP_UTF8, 0, wide, len, &result[0], utf8Len, nullptr, nullptr);
                // Remove trailing null
                if (!result.empty() && result.back() == '\0') result.pop_back();
            }
            GlobalUnlock(hData);
        }
    }
    CloseClipboard();
    return result;
#else
    return "";
#endif
}

void TzdConsole::clipboardCopy(const std::string& text) {
#ifdef _WIN32
    if (!OpenClipboard(nullptr)) return;
    EmptyClipboard();
    // Convert UTF-8 to wide
    int wideLen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), (int)text.size(), nullptr, 0);
    if (wideLen > 0) {
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (wideLen + 1) * sizeof(wchar_t));
        if (hMem) {
            wchar_t* wide = (wchar_t*)GlobalLock(hMem);
            MultiByteToWideChar(CP_UTF8, 0, text.c_str(), (int)text.size(), wide, wideLen);
            wide[wideLen] = 0;
            GlobalUnlock(hMem);
            SetClipboardData(CF_UNICODETEXT, hMem);
        }
    }
    CloseClipboard();
#endif
}

// ============================================================================
// Read line with full editing support
// ============================================================================
std::string TzdConsole::readLine(const std::string& prompt) {
    m_lineBuffer.clear();
    m_cursorPos = 0;

    // Print prompt
    printPrompt(prompt);
    std::cout.flush();

#ifdef _WIN32
    // Switch to raw input mode for per-keystroke reading
    DWORD oldMode;
    GetConsoleMode(m_hInput, &oldMode);
    DWORD newMode = 0; // Disable line input, echo, etc.
    SetConsoleMode(m_hInput, newMode);

    INPUT_RECORD ir;
    DWORD eventsRead;

    while (true) {
        if (!ReadConsoleInput(m_hInput, &ir, 1, &eventsRead) || eventsRead == 0) continue;
        if (ir.EventType != KEY_EVENT) continue;
        if (!ir.Event.KeyEvent.bKeyDown) continue;

        WORD vk = ir.Event.KeyEvent.wVirtualKeyCode;
        char ch = ir.Event.KeyEvent.uChar.AsciiChar;
        DWORD ctrl = ir.Event.KeyEvent.dwControlKeyState;

        bool isCtrl = (ctrl & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;

        // Enter key
        if (vk == VK_RETURN) {
            std::cout << "\r\n";
            // Add to history
            if (!m_lineBuffer.empty()) {
                m_history.push_back(m_lineBuffer);
                m_historyIndex = (int)m_history.size();
            }
            break;
        }

        // Backspace
        if (vk == VK_BACK) {
            if (m_cursorPos > 0) {
                m_lineBuffer.erase(m_cursorPos - 1, 1);
                m_cursorPos--;
                renderLine(prompt, m_lineBuffer, m_cursorPos);
            }
            continue;
        }

        // Delete key
        if (vk == VK_DELETE) {
            if (m_cursorPos < m_lineBuffer.size()) {
                m_lineBuffer.erase(m_cursorPos, 1);
                renderLine(prompt, m_lineBuffer, m_cursorPos);
            }
            continue;
        }

        // Arrow keys
        if (vk == VK_LEFT) {
            if (m_cursorPos > 0) {
                m_cursorPos--;
                renderLine(prompt, m_lineBuffer, m_cursorPos);
            }
            continue;
        }
        if (vk == VK_RIGHT) {
            if (m_cursorPos < m_lineBuffer.size()) {
                m_cursorPos++;
                renderLine(prompt, m_lineBuffer, m_cursorPos);
            }
            continue;
        }
        if (vk == VK_HOME) {
            m_cursorPos = 0;
            renderLine(prompt, m_lineBuffer, m_cursorPos);
            continue;
        }
        if (vk == VK_END) {
            m_cursorPos = m_lineBuffer.size();
            renderLine(prompt, m_lineBuffer, m_cursorPos);
            continue;
        }

        // History navigation
        if (vk == VK_UP) {
            if (m_historyIndex > 0) {
                m_historyIndex--;
                m_lineBuffer = m_history[m_historyIndex];
                m_cursorPos = m_lineBuffer.size();
                renderLine(prompt, m_lineBuffer, m_cursorPos);
            }
            continue;
        }
        if (vk == VK_DOWN) {
            if (m_historyIndex < (int)m_history.size() - 1) {
                m_historyIndex++;
                m_lineBuffer = m_history[m_historyIndex];
                m_cursorPos = m_lineBuffer.size();
                renderLine(prompt, m_lineBuffer, m_cursorPos);
            } else {
                m_historyIndex = (int)m_history.size();
                m_lineBuffer.clear();
                m_cursorPos = 0;
                renderLine(prompt, m_lineBuffer, m_cursorPos);
            }
            continue;
        }

        // Ctrl+C: copy current line to clipboard, or abort
        if (isCtrl && ch == 3) { // Ctrl+C
            if (!m_lineBuffer.empty()) {
                clipboardCopy(m_lineBuffer);
                std::cout << "\r\n[Copied to clipboard]\r\n";
                printPrompt(prompt);
                renderLine(prompt, m_lineBuffer, m_cursorPos);
            }
            continue;
        }

        // Ctrl+V: paste from clipboard
        if (isCtrl && ch == 22) { // Ctrl+V
            std::string clip = clipboardPaste();
            if (!clip.empty()) {
                // Remove newlines
                std::string cleaned;
                for (char c : clip) {
                    if (c != '\r' && c != '\n') cleaned += c;
                }
                m_lineBuffer.insert(m_cursorPos, cleaned);
                m_cursorPos += cleaned.size();
                renderLine(prompt, m_lineBuffer, m_cursorPos);
            }
            continue;
        }

        // Ctrl+L: clear screen
        if (isCtrl && ch == 12) {
            clearScreen();
            printPrompt(prompt);
            renderLine(prompt, m_lineBuffer, m_cursorPos);
            continue;
        }

        // Regular printable character
        if (ch >= 32 && ch < 127) {
            m_lineBuffer.insert(m_lineBuffer.begin() + m_cursorPos, ch);
            m_cursorPos++;
            renderLine(prompt, m_lineBuffer, m_cursorPos);
            continue;
        }

        // Tab: auto-complete (basic - just insert spaces for now)
        if (vk == VK_TAB) {
            m_lineBuffer.insert(m_cursorPos, "    ");
            m_cursorPos += 4;
            renderLine(prompt, m_lineBuffer, m_cursorPos);
            continue;
        }
    }

    // Restore input mode
    SetConsoleMode(m_hInput, oldMode);
#else
    // Fallback: use getline
    std::string line;
    std::getline(std::cin, line);
    m_lineBuffer = line;
    if (!m_lineBuffer.empty()) {
        m_history.push_back(m_lineBuffer);
        m_historyIndex = (int)m_history.size();
    }
#endif

    return m_lineBuffer;
}

// ============================================================================
// Run interactive loop with multi-line support
// ============================================================================
void TzdConsole::run(const std::string& prompt, const std::string& continuePrompt,
                     ProcessCallback callback) {
    std::string inputBuffer;
    m_braceCount = 0;
    m_inString = false;

    while (true) {
        std::string p = (inputBuffer.empty()) ? prompt : continuePrompt;
        std::string line = readLine(p);

        // Handle exit
        std::string trimLine = line;
        size_t start = trimLine.find_first_not_of(" \t\r\n");
        if (start != std::string::npos) trimLine = trimLine.substr(start);
        size_t end = trimLine.find_last_not_of(" \t\r\n");
        if (end != std::string::npos) trimLine = trimLine.substr(0, end + 1);

        if (trimLine == "exit" || trimLine == "exit;") break;
        if (trimLine == "clear" || trimLine == "cls") {
            clearScreen();
            inputBuffer.clear();
            m_braceCount = 0;
            m_inString = false;
            continue;
        }
        if (trimLine == "history") {
            for (size_t i = 0; i < m_history.size(); i++) {
                std::cout << "  [" << (i + 1) << "] " << m_history[i] << std::endl;
            }
            continue;
        }

        // Track braces and strings for multi-line input
        bool inQuotes = false;
        for (char c : line) {
            if (c == '"' && (line.size() == 0 || line.back() != '\\')) inQuotes = !inQuotes;
            if (!inQuotes) {
                if (c == '{') m_braceCount++;
                if (c == '}') m_braceCount--;
            }
        }

        inputBuffer += line + "\n";

        // Check if input is complete (braces balanced and ends with ; or })
        std::string tempBuffer = inputBuffer;
        size_t lastCharIdx = tempBuffer.find_last_not_of(" \t\r\n");
        bool endsWithSemi = (lastCharIdx != std::string::npos &&
                            (tempBuffer[lastCharIdx] == ';' || tempBuffer[lastCharIdx] == '}'));

        if (m_braceCount <= 0 && endsWithSemi) {
            bool shouldContinue = callback(inputBuffer);
            if (!shouldContinue) break;
            inputBuffer.clear();
            m_braceCount = 0;
            m_inString = false;
        }
    }
}

// ============================================================================
// Cursor movement helpers
// ============================================================================
void TzdConsole::moveCursorLeft(int n) {
    if (m_vtEnabled) {
        std::cout << "\033[" << n << "D";
    }
}

void TzdConsole::moveCursorRight(int n) {
    if (m_vtEnabled) {
        std::cout << "\033[" << n << "C";
    }
}

void TzdConsole::moveCursorTo(int col) {
    if (m_vtEnabled) {
        std::cout << "\r";
        if (col > 0) std::cout << "\033[" << col << "C";
    }
}
