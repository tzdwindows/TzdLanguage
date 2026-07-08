#define _WINSOCKAPI_

#include "Generated/TzdInterpreter.h"
#include "TzdDebugger.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <sstream>
#include <fstream>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <map>
#include <set>
#include <vector>
#include <algorithm>

namespace TzdDebugger {

bool g_DebugActive = false;

struct Breakpoint {
    int id;
    std::string file;
    int line;
};

struct DebugState {
    std::mutex mutex;
    std::condition_variable cond;
    bool isSuspended = false;

    // Stepping modes
    bool stepInto = false;
    bool stepOver = false;
    bool stepOut = false;
    size_t targetCallDepth = 0;

    std::vector<Breakpoint> breakpoints;
    int nextBreakpointId = 1;

    SOCKET clientSocket = INVALID_SOCKET;
    TzdInterpreter* interpreter = nullptr;

    // State to avoid re-entry of debugger during eval
    bool isEvaluating = false;
};

static DebugState g_DebugState;

std::string formatTzdValue(const TzdValue& val) {
    switch (val.type) {
        case TzdValue::NONE: return "null";
        case TzdValue::BOOL: return val.bVal ? "true" : "false";
        case TzdValue::STRING: return "\"" + val.sVal + "\"";
        case TzdValue::DOUBLE:
        case TzdValue::FLOAT: return std::to_string(val.dVal);
        case TzdValue::LONG:
        case TzdValue::INT:
        case TzdValue::SHORT:
        case TzdValue::SBYTE: return std::to_string(val.lVal);
        case TzdValue::ULONG:
        case TzdValue::UINT:
        case TzdValue::USHORT:
        case TzdValue::BYTE: return std::to_string(val.ulVal);
        case TzdValue::POINTER: {
            char buf[64];
            snprintf(buf, sizeof(buf), "%p", val.ptrVal);
            return std::string("pointer(") + buf + ")";
        }
        case TzdValue::ARRAY: return "array(size=" + std::to_string(val.arrVal.size()) + ")";
        case TzdValue::MAP: return "map(size=" + std::to_string(val.mapVal.size()) + ")";
        case TzdValue::FUNCTION: return "function " + val.name + "(" + std::to_string(val.params.size()) + " args)";
        case TzdValue::NATIVE_FUNCTION: return "native_function " + val.name;
        case TzdValue::CLASS_DEF: return "class " + val.name;
        case TzdValue::INSTANCE: return "instance of class";
        default: return "unknown_type";
    }
}

void checkBreakpointAndSuspend(TzdInterpreter* interpreter, const std::string& file, int line) {
    {
        std::ofstream logFile("C:\\Users\\tzdwindows 7\\debug.log", std::ios::app);
        logFile << "[DEBUG_HOOK] File: " << file << ", Line: " << line << ", Depth: " << interpreter->m_callStackFrames.size() << "\n";
    }

    if (g_DebugState.isEvaluating) return;

    // Check if we need to wait for debugger connection at startup
    static bool firstStatement = true;
    if (firstStatement && g_DebugState.interpreter != nullptr && g_DebugState.clientSocket == INVALID_SOCKET) {
        std::unique_lock<std::mutex> lock(g_DebugState.mutex);
        if (g_DebugState.clientSocket == INVALID_SOCKET) {
            std::cout << "[DEBUGGER] Paused at startup. Waiting for debugger client to connect..." << std::endl;
            g_DebugState.cond.wait(lock, []() { return g_DebugState.clientSocket != INVALID_SOCKET; });
        }
        firstStatement = false;
        g_DebugState.isSuspended = true;

        std::string msg = "\n*BREAK* Attached. Paused at startup (line " + std::to_string(line) + ")\n";
        msg += "Type ':bp add <line>' to add breakpoints, ':c' to resume.\nTzdDebug> ";
        send(g_DebugState.clientSocket, msg.c_str(), (int)msg.size(), 0);

        g_DebugState.cond.wait(lock, []() { return !g_DebugState.isSuspended; });
        return;
    }
    firstStatement = false;

    size_t currentDepth = interpreter->m_callStackFrames.size();
    bool shouldSuspend = false;
    std::string reason = "";

    {
        std::unique_lock<std::mutex> lock(g_DebugState.mutex);

        // A. Step Into
        if (g_DebugState.stepInto) {
            shouldSuspend = true;
            reason = "step into";
            g_DebugState.stepInto = false;
        }
        // B. Step Over
        else if (g_DebugState.stepOver && currentDepth <= g_DebugState.targetCallDepth) {
            shouldSuspend = true;
            reason = "step over";
            g_DebugState.stepOver = false;
            g_DebugState.stepInto = false;
            g_DebugState.stepOut = false;
        }
        // C. Step Out
        else if (g_DebugState.stepOut && currentDepth < g_DebugState.targetCallDepth) {
            shouldSuspend = true;
            reason = "step out";
            g_DebugState.stepOut = false;
            g_DebugState.stepInto = false;
            g_DebugState.stepOver = false;
        }
        // D. Breakpoint
        else {
            for (const auto& bp : g_DebugState.breakpoints) {
                bool fileMatch = bp.file.empty();
                if (!fileMatch) {
                    std::string f1 = file;
                    std::string f2 = bp.file;
                    std::transform(f1.begin(), f1.end(), f1.begin(), ::tolower);
                    std::transform(f2.begin(), f2.end(), f2.begin(), ::tolower);
                    if (f1.find(f2) != std::string::npos || f2.find(f1) != std::string::npos) {
                        fileMatch = true;
                    }
                }

                if (fileMatch && bp.line == line) {
                    shouldSuspend = true;
                    reason = "breakpoint " + std::to_string(bp.id);
                    break;
                }
            }
        }
    }

    if (shouldSuspend) {
        std::unique_lock<std::mutex> lock(g_DebugState.mutex);
        g_DebugState.isSuspended = true;

        if (g_DebugState.clientSocket != INVALID_SOCKET) {
            std::string msg = "\n*BREAK* Hit " + reason + " at " + file + ":" + std::to_string(line);
            if (!interpreter->m_callStackFrames.empty()) {
                msg += " (frame: " + interpreter->m_callStackFrames.back() + ")";
            }
            msg += "\nType ':c' to resume, ':locals' for vars, ':bt' for callstack.\nTzdDebug> ";
            send(g_DebugState.clientSocket, msg.c_str(), (int)msg.size(), 0);
        }

        g_DebugState.cond.wait(lock, []() { return !g_DebugState.isSuspended; });
    }
}

// Helper send function
void sendStr(SOCKET sock, const std::string& str) {
    if (sock != INVALID_SOCKET) {
        send(sock, str.c_str(), (int)str.size(), 0);
    }
}

void processDebugCommand(SOCKET clientSocket, TzdInterpreter* interpreter, const std::string& line) {
    if (line.empty()) {
        sendStr(clientSocket, "TzdDebug> ");
        return;
    }

    if (line[0] == ':') {
        std::string cmd = line.substr(1);
        std::stringstream ss(cmd);
        std::string action;
        ss >> action;

        if (action == "bp") {
            std::string subAction;
            ss >> subAction;
            if (subAction == "add") {
                std::string fileOrLine;
                ss >> fileOrLine;
                std::string lineStr;
                if (ss >> lineStr) {
                    int bpLine = std::stoi(lineStr);
                    std::unique_lock<std::mutex> lock(g_DebugState.mutex);
                    Breakpoint bp{ g_DebugState.nextBreakpointId++, fileOrLine, bpLine };
                    g_DebugState.breakpoints.push_back(bp);
                    sendStr(clientSocket, "Breakpoint " + std::to_string(bp.id) + " added at " + fileOrLine + ":" + std::to_string(bpLine) + "\n");
                } else if (!fileOrLine.empty()) {
                    int bpLine = std::stoi(fileOrLine);
                    std::unique_lock<std::mutex> lock(g_DebugState.mutex);
                    Breakpoint bp{ g_DebugState.nextBreakpointId++, "", bpLine };
                    g_DebugState.breakpoints.push_back(bp);
                    sendStr(clientSocket, "Breakpoint " + std::to_string(bp.id) + " added at line " + std::to_string(bpLine) + "\n");
                } else {
                    sendStr(clientSocket, "Usage: :bp add [file] <line>\n");
                }
            }
            else if (subAction == "list") {
                std::unique_lock<std::mutex> lock(g_DebugState.mutex);
                std::stringstream reply;
                reply << "=== Active Breakpoints ===\n";
                if (g_DebugState.breakpoints.empty()) {
                    reply << "No breakpoints set.\n";
                } else {
                    for (const auto& bp : g_DebugState.breakpoints) {
                        reply << "  [" << bp.id << "] " 
                              << (bp.file.empty() ? "*any*" : bp.file) 
                              << ":" << bp.line << "\n";
                    }
                }
                sendStr(clientSocket, reply.str());
            }
            else if (subAction == "del") {
                int bpId = -1;
                if (ss >> bpId) {
                    std::unique_lock<std::mutex> lock(g_DebugState.mutex);
                    auto it = std::remove_if(g_DebugState.breakpoints.begin(), g_DebugState.breakpoints.end(),
                        [bpId](const Breakpoint& bp) { return bp.id == bpId; });
                    if (it != g_DebugState.breakpoints.end()) {
                        g_DebugState.breakpoints.erase(it, g_DebugState.breakpoints.end());
                        sendStr(clientSocket, "Breakpoint " + std::to_string(bpId) + " deleted.\n");
                    } else {
                        sendStr(clientSocket, "Breakpoint ID " + std::to_string(bpId) + " not found.\n");
                    }
                } else {
                    sendStr(clientSocket, "Usage: :bp del <id>\n");
                }
            }
            else {
                sendStr(clientSocket, "Usage: :bp [add|list|del] ...\n");
            }
        }
        else if (action == "step" || action == "over") {
            std::unique_lock<std::mutex> lock(g_DebugState.mutex);
            if (!g_DebugState.isSuspended) {
                sendStr(clientSocket, "Not suspended. Cannot step.\n");
            } else {
                g_DebugState.stepOver = true;
                g_DebugState.targetCallDepth = interpreter->m_callStackFrames.size();
                g_DebugState.isSuspended = false;
                g_DebugState.cond.notify_all();
                return; // Do not send prompt, interpreter is running
            }
        }
        else if (action == "into") {
            std::unique_lock<std::mutex> lock(g_DebugState.mutex);
            if (!g_DebugState.isSuspended) {
                sendStr(clientSocket, "Not suspended. Cannot step.\n");
            } else {
                g_DebugState.stepInto = true;
                g_DebugState.isSuspended = false;
                g_DebugState.cond.notify_all();
                return;
            }
        }
        else if (action == "out") {
            std::unique_lock<std::mutex> lock(g_DebugState.mutex);
            if (!g_DebugState.isSuspended) {
                sendStr(clientSocket, "Not suspended. Cannot step.\n");
            } else {
                g_DebugState.stepOut = true;
                g_DebugState.targetCallDepth = interpreter->m_callStackFrames.size();
                g_DebugState.isSuspended = false;
                g_DebugState.cond.notify_all();
                return;
            }
        }
        else if (action == "resume" || action == "c") {
            std::unique_lock<std::mutex> lock(g_DebugState.mutex);
            if (!g_DebugState.isSuspended) {
                sendStr(clientSocket, "Not suspended. Cannot resume.\n");
            } else {
                g_DebugState.isSuspended = false;
                g_DebugState.cond.notify_all();
                return;
            }
        }
        else if (action == "stack" || action == "bt") {
            std::stringstream reply;
            reply << "=== Call Stack (depth: " << interpreter->m_callStackFrames.size() << ") ===\n";
            if (interpreter->m_callStackFrames.empty()) {
                reply << "  [0] <global_scope>\n";
            } else {
                for (int i = (int)interpreter->m_callStackFrames.size() - 1; i >= 0; --i) {
                    reply << "  [" << i + 1 << "] " << interpreter->m_callStackFrames[i] << "\n";
                }
                reply << "  [0] <global_scope>\n";
            }
            sendStr(clientSocket, reply.str());
        }
        else if (action == "locals") {
            if (interpreter->scopes.empty()) {
                sendStr(clientSocket, "No active scope.\n");
            } else {
                std::stringstream reply;
                reply << "=== Local Variables (frame depth: " << interpreter->m_callStackFrames.size() << ") ===\n";
                const auto& currentScope = interpreter->scopes.back();
                if (currentScope.empty()) {
                    reply << "(empty scope)\n";
                } else {
                    for (const auto& [name, val] : currentScope) {
                        reply << "  " << name << " = " << formatTzdValue(val) << "\n";
                    }
                }
                sendStr(clientSocket, reply.str());
            }
        }
        else if (action == "globals") {
            if (interpreter->scopes.empty()) {
                sendStr(clientSocket, "No active scope.\n");
            } else {
                std::stringstream reply;
                reply << "=== Global Variables ===\n";
                const auto& globalScope = interpreter->scopes.front();
                if (globalScope.empty()) {
                    reply << "(empty scope)\n";
                } else {
                    for (const auto& [name, val] : globalScope) {
                        reply << "  " << name << " = " << formatTzdValue(val) << "\n";
                    }
                }
                sendStr(clientSocket, reply.str());
            }
        }
        else if (action == "fun") {
            std::string sub;
            if (ss >> sub) {
                if (sub == "list") {
                    if (interpreter->scopes.empty()) return;
                    std::stringstream reply;
                    reply << "=== User Functions ===\n";
                    const auto& globalScope = interpreter->scopes.front();
                    int count = 0;
                    for (const auto& [name, val] : globalScope) {
                        if (val.type == TzdValue::FUNCTION) {
                            reply << "  " << name << " (defined at " << val.sourceFile << ":" << val.line << ")\n";
                            count++;
                        }
                    }
                    if (count == 0) {
                        reply << "No user-defined functions found.\n";
                    }
                    sendStr(clientSocket, reply.str());
                }
                else if (sub == "view") {
                    std::string funName;
                    if (ss >> funName) {
                        if (interpreter->scopes.empty()) return;
                        const auto& globalScope = interpreter->scopes.front();
                        if (!globalScope.count(funName)) {
                            sendStr(clientSocket, "Function '" + funName + "' not found.\n");
                        } else {
                            const auto& val = globalScope.at(funName);
                            if (val.type != TzdValue::FUNCTION) {
                                sendStr(clientSocket, "'" + funName + "' is not a function.\n");
                            } else {
                                std::stringstream reply;
                                reply << "Function: " << funName << "\n";
                                reply << "File: " << val.sourceFile << "\n";
                                reply << "Line: " << val.line << "\n";
                                reply << "Parameters: ";
                                for (size_t i = 0; i < val.params.size(); ++i) {
                                    reply << val.params[i] << (i + 1 < val.params.size() ? ", " : "");
                                }
                                reply << "\n";
                                sendStr(clientSocket, reply.str());
                            }
                        }
                    } else {
                        sendStr(clientSocket, "Usage: :fun view <name>\n");
                    }
                }
            } else {
                sendStr(clientSocket, "Usage: :fun [list|view] ...\n");
            }
        }
        else if (action == "info") {
            std::stringstream reply;
            reply << "=== Debugger Info ===\n"
                  << "  Status: " << (g_DebugState.isSuspended ? "SUSPENDED" : "RUNNING") << "\n"
                  << "  Call Depth: " << interpreter->m_callStackFrames.size() << "\n"
                  << "  Active Breakpoints: " << g_DebugState.breakpoints.size() << "\n";
            sendStr(clientSocket, reply.str());
        }
        else {
            sendStr(clientSocket, "Unknown debug command: :" + action + "\n");
        }
    }
    else {
        // Normal expression evaluation
        std::stringstream outputBuffer;
        std::streambuf* oldCout = std::cout.rdbuf(outputBuffer.rdbuf());
        std::streambuf* oldCerr = std::cerr.rdbuf(outputBuffer.rdbuf());

        bool oldSilent = interpreter->m_silentMode;
        interpreter->m_silentMode = false; // echo values inside debug terminal

        g_DebugState.isEvaluating = true;

        try {
            interpreter->loadScript(line);
        }
        catch (const std::exception& ex) {
            outputBuffer << "Error: " << ex.what() << "\n";
        }

        g_DebugState.isEvaluating = false;
        interpreter->m_silentMode = oldSilent;

        std::cout.rdbuf(oldCout);
        std::cerr.rdbuf(oldCerr);

        std::string response = outputBuffer.str();
        if (response.empty()) {
            response = "(no output)\n";
        }
        sendStr(clientSocket, response);
    }

    sendStr(clientSocket, "TzdDebug> ");
}

void startDebugServer(TzdInterpreter* interpreter, const std::string& host, int port) {
    g_DebugActive = true;
    g_DebugState.interpreter = interpreter;
    
    std::thread([host, port]() {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "[DEBUGGER] WSAStartup failed" << std::endl;
            return;
        }
        SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listenSocket == INVALID_SOCKET) {
            std::cerr << "[DEBUGGER] socket creation failed" << std::endl;
            WSACleanup();
            return;
        }
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

        if (bind(listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            std::cerr << "[DEBUGGER] bind failed for " << host << ":" << port << std::endl;
            closesocket(listenSocket);
            WSACleanup();
            return;
        }
        if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "[DEBUGGER] listen failed" << std::endl;
            closesocket(listenSocket);
            WSACleanup();
            return;
        }

        std::cout << "[DEBUGGER] Server listening on " << host << ":" << port << "..." << std::endl;

        while (true) {
            SOCKET clientSocket = accept(listenSocket, NULL, NULL);
            if (clientSocket == INVALID_SOCKET) break;

            // Only allow one debug client at a time for state consistency
            {
                std::unique_lock<std::mutex> lock(g_DebugState.mutex);
                if (g_DebugState.clientSocket != INVALID_SOCKET) {
                    std::string busy = "Debugger is busy with another client connection.\n";
                    send(clientSocket, busy.c_str(), (int)busy.size(), 0);
                    closesocket(clientSocket);
                    continue;
                }
                g_DebugState.clientSocket = clientSocket;
                g_DebugState.cond.notify_all();
            }

            std::thread([clientSocket]() {
                std::string welcome = "--- Tzd Remote Debugger Service ---\n";
                welcome += "Use ':' prefix for debug actions (e.g. :bp add 10, :locals, :bt, :step, :into, :out, :c)\n";
                welcome += "Or type any TzdLang expression to evaluate dynamically in the current scope.\n";
                welcome += "Type 'exit' to disconnect.\n\nTzdDebug> ";
                send(clientSocket, welcome.c_str(), (int)welcome.size(), 0);

                char buf[4096];
                std::string recvBuffer;
                while (true) {
                    int bytesReceived = recv(clientSocket, buf, sizeof(buf) - 1, 0);
                    if (bytesReceived <= 0) break;
                    buf[bytesReceived] = '\0';
                    recvBuffer += buf;

                    size_t pos;
                    while ((pos = recvBuffer.find('\n')) != std::string::npos) {
                        std::string line = recvBuffer.substr(0, pos);
                        recvBuffer.erase(0, pos + 1);

                        if (!line.empty() && line.back() == '\r') {
                            line.pop_back();
                        }

                        if (line == "exit") {
                            std::string goodbye = "Disconnecting...\n";
                            send(clientSocket, goodbye.c_str(), (int)goodbye.size(), 0);
                            closesocket(clientSocket);
                            
                            std::unique_lock<std::mutex> lock(g_DebugState.mutex);
                            g_DebugState.clientSocket = INVALID_SOCKET;
                            g_DebugState.isSuspended = false;
                            g_DebugState.cond.notify_all();
                            return;
                        }

                        processDebugCommand(clientSocket, g_DebugState.interpreter, line);
                    }
                }
                closesocket(clientSocket);
                
                std::unique_lock<std::mutex> lock(g_DebugState.mutex);
                g_DebugState.clientSocket = INVALID_SOCKET;
                g_DebugState.isSuspended = false;
                g_DebugState.cond.notify_all();
            }).detach();
        }
        closesocket(listenSocket);
        WSACleanup();
    }).detach();
}

} // namespace TzdDebugger

// Implementation of TzdInterpreter::visit
std::any TzdInterpreter::visit(antlr4::tree::ParseTree *tree) {
    if (tree != nullptr) {
        TzdLangParser::StatementContext* stmt = dynamic_cast<TzdLangParser::StatementContext*>(tree);
        if (stmt != nullptr) {
            antlr4::Token* startToken = stmt->getStart();
            if (startToken != nullptr) {
                int line = startToken->getLine();
                std::string file = "memory";
                if (!this->m_debugFileStack.empty()) {
                    file = this->m_debugFileStack.back();
                } else if (!this->m_scriptPathStack.empty()) {
                    file = this->m_scriptPathStack.back().string();
                }
                TzdDebugger::checkBreakpointAndSuspend(this, file, line);
            }
        }
    }
    return TzdLangBaseVisitor::visit(tree);
}
