function fib(n) {
    if (n <= 2) return 1;
    return fib(n - 1) + fib(n - 2);
}

function countPrimes(limit) {
    const sieve = new Array(limit).fill(1);
    sieve[0] = 0;
    sieve[1] = 0;
    for (let p = 2; p * p < limit; p++) {
        if (sieve[p]) {
            for (let m = p * p; m < limit; m += p) sieve[m] = 0;
        }
    }
    let count = 0;
    for (let k = 0; k < limit; k++) if (sieve[k]) count++;
    return count;
}

function mandelbrot(width, height, maxIter) {
    let total = 0;
    for (let py = 0; py < height; py++) {
        for (let px = 0; px < width; px++) {
            const x0 = px / width * 3.5 - 2.5;
            const y0 = py / height * 2.0 - 1.0;
            let x = 0.0, y = 0.0, it = 0;
            while (it < maxIter && x * x + y * y <= 4.0) {
                const xt = x * x - y * y + x0;
                y = 2.0 * x * y + y0;
                x = xt;
                it++;
            }
            total += it;
        }
    }
    return total;
}

function loopSum(n) {
    let s = 0;
    for (let i = 0; i < n; i++) s += i;
    return s;
}

function bench(name, fn, ...args) {
    const t0 = performance.now();
    const r = fn(...args);
    const t1 = performance.now();
    console.log(`${name}=${r} | ${(t1 - t0).toFixed(1)} ms`);
}

bench("fib(34)", fib, 34);
bench("primes<1e6", countPrimes, 1000000);
bench("mandelbrot", mandelbrot, 400, 400, 100);
bench("loopSum_1e8", loopSum, 100000000);
