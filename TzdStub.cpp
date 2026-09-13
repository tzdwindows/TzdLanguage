#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include "TzdExeCompiler.h"

int main(int argc, char* argv[]) {
    // Set console output to UTF-8 for international & Chinese characters
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    int exitCode = 0;
    if (tzd::TzdExeCompiler::tryRunEmbeddedExecutable(argc, argv, &exitCode)) {
        return exitCode;
    }

    // If launched standalone without an embedded payload, show helpful message
    std::cerr << "================================================================\n"
              << "  TzdLang Standalone Runtime Runner Stub (Zero-DLL Edition)\n"
              << "================================================================\n"
              << "  This executable is the base runtime stub for standalone Tzd apps.\n"
              << "  To build a standalone executable from your .tzd / .tzdc script:\n\n"
              << "      TzdTools.exe build <your_script.tzd> -o <output.exe>\n\n"
              << "================================================================\n";
    return 1;
}
