public class BenchCompare {
    // 1. Loop sum (1M iterations)
    static long sumLoop(int n) {
        long s = 0;
        int i = 0;
        while (i < n) { s += i; i++; }
        return s;
    }

    // 2. Nested loop (n^2 iterations)
    static long nestedLoop(int n) {
        long s = 0;
        int i = 0;
        while (i < n) {
            int j = 0;
            while (j < n) { s += (long)i * j; j++; }
            i++;
        }
        return s;
    }

    // 3. Fibonacci
    static long fib(int n) {
        if (n <= 1) return n;
        return fib(n - 1) + fib(n - 2);
    }

    // 4. Ackermann
    static long ackermann(long m, long n) {
        if (m == 0) return n + 1;
        if (n == 0) return ackermann(m - 1, 1);
        return ackermann(m - 1, ackermann(m, n - 1));
    }

    // 5. GCD
    static long gcd(long a, long b) {
        if (b == 0) return a;
        return gcd(b, a % b);
    }

    // 6. Prime counting
    static int countPrimes(int n) {
        int count = 0;
        for (int i = 2; i < n; i++) {
            boolean isPrime = true;
            for (int j = 2; j * j <= i; j++) {
                if (i % j == 0) { isPrime = false; break; }
            }
            if (isPrime) count++;
        }
        return count;
    }

    // 7. Function call overhead
    static long noop(long x) { return x + 1; }
    static long callOverhead(int n) {
        long s = 0;
        for (int i = 0; i < n; i++) s = noop(s);
        return s;
    }

    // 8. Newton's sqrt
    static double newtonSqrt(double n, int iter) {
        double x = n;
        for (int i = 0; i < iter; i++) x = (x + n / x) * 0.5;
        return x;
    }

    public static void main(String[] args) {
        // Warmup
        sumLoop(1000000); nestedLoop(100); fib(20); gcd(100, 97);

        System.out.println("=== Java Benchmark ===");

        long t1 = System.nanoTime();
        long r1 = sumLoop(1000000);
        long t2 = System.nanoTime();
        System.out.printf("sumLoop(1M)=%d  time=%.6fs%n", r1, (t2-t1)/1e9);

        long t3 = System.nanoTime();
        long r2 = nestedLoop(1000);
        long t4 = System.nanoTime();
        System.out.printf("nestedLoop(1k)=%d  time=%.6fs%n", r2, (t4-t3)/1e9);

        long t5 = System.nanoTime();
        long r3 = fib(35);
        long t6 = System.nanoTime();
        System.out.printf("fib(35)=%d  time=%.6fs%n", r3, (t6-t5)/1e9);

        long t7 = System.nanoTime();
        long r4 = ackermann(3, 6);
        long t8 = System.nanoTime();
        System.out.printf("ackermann(3,6)=%d  time=%.6fs%n", r4, (t8-t7)/1e9);

        long t9 = System.nanoTime();
        long r5 = gcd(1000003, 999999);
        long t10 = System.nanoTime();
        System.out.printf("gcd=%d  time=%.6fs%n", r5, (t10-t9)/1e9);

        long t11 = System.nanoTime();
        int r6 = countPrimes(10000);
        long t12 = System.nanoTime();
        System.out.printf("primes(10k)=%d  time=%.6fs%n", r6, (t12-t11)/1e9);

        long t13 = System.nanoTime();
        long r7 = callOverhead(1000000);
        long t14 = System.nanoTime();
        System.out.printf("callOverhead(1M)=%d  time=%.6fs%n", r7, (t14-t13)/1e9);

        long t15 = System.nanoTime();
        double r8 = newtonSqrt(2.0, 20);
        long t16 = System.nanoTime();
        System.out.printf("newtonSqrt(2.0,20)=%.15f  time=%.6fs%n", r8, (t16-t15)/1e9);
    }
}
