import java.util.HashMap;

public class Round2 {
    static final int N = 300000;
    static long[] G = new long[N];

    // K1: string build
    static int stringBuild(int n) {
        String s = "";
        for (int i = 0; i < n; i++) {
            s = s + Integer.toString(i) + "-" + Integer.toString(i % 7);
        }
        return s.length();
    }

    // K2: Monte Carlo pi via Lehmer LCG
    static double monteCarlo(int n) {
        long x = 1;
        int count = 0;
        for (int i = 0; i < n; i++) {
            x = (x * 16807L) % 2147483647L;
            long y = (x * 16807L) % 2147483647L;
            double fx = x / 2147483647.0;
            double fy = y / 2147483647.0;
            if (fx * fx + fy * fy <= 1.0) count++;
        }
        return 4.0 * count / n;
    }

    // K3: map insert + lookup
    static long mapOps(int n) {
        HashMap<String, Integer> m = new HashMap<>();
        for (int i = 0; i < n; i++) m.put(Integer.toString(i), i);
        long sum = 0;
        for (int i = 0; i < n; i++) sum = sum + m.get(Integer.toString(i));
        return sum;
    }

    // K4: OOP allocation + dynamic dispatch
    static class Pt {
        double x, y;
        Pt(double x, double y) { this.x = x; this.y = y; }
        double d2(Pt p) {
            double dx = this.x - p.x;
            double dy = this.y - p.y;
            return dx * dx + dy * dy;
        }
    }

    static double oopDispatch(int n) {
        long x = 1;
        double sum = 0.0;
        for (int i = 0; i < n; i++) {
            x = (x * 16807L) % 2147483647L;
            long a = x % 1000;
            x = (x * 16807L) % 2147483647L;
            long b = x % 1000;
            x = (x * 16807L) % 2147483647L;
            long c = x % 1000;
            x = (x * 16807L) % 2147483647L;
            long d = x % 1000;
            Pt p1 = new Pt(a * 1.0, b * 1.0);
            Pt p2 = new Pt(c * 1.0, d * 1.0);
            sum = sum + p1.d2(p2);
        }
        return sum;
    }

    // K5: hand-written Lomuto quicksort over a static array
    static void qsortG(long lo, long hi) {
        if (lo >= hi) return;
        long p = G[(int) hi];
        long i = lo - 1;
        long j = lo;
        while (j < hi) {
            if (G[(int) j] <= p) {
                i++;
                long t = G[(int) i]; G[(int) i] = G[(int) j]; G[(int) j] = t;
            }
            j++;
        }
        i++;
        long t2 = G[(int) i]; G[(int) i] = G[(int) hi]; G[(int) hi] = t2;
        qsortG(lo, i - 1);
        qsortG(i + 1, hi);
    }

    static long quickSort() {
        long x = 1;
        long fillSum = 0;
        for (int i = 0; i < N; i++) {
            x = (x * 16807L) % 2147483647L;
            long v = x % 1000000007L;
            G[i] = v;
            fillSum += v;
        }
        qsortG(0, N - 1);
        System.out.printf("qsort min=%d max=%d fill=%d%n", G[0], G[N - 1], fillSum);
        return fillSum;
    }

    static void bench(String name, Runnable r) {
        long t0 = System.nanoTime();
        r.run();
        long t1 = System.nanoTime();
        System.out.printf("%s | %.1f ms%n", name, (t1 - t0) / 1e6);
    }

    public static void main(String[] args) {
        final int[] lastLen = new int[1];
        bench("stringBuild", () -> lastLen[0] = stringBuild(20000));
        System.out.println("stringBuild len=" + lastLen[0]);

        final double[] piOut = new double[1];
        bench("monteCarlo", () -> piOut[0] = monteCarlo(1000000));
        System.out.println("monteCarlo pi=" + piOut[0]);

        final long[] sumOut = new long[1];
        bench("mapOps", () -> sumOut[0] = mapOps(200000));
        System.out.println("mapOps sum=" + sumOut[0]);

        final double[] oopOut = new double[1];
        bench("oop", () -> oopOut[0] = oopDispatch(200000));
        System.out.println("oop sum=" + oopOut[0]);

        bench("qsort", () -> quickSort());
    }
}
