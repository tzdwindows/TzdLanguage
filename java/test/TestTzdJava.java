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

            System.out.println("    [PASS] TzdContext operations all succeeded\n");
        }

        System.out.println("========================================================");
        System.out.println("  >>> All Tzd Java Interface tests PASSED! <<<");
        System.out.println("========================================================");
    }
}
