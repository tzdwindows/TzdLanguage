#include "TzdInterpreter.h"
#include "TzdSysModule.h"

#include <windows.h>
#pragma comment(lib, "kernel32.lib")

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <ctime>
#include <sstream>
#include <iomanip>

static std::unordered_map<std::string, TzdValue> executeProcess(const std::string& commandLine) {
    std::unordered_map<std::string, TzdValue> result;
    result["exitCode"] = TzdValue((double)-1);
    result["stdout"] = TzdValue("");
    result["stderr"] = TzdValue("");
    result["ok"] = TzdValue(false);

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    HANDLE hOutRead = NULL, hOutWrite = NULL;
    HANDLE hErrRead = NULL, hErrWrite = NULL;

    if (!CreatePipe(&hOutRead, &hOutWrite, &sa, 0)) return result;
    SetHandleInformation(hOutRead, HANDLE_FLAG_INHERIT, 0);

    if (!CreatePipe(&hErrRead, &hErrWrite, &sa, 0)) {
        CloseHandle(hOutRead);
        CloseHandle(hOutWrite);
        return result;
    }
    SetHandleInformation(hErrRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(STARTUPINFOA));
    si.cb = sizeof(STARTUPINFOA);
    si.hStdOutput = hOutWrite;
    si.hStdError = hErrWrite;
    si.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    // CMD invocation
    std::string cmd = "cmd.exe /c " + commandLine;
    std::vector<char> cmdBuf(cmd.begin(), cmd.end());
    cmdBuf.push_back('\0');

    BOOL success = CreateProcessA(
        NULL, cmdBuf.data(), NULL, NULL, TRUE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi
    );

    CloseHandle(hOutWrite);
    CloseHandle(hErrWrite);

    if (!success) {
        CloseHandle(hOutRead);
        CloseHandle(hErrRead);
        result["stderr"] = TzdValue("Failed to launch process: " + std::to_string(GetLastError()));
        return result;
    }

    std::string outStr;
    std::string errStr;
    char buffer[4096];
    DWORD bytesRead = 0;

    while (ReadFile(hOutRead, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
        outStr.append(buffer, bytesRead);
    }
    while (ReadFile(hErrRead, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
        errStr.append(buffer, bytesRead);
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hOutRead);
    CloseHandle(hErrRead);

    result["exitCode"] = TzdValue((double)exitCode);
    result["stdout"] = TzdValue(outStr);
    result["stderr"] = TzdValue(errStr);
    result["ok"] = TzdValue(exitCode == 0);
    return result;
}

void TzdSysModule::init(TzdInterpreter* interp) {
    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f);
        v.name = name;
        interp->setGlobalVariable(name, v);
    };

    // -------------------------------------------------------------
    // Process & System API
    // -------------------------------------------------------------
    reg("sys_exec", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("sys_exec(cmd) requires command string.");
        std::string cmd = TzdInterpreter::getAsString(args[0]);
        return TzdValue(executeProcess(cmd));
    });

    reg("sys_pid", [](const std::vector<TzdValue>&) -> TzdValue {
        return TzdValue((double)GetCurrentProcessId());
    });

    reg("sys_cpu_count", [](const std::vector<TzdValue>&) -> TzdValue {
        unsigned int c = std::thread::hardware_concurrency();
        return TzdValue((double)(c > 0 ? c : 1));
    });

    reg("sys_memory_info", [](const std::vector<TzdValue>&) -> TzdValue {
        MEMORYSTATUSEX mem;
        mem.dwLength = sizeof(mem);
        GlobalMemoryStatusEx(&mem);

        std::unordered_map<std::string, TzdValue> res;
        res["totalMB"] = TzdValue((double)(mem.ullTotalPhys / (1024 * 1024)));
        res["availMB"] = TzdValue((double)(mem.ullAvailPhys / (1024 * 1024)));
        res["loadPercent"] = TzdValue((double)mem.dwMemoryLoad);
        return TzdValue(res);
    });

    reg("sys_get_env", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        std::string key = TzdInterpreter::getAsString(args[0]);
        char buf[4096] = {0};
        DWORD res = GetEnvironmentVariableA(key.c_str(), buf, sizeof(buf));
        if (res > 0) return TzdValue(std::string(buf));
        return TzdValue("");
    });

    reg("sys_set_env", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2) return TzdValue(false);
        std::string key = TzdInterpreter::getAsString(args[0]);
        std::string val = TzdInterpreter::getAsString(args[1]);
        BOOL res = SetEnvironmentVariableA(key.c_str(), val.c_str());
        return TzdValue(res != 0);
    });

    reg("sys_os_name", [](const std::vector<TzdValue>&) -> TzdValue {
        return TzdValue("Windows");
    });

    // -------------------------------------------------------------
    // Timing & Datetime API
    // -------------------------------------------------------------
    reg("time_now_sec", [](const std::vector<TzdValue>&) -> TzdValue {
        auto now = std::chrono::system_clock::now();
        double sec = std::chrono::duration<double>(now.time_since_epoch()).count();
        return TzdValue(sec);
    });

    reg("time_now_ms", [](const std::vector<TzdValue>&) -> TzdValue {
        auto now = std::chrono::system_clock::now();
        double ms = (double)std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        return TzdValue(ms);
    });

    reg("time_now_ns", [](const std::vector<TzdValue>&) -> TzdValue {
        auto now = std::chrono::high_resolution_clock::now();
        double ns = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        return TzdValue(ns);
    });

    reg("time_sleep", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (!args.empty()) {
            double ms = TzdInterpreter::getAsDoubleInternal(args[0]);
            if (ms > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds((int)ms));
            }
        }
        return TzdValue();
    });

    reg("time_format", [](const std::vector<TzdValue>& args) -> TzdValue {
        std::time_t t;
        if (!args.empty() && args[0].type != TzdValue::NONE) {
            t = (std::time_t)TzdInterpreter::getAsDoubleInternal(args[0]);
        } else {
            t = std::time(nullptr);
        }

        std::string fmt = (args.size() > 1) ? TzdInterpreter::getAsString(args[1]) : "%Y-%m-%d %H:%M:%S";
        std::tm tm;
        localtime_s(&tm, &t);

        char buf[256] = {0};
        std::strftime(buf, sizeof(buf), fmt.c_str(), &tm);
        return TzdValue(std::string(buf));
    });

    reg("time_date_parts", [](const std::vector<TzdValue>& args) -> TzdValue {
        std::time_t t;
        if (!args.empty() && args[0].type != TzdValue::NONE) {
            t = (std::time_t)TzdInterpreter::getAsDoubleInternal(args[0]);
        } else {
            t = std::time(nullptr);
        }

        std::tm tm;
        localtime_s(&tm, &t);

        std::unordered_map<std::string, TzdValue> res;
        res["year"] = TzdValue((double)(tm.tm_year + 1900));
        res["month"] = TzdValue((double)(tm.tm_mon + 1));
        res["day"] = TzdValue((double)tm.tm_mday);
        res["hour"] = TzdValue((double)tm.tm_hour);
        res["minute"] = TzdValue((double)tm.tm_min);
        res["second"] = TzdValue((double)tm.tm_sec);
        res["dayOfWeek"] = TzdValue((double)tm.tm_wday);
        res["dayOfYear"] = TzdValue((double)tm.tm_yday);
        return TzdValue(res);
    });
}
