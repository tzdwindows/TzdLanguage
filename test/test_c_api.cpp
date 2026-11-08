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

    // 7. Dynamic Class Retransformation (Hot Swap)
    std::cout << "[7] Testing Class Retransformation (tzd_retransform_class)...\n";
    tiCtx.eval("class Worker { fun work() { return \"v1_standard\"; } }");
    tiCtx.eval("var w = new Worker();");
    std::string res1 = tiCtx.eval("return w.work();");
    std::cout << "    Before retransform: w.work() = " << res1 << std::endl;

    // Retransform Worker class dynamically
    bool retOk = tiCtx.retransformClass("Worker", "class Worker { fun work() { return \"v2_retransformed_hot_swapped!\"; } }");
    assert(retOk);
    std::string res2 = tiCtx.eval("return w.work();");
    std::cout << "    After retransform: w.work() = " << res2 << std::endl;
    assert(res2.find("v2_retransformed") != std::string::npos);

    // Also verify a new instance uses the retransformed class
    std::string res3 = tiCtx.eval("var w2 = new Worker(); return w2.work();");
    std::cout << "    New instance w2.work() = " << res3 << std::endl;
    assert(res3.find("v2_retransformed") != std::string::npos);
    std::cout << "    [PASS] Class Retransformation (Hot Swap)\n\n";

    // 8. Object Size Calculation (JVMTI GetObjectSize equivalent)
    std::cout << "[8] Testing Object Size API (tzd_get_object_size & tzd_ti_get_object_size)...\n";
    assert(foundCount > 0);
    size_t objSize = 0;
    int sz_rc = tzd_ti_get_object_size(nullptr, objs[0], &objSize);
    assert(sz_rc == 0);
    std::cout << "    Instance 'UserAccount' memory size: " << objSize << " bytes\n";
    assert(objSize >= sizeof(void*));

    size_t instSizeCpp = tiCtx.getObjectSize(objs[0]);
    std::cout << "    Via C++ wrapper: " << instSizeCpp << " bytes\n";
    assert(instSizeCpp == objSize);

    size_t varSize = tiCtx.getVariableSize("w");
    std::cout << "    Variable 'w' size via getVariableSize(): " << varSize << " bytes\n";
    assert(varSize > 0);

    std::cout << "    [PASS] Object Size API\n\n";

    // 9. Dynamic Compression API (动态压缩技术)
    std::cout << "[9] Testing Dynamic Compression API (tzd_dynamic_compress & tzd_compress_instance)...\n" << std::flush;
    tzd_compression_stats_t c_stats = {0};
    int comp_rc = tzd_dynamic_compress(tiCtx.getContext(), &c_stats);
    assert(comp_rc == TZD_OK);
    std::cout << "    Dynamic Compression executed successfully:\n";
    std::cout << "      Objects Scanned:      " << c_stats.total_objects_scanned << "\n";
    std::cout << "      Instances Compressed: " << c_stats.instances_compressed << "\n";
    std::cout << "      Bytes Saved:          " << c_stats.bytes_saved << " bytes\n";
    std::cout << "      Compression Ratio:    " << c_stats.compression_ratio << "%\n";

    // Test C++ wrapper dynamicCompress()
    auto cpp_stats = tiCtx.dynamicCompress();
    std::cout << "    Via C++ wrapper dynamicCompress(): ratio = " << cpp_stats.compression_ratio << "%\n";

    // Test single instance compression / decompression
    assert(foundCount > 0);
    size_t saved = 0;
    int inst_comp_rc = tzd_compress_instance(objs[0], &saved);
    assert(inst_comp_rc == TZD_OK);
    size_t compSize = 0;
    tzd_get_object_size(objs[0], &compSize);
    std::cout << "    Compressed instance size: " << compSize << " bytes (was " << objSize << " bytes)\n";
    assert(compSize <= objSize);

    // Decompress instance
    int de_rc = tzd_decompress_instance(objs[0]);
    assert(de_rc == TZD_OK);
    size_t decompSize = 0;
    tzd_get_object_size(objs[0], &decompSize);
    std::cout << "    Decompressed instance size: " << decompSize << " bytes\n";

    std::cout << "    [PASS] Dynamic Compression API\n\n";

    // 10. Dynamic Compression Events (动态压缩事件系统)
    std::cout << "[10] Testing Dynamic Compression Events (Lifecycle & Object Hooks)...\n" << std::flush;
    static int g_eventStartCount = 0;
    static int g_eventFinishCount = 0;
    static int g_eventCompressedCount = 0;
    static int g_eventDecompressedCount = 0;

    tiCtx.setCompressionEventListener([](const tzd_compression_event_t& evt) {
        if (evt.type == TZD_COMPRESSION_EVENT_START) {
            g_eventStartCount++;
        } else if (evt.type == TZD_COMPRESSION_EVENT_FINISH) {
            g_eventFinishCount++;
        } else if (evt.type == TZD_COMPRESSION_EVENT_OBJECT_COMPRESSED) {
            g_eventCompressedCount++;
        } else if (evt.type == TZD_COMPRESSION_EVENT_OBJECT_DECOMPRESSED) {
            g_eventDecompressedCount++;
        }
    });

    tiCtx.eval("class MetricNode { var string tag; var int count; var ptr p1; var ptr p2; MetricNode(t, c) { this.tag = t; this.count = c; } }");
    tiCtx.eval("var m1 = new MetricNode(\"cpu_load\", 85); var m2 = new MetricNode(\"mem_load\", 42);");

    int prevStart = g_eventStartCount;
    int prevFinish = g_eventFinishCount;
    auto eventStats = tiCtx.dynamicCompress();
    std::cout << "    dynamicCompress() finished: scanned " << eventStats.total_objects_scanned
              << ", saved " << eventStats.bytes_saved << " bytes\n";
    assert(g_eventStartCount > prevStart);
    assert(g_eventFinishCount > prevFinish);
    assert(g_eventCompressedCount > 0);
    std::cout << "    [PASS] Compression Lifecycle & Object Compressed events fired successfully!\n";

    // Test decompress-on-write event
    int prevDecomp = g_eventDecompressedCount;
    tiCtx.eval("m1.count = 99;");
    assert(g_eventDecompressedCount > prevDecomp);
    std::cout << "    [PASS] Object Decompressed event fired on write mutation!\n";
    std::cout << "    [PASS] Dynamic Compression Events System\n\n";

    std::cout << "========================================================\n";
    std::cout << "  >>> All Tzd C/C++ API tests PASSED successfully! <<<\n";
    std::cout << "========================================================\n";
    return 0;
}
