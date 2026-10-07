public class StringBench {
    public static void benchTwoStrings() {
        String a = "Hello, World! This is a test string that is moderately long.";
        String b = " Another piece of string to concatenate.";
        long t0 = System.nanoTime();
        String res = "";
        for (int i = 0; i < 100000; i++) {
            res = a + b;
        }
        long t1 = System.nanoTime();
        System.out.printf("Java Two strings 100k (a + b): %.3f ms, len=%d\n", (t1 - t0) / 1e6, res.length());
    }

    public static void benchMultiConcat() {
        long t0 = System.nanoTime();
        String last = "";
        for (int i = 0; i < 100000; i++) {
            last = "prefix_" + i + "_suffix";
        }
        long t1 = System.nanoTime();
        System.out.printf("Java Multi concat 100k ('prefix_' + i + '_suffix'): %.3f ms, last=%s\n", (t1 - t0) / 1e6, last);
    }

    public static void benchAppend() {
        long t0 = System.nanoTime();
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < 50000; i++) {
            sb.append("a");
        }
        String s = sb.toString();
        long t1 = System.nanoTime();
        System.out.printf("Java StringBuilder 50k append: %.3f ms, len=%d\n", (t1 - t0) / 1e6, s.length());
    }

    public static void main(String[] args) {
        System.out.println("=== Java String Benchmark ===");
        // warmup
        for (int i = 0; i < 3; i++) {
            benchTwoStrings();
            benchMultiConcat();
            benchAppend();
        }
        System.out.println("--- Measured ---");
        benchTwoStrings();
        benchMultiConcat();
        benchAppend();
    }
}
