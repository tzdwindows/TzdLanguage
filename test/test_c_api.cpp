#include <iostream>
#include <cassert>
#include <cmath>
#include "tzd.hpp"

int main() {
    std::cout << "========================================================\n";
    std::cout << "  Testing TzdLanguage C / C++ Public API (tzd.dll)\n";
    std::cout << "========================================================\n\n";

    // 1. Version and Engine Info
    std::cout << "[1] Engine Version: " << tzd::Engine::getVersion() << std::endl;
    assert(!tzd::Engine::getVersion().empty());
    std::cout << "    Opt Level: " << tzd::Engine::getOptLevel() << std::endl;
    std::cout << "    JIT Enabled: " << (tzd::Engine::isJitEnabled() ? "true" : "false") << std::endl;
    std::cout << "    [PASS] Version & Config API\n\n";

    // 2. Global runString
    std::cout << "[2] Testing tzd_run_string()...\n";
    int r1 = tzd::Engine::runString("var a = 100; var b = 200; var c = a + b;");
    assert(r1 == 0);
    std::cout << "    [PASS] tzd_run_string() executed successfully\n\n";

    // 3. Stateful Context API (C++ Wrapper)
    std::cout << "[3] Testing Stateful Context API (tzd::Context)...\n";
    tzd::Context ctx;

    // Define function and variable
    ctx.eval("fun multiply(x, y) { return x * y; }");
    ctx.eval("var my_score = 98.5;");

    // Retrieve variable
    double score = ctx.getDouble("my_score");
    std::cout << "    Retrieved my_score = " << score << std::endl;
    assert(std::fabs(score - 98.5) < 1e-6);

    // Call function
    std::string mulRes = ctx.call("multiply", { "6", "7" });
    std::cout << "    Called multiply(6, 7) = " << mulRes << std::endl;
    assert(mulRes == "42" || mulRes == "42.0" || mulRes.find("42") != std::string::npos);

    // Set variable from C++ and read back in Tzd
    ctx.setDouble("scale_factor", 1.5);
    ctx.eval("var scaled = my_score * scale_factor;");
    double scaled = ctx.getDouble("scaled");
    std::cout << "    Calculated in Tzd: scaled = " << scaled << " (expected 147.75)\n";
    assert(std::fabs(scaled - 147.75) < 1e-4);

    // Set and get string
    ctx.setString("greeting", "Hello from C++ Embedding!");
    std::string greeting = ctx.getString("greeting");
    std::cout << "    Greeting = " << greeting << std::endl;
    assert(greeting == "Hello from C++ Embedding!");

    std::cout << "    [PASS] Stateful Context & Variable Interop\n\n";

    // 4. Pure C API direct test
    std::cout << "[4] Testing Pure C API (tzd.h)...\n";
    tzd_context_t* c_ctx = tzd_create_context();
    assert(c_ctx != nullptr);

    int c_rc = tzd_eval_string(c_ctx, "fun add3(a, b, c) { return a + b + c; }", nullptr, 0);
    assert(c_rc == TZD_OK);

    const char* c_args[] = { "10", "20", "30" };
    char c_out[128] = {0};
    c_rc = tzd_call_function(c_ctx, "add3", 3, c_args, c_out, sizeof(c_out));
    assert(c_rc == TZD_OK);
    std::cout << "    C API add3(10, 20, 30) = " << c_out << std::endl;

    tzd_destroy_context(c_ctx);
    std::cout << "    [PASS] Pure C API Functions\n\n";

    // 5. Thread Management API
    std::cout << "[5] Testing Thread Management API...\n";
    size_t threadCount = tzd::ThreadManager::getCount();
    std::cout << "    Active thread count: " << threadCount << std::endl;
    auto threadIds = tzd::ThreadManager::getAllIds();
    std::cout << "    Enumerated thread IDs count: " << threadIds.size() << std::endl;
    uint64_t currentTid = tzd::ThreadManager::getCurrentId();
    std::cout << "    Current Thread ID: " << currentTid << std::endl;

    tzd::ThreadInfo tinfo;
    if (tzd::ThreadManager::getInfo(currentTid, tinfo)) {
        std::cout << "    Thread info: ID=" << tinfo.id << ", Name=\"" << tinfo.name
                  << "\", State=" << tinfo.stateStr << ", Alive=" << (tinfo.isAlive ? "true" : "false") << std::endl;
    }
    std::string threadDump = tzd::ThreadManager::getDump();
    assert(!threadDump.empty());
    std::cout << "    Thread dump length: " << threadDump.size() << " bytes\n";
    std::cout << "    [PASS] Thread Management API\n\n";

    // 6. TI Interface: get_objects_by_class & thread events
    std::cout << "[6] Testing TZD TI (get_objects_by_class)...\n";
    tzd::Context tiCtx;
    tiCtx.eval("class UserAccount { var string name; UserAccount(n) { this.name = n; } }");
    tiCtx.eval("var u1 = new UserAccount(\"Alice\"); var u2 = new UserAccount(\"Bob\");");

    void* objs[16] = { nullptr };
    size_t foundCount = 0;
    int ti_rc = tzd_ti_get_objects_by_class(nullptr, tiCtx.getContext(), "UserAccount", objs, 16, &foundCount);
    std::cout << "    tzd_ti_get_objects_by_class(\"UserAccount\") returned: " << foundCount << " instances\n";
    assert(ti_rc == 0);
    assert(foundCount >= 2);
    std::cout << "    [PASS] TZD TI get_objects_by_class\n\n";

    std::cout << "========================================================\n";
    std::cout << "  >>> All Tzd C/C++ API tests PASSED successfully! <<<\n";
    std::cout << "========================================================\n";
    return 0;
}
