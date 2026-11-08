package test;

import com.tzd.TzdContext;
import com.tzd.TzdLanguage;

public class TestTzdJava {
    public static void main(String[] args) {
        System.out.println("========================================================");
        System.out.println("  Testing TzdLanguage Java Interface (via JNI & tzd.dll)");
        System.out.println("========================================================\n");

        // 1. Version & Config
        System.out.println("[1] Tzd Native Version: " + TzdLanguage.getVersion());
        System.out.println("    Opt Level: " + TzdLanguage.getOptLevel());
        System.out.println("    JIT Enabled: " + TzdLanguage.isJitEnabled());
        System.out.println("    [PASS] Native Library loaded & Version check\n");

        // 2. Global runString
        System.out.println("[2] Testing TzdLanguage.runString()...");
        int rc = TzdLanguage.runString("var a = 123; var b = 456; var sum = a + b;");
        if (rc != 0) throw new AssertionError("runString failed: " + rc);
        System.out.println("    [PASS] runString succeeded\n");

        // 3. Stateful TzdContext (AutoCloseable)
        System.out.println("[3] Testing TzdContext (eval, call, variables)...");
        try (TzdContext ctx = new TzdContext()) {
            ctx.eval("fun power(base, exp) { var res = 1; for (var i = 0; i < exp; i = i + 1) { res = res * base; } return res; }");
            String powRes = ctx.call("power", "2", "10");
            System.out.println("    power(2, 10) = " + powRes);
            if (!powRes.equals("1024") && !powRes.equals("1024.0")) {
                throw new AssertionError("Expected 1024, got " + powRes);
            }

            // Variable interop
            ctx.setDouble("learning_rate", 0.0035);
            double lr = ctx.getDouble("learning_rate");
            System.out.println("    learning_rate = " + lr);
            if (Math.abs(lr - 0.0035) > 1e-6) throw new AssertionError("Value mismatch");

            ctx.setString("framework_name", "TzdLanguage 2026");
            String fw = ctx.getString("framework_name");
            System.out.println("    framework_name = " + fw);
            if (!fw.equals("TzdLanguage 2026")) throw new AssertionError("String mismatch");

            ctx.eval("var is_ready = true;");
            boolean hasReady = ctx.has("is_ready");
            System.out.println("    has 'is_ready' = " + hasReady);
            if (!hasReady) throw new AssertionError("Expected is_ready to exist");

            // Class Retransformation test in Java
            System.out.println("\n[4] Testing Class Retransformation from Java...");
            ctx.eval("class Device { fun ping() { return \"ping_v1\"; } }");
            ctx.eval("var d = new Device();");
            String ping1 = ctx.eval("return d.ping();");
            System.out.println("    Device.ping() before = " + ping1);
            boolean ok = ctx.retransformClass("Device", "class Device { fun ping() { return \"ping_v2_retransformed\"; } }");
            if (!ok) throw new AssertionError("retransformClass failed");
            String ping2 = ctx.eval("return d.ping();");
            System.out.println("    Device.ping() after = " + ping2);
            if (!ping2.contains("ping_v2_retransformed")) {
                throw new AssertionError("Expected ping_v2_retransformed, got " + ping2);
            }
            System.out.println("    [PASS] Class Retransformation in Java succeeded\n");

            // Object Size introspection test in Java
            System.out.println("[5] Testing Object Size from Java...");
            long dSize = ctx.getVariableSize("d");
            System.out.println("    Variable 'd' (Device instance) memory size: " + dSize + " bytes");
            if (dSize <= 0) throw new AssertionError("Expected positive object size for d");
            System.out.println("    [PASS] Object Size in Java succeeded\n");

            // Dynamic Compression test in Java
            System.out.println("[6] Testing Dynamic Compression from Java...");
            TzdLanguage.CompressionStats stats = ctx.dynamicCompress();
            System.out.println("    " + stats);
            ctx.setDynamicCompression(true);
            if (!ctx.isDynamicCompressionEnabled()) {
                throw new AssertionError("Dynamic compression expected to be enabled");
            }
            System.out.println("    [PASS] Dynamic Compression in Java succeeded\n");

            // Dynamic Compression Events in Java
            System.out.println("[7] Testing Dynamic Compression Events in Java...");
            final int[] eventCount = new int[4]; // 0: start, 1: finish, 2: compressed, 3: decompressed
            ctx.setCompressionEventListener(new TzdLanguage.CompressionEventListener() {
                @Override
                public void onCompressionEvent(TzdLanguage.CompressionEvent event) {
                    if (event.type == TzdLanguage.CompressionEventType.START) eventCount[0]++;
                    else if (event.type == TzdLanguage.CompressionEventType.FINISH) eventCount[1]++;
                    else if (event.type == TzdLanguage.CompressionEventType.OBJECT_COMPRESSED) eventCount[2]++;
                    else if (event.type == TzdLanguage.CompressionEventType.OBJECT_DECOMPRESSED) eventCount[3]++;
                }
            });

            ctx.eval("class Session { var string user; var int token; var ptr extra; Session(u, t) { this.user = u; this.token = t; } }");
            ctx.eval("var s1 = new Session(\"admin\", 1001); var s2 = new Session(\"guest\", 1002);");
            ctx.dynamicCompress();
            if (eventCount[0] < 1 || eventCount[1] < 1) {
                throw new AssertionError("Expected START and FINISH compression events");
            }
            System.out.println("    Captured Java compression events: start=" + eventCount[0] + ", finish=" + eventCount[1] + ", objects=" + eventCount[2]);
            System.out.println("    [PASS] Dynamic Compression Events in Java succeeded\n");

            System.out.println("    [PASS] TzdContext operations all succeeded\n");
        }

        // 8. Hardware & High-Resolution NanoTime API
        System.out.println("[8] Testing Hardware & NanoTime Java API...");
        long t1 = TzdLanguage.nanoTime();
        TzdLanguage.HardwareInfo hw = TzdLanguage.getHardwareInfo();
        long t2 = TzdLanguage.nanoTime();
        System.out.println("    Hardware Info: " + hw);
        System.out.println("    NanoTime delta: " + (t2 - t1) + " ns");
        if (hw == null || hw.logicalCores < 1 || hw.cpuBrand == null) {
            throw new RuntimeException("HardwareInfo validation failed");
        }
        System.out.println("    [PASS] Hardware & NanoTime Java API succeeded\n");

        System.out.println("========================================================");
        System.out.println("  >>> All Tzd Java Interface tests PASSED! <<<");
        System.out.println("========================================================");
    }
}
