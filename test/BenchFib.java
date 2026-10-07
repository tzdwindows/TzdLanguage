public class BenchFib {
    static long fib(int n) {
        if (n <= 1) return n;
        return fib(n - 1) + fib(n - 2);
    }

    public static void main(String[] args) {
        // Warmup: trigger JIT compilation of fib()
        long warmup = fib(30);
        System.out.println("warmup: fib(30)=" + warmup);

        // Measured run (after JIT)
        long t1 = System.nanoTime();
        long f = fib(30);
        long t2 = System.nanoTime();
        double seconds = (t2 - t1) / 1_000_000_000.0;
        System.out.println("fib(30)=" + f + "  time=" + seconds + "s");

        // Run 3 more times for stable measurement
        for (int i = 0; i < 3; i++) {
            t1 = System.nanoTime();
            f = fib(30);
            t2 = System.nanoTime();
            seconds = (t2 - t1) / 1_000_000_000.0;
            System.out.println("fib(30)=" + f + "  time=" + seconds + "s");
        }
    }
}
