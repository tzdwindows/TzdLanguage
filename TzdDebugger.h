#ifndef TZD_DEBUGGER_H
#define TZD_DEBUGGER_H

#include "Generated/TzdInterpreter.h"
#include <string>

namespace TzdDebugger {
    extern bool g_DebugActive;
    void startDebugServer(TzdInterpreter* interpreter, const std::string& host, int port);
    void shutdownServer();
    bool hasBreakpointsInFunction(const std::string& file, int startLine, int endLine);
    bool isStepping();
}

#endif
