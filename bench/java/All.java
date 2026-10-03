public class All {
    static double fib(int n) {
        if (n <= 2) return 1.0;
        return fib(n - 1) + fib(n - 2);
    }

    static int countPrimes(int limit) {
        boolean[] sieve = new boolean[limit];
        for (int i = 2; i < limit; i++) sieve[i] = true;
        for (int p = 2; (long) p * p < limit; p++) {
            if (sieve[p]) {
                for (int m = p * p; m < limit; m += p) sieve[m] = false;
            }
        }
        int count = 0;
        for (int k = 0; k < limit; k++) if (sieve[k]) count++;
        return count;
    }

    static int mandelbrot(int width, int height, int maxIter) {
        int total = 0;
        for (int py = 0; py < height; py++) {
            for (int px = 0; px < width; px++) {
                double x0 = (double) px / width * 3.5 - 2.5;
                double y0 = (double) py / height * 2.0 - 1.0;
                double x = 0.0, y = 0.0;
                int it = 0;
                while (it < maxIter && x * x + y * y <= 4.0) {
                    double xt = x * x - y * y + x0;
                    y = 2.0 * x * y + y0;
                    x = xt;
                    it++;
                }
                total += it;
            }
        }
        return total;
    }

    static long loopSum(long n) {
        long s = 0;
        for (long i = 0; i < n; i++) s += i;
        return s;
    }

    public static void main(String[] args) {
        long t0, t1;

        t0 = System.nanoTime();
        double f = fib(34);
        t1 = System.nanoTime();
        System.out.printf("fib(34)=%s | %.1f ms%n", f, (t1 - t0) / 1e6);

        t0 = System.nanoTime();
        int primes = countPrimes(1000000);
        t1 = System.nanoTime();
        System.out.printf("primes<1e6=%d | %.1f ms%n", primes, (t1 - t0) / 1e6);

        t0 = System.nanoTime();
        int mb = mandelbrot(400, 400, 100);
        t1 = System.nanoTime();
        System.out.printf("mandelbrot_sum=%d | %.1f ms%n", mb, (t1 - t0) / 1e6);

        t0 = System.nanoTime();
        long s = loopSum(100000000L);
        t1 = System.nanoTime();
        System.out.printf("loopSum_1e8=%d | %.1f ms%n", s, (t1 - t0) / 1e6);
    }
}
