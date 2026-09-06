// ============================================================================
// TzdConsole.h - Python-like interactive console with syntax highlighting,
//                clipboard support, command history, and line editing
// ============================================================================

#pragma once

#include <string>
#include <vector>
#include <functional>

#ifdef _WIN32
#include <windows.h>
#else
typedef void* HANDLE;
#endif

class TzdConsole {
public:
    // Constructor: initializes console mode, VT processing
    TzdConsole();

    // Destructor: restores original console mode
    ~TzdConsole();

    // Read a single line with highlighting, editing, history, clipboard
    // Returns the line text, or empty string if EOF
    std::string readLine(const std::string& prompt);

    // Process a complete input buffer (multi-line) - calls the callback
    // Returns true if should continue, false if should exit
    using ProcessCallback = std::function<bool(const std::string&)>;
    void run(const std::string& prompt, const std::string& continuePrompt,
             ProcessCallback callback);

    // Print colored text
    static void printColored(const std::string& text, const std::string& color);
    static void printError(const std::string& text);
    static void printSuccess(const std::string& text);
    static void printPrompt(const std::string& text);

    // Enable/disable syntax highlighting
    void setHighlightEnabled(bool enabled) { m_highlightEnabled = enabled; }

    // Clear screen
    void clearScreen();

    // Get history
    const std::vector<std::string>& getHistory() const { return m_history; }

private:
#ifdef _WIN32
    HANDLE m_hInput;
    HANDLE m_hOutput;
    DWORD m_oldInputMode;
    DWORD m_oldOutputMode;
#endif

    bool m_highlightEnabled;
    bool m_vtEnabled;

    // History
    std::vector<std::string> m_history;
    int m_historyIndex;

    // Line editing state
    std::string m_lineBuffer;
    size_t m_cursorPos;

    // Multi-line tracking
    int m_braceCount;
    bool m_inString;
    bool m_inBlockComment;

    // Clipboard
    std::string clipboardPaste();
    void clipboardCopy(const std::string& text);

    // Syntax highlighting
    std::string highlightLine(const std::string& line);
    void renderLine(const std::string& prompt, const std::string& line, size_t cursor);

    // Key handling
    bool handleKey(int ch, bool isControl, const std::string& prompt);

    // Tokenizer for highlighting
    enum class TokenType {
        Keyword, Type, String, Number, Operator,
        Comment, Function, Identifier, Whitespace, Unknown
    };

    struct Token {
        TokenType type;
        std::string text;
        int start;
        int length;
    };

    std::vector<Token> tokenize(const std::string& line);
    std::string tokenColor(TokenType type);

    // Helper: move cursor left/right
    void moveCursorLeft(int n);
    void moveCursorRight(int n);
    void moveCursorTo(int col);

    // ANSI escape helpers
    static std::string ansiColor(const std::string& code);
    static std::string ansiReset();

    // Keywords and types
    static const std::vector<std::string>& getKeywords();
    static const std::vector<std::string>& getTypes();

    // Check if keyword
    static bool isKeyword(const std::string& s);
    static bool isType(const std::string& s);
};
