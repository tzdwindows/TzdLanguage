public class MathBench {
    public static double square(double x) {
        return x * x;
    }

    public double instSquare(double x) {
        return x * x;
    }

    public static double testStatic(int iterations) {
        double acc = 0;
        int i = 0;
        while (i < iterations) {
            acc = acc + MathBench.square(i);
            i = i + 1;
        }
        return acc;
    }

    public static double testInstance(int iterations) {
        MathBench bench = new MathBench();
        double acc = 0;
        int i = 0;
        while (i < iterations) {
            acc = acc + bench.instSquare(i);
            i = i + 1;
        }
        return acc;
    }

    public static void main(String[] args) {
        // Warmup
        for (int w = 0; w < 5; w++) {
            testStatic(100000);
            testInstance(100000);
        }

        long t0 = System.nanoTime();
        double sRes = testStatic(100000);
        long t1 = System.nanoTime();
        double sTime = (t1 - t0) / 1e6;

        long t2 = System.nanoTime();
        double iRes = testInstance(100000);
        long t3 = System.nanoTime();
        double iTime = (t3 - t2) / 1e6;

        System.out.printf("Java Static (100k):   %.4f ms (Result: %.0f)%n", sTime, sRes);
        System.out.printf("Java Instance (100k): %.4f ms (Result: %.0f)%n", iTime, iRes);

        // 1M iterations
        for (int w = 0; w < 5; w++) {
            testStatic(1000000);
            testInstance(1000000);
        }

        t0 = System.nanoTime();
        sRes = testStatic(1000000);
        t1 = System.nanoTime();
        sTime = (t1 - t0) / 1e6;

        t2 = System.nanoTime();
        iRes = testInstance(1000000);
        t3 = System.nanoTime();
        iTime = (t3 - t2) / 1e6;

        System.out.printf("Java Static (1M):     %.4f ms (Result: %.0f)%n", sTime, sRes);
        System.out.printf("Java Instance (1M):   %.4f ms (Result: %.0f)%n", iTime, iRes);
    }
}
